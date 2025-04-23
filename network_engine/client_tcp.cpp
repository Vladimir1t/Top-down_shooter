#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <string>
#include <array>
#include <thread>
#include <mutex>
#include <cstdlib>

#include "game_state.hpp"

static std::mutex state_mutex;
static int index_cli;
static sf::Clock clock_fps;
static sf::Time delta_time;

static void network_handler(game::control_struct& ctrl_handler, game::game_state_client& global_state,
    sf::TcpSocket& server, ushort& player_count) {

    sf::Packet incoming_state;
    sf::Packet outcoming_data;

    while (true) {
        if (server.receive(incoming_state) != sf::Socket::Status::Done) {
            std::cerr << "Error while recieving" << std::endl;
        }
        else {
            delta_time = clock_fps.restart();

            uint64_t unique_index;
            float x, y, rot;
            int sprite_status;
            incoming_state >> player_count;
            // std::cout << "rn " << player_count << " are playing" << std::endl;

            std::lock_guard<std::mutex> lock(state_mutex);

            for (int i = 0; i < player_count; ++i) {
                incoming_state >> unique_index >> x >> y >> rot >> sprite_status;
                // std::cout << "Player: " << unique_index << "\n\tx: " << x << "\n\ty: " << y << "\n\tr: " 
                //           << rot << "\n\ts: " << sprite_status << std::endl;
                game::object& obj = global_state.player_objects[unique_index];
                /* --- information about each mob --- */
                obj.setPosition({x, y});
                obj.setRotation(sf::radians(rot));
                obj.sprite_status = sprite_status;
            }
        }
        if (ctrl_handler.changed) {
            outcoming_data << ctrl_handler.move_x << ctrl_handler.move_y 
                           << ctrl_handler.rotate << ctrl_handler.sprite_status;
            ctrl_handler.changed = 0;
            if(server.send(outcoming_data) != sf::Socket::Status::Done) {
                std::cout << "error while sending to server\n";
            }
        }
        
        incoming_state.clear();
        outcoming_data.clear();
    }
}

static void change_status_sprite(sf::Clock& clock, int& status_sprite, game::control_struct& ctrl_handler, 
    Status_sprite_index sprite_index) {

    status_sprite = (status_sprite == 1) ? 0 : 1;
    ctrl_handler.sprite_status = static_cast<int>(sprite_index) + status_sprite;
    clock.restart();
}

static void render_window(game::control_struct& ctrl_handler, const game::game_state_client& global_state,
    ushort& player_count, size_t index_cli) {

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Game Shooter");
    window.setVerticalSyncEnabled(true);
    const sf::Font font("open-sans/OpenSans-Regular.ttf");

    /* ------- Hero ------ */
    game::Mob hero;
    hero.make_sprites();
    hero.set_sprite(Status_sprite_index::DOWN);
    /* ------------------- */

    sf::Clock clock;

    /* -------- view ------- */
    sf::View view (sf::Vector2f({0, 0}), sf::Vector2f({600, 400}));
    /* --------------------- */

    int move_x_old = 0;
    int move_y_old = 0;

    int move_x = 0;
    int move_y = 0;

    int move_x_plus = 0;
    int move_x_minus = 0;
    int move_y_plus = 0;
    int move_y_minus = 0;

    int status_sprite = 1;

	while (window.isOpen()) {
        /* Drawing and handling key buttons */

        while (std::optional<sf::Event> event = window.pollEvent()) {
            if (const auto* key_pressed = event->getIf<sf::Event::KeyPressed>()) {
                switch (key_pressed->code) {
                    case sf::Keyboard::Key::D:
                        if (clock.getElapsedTime().asSeconds() > 0.15) {
                            change_status_sprite(clock, status_sprite, 
                                ctrl_handler, Status_sprite_index::RIGHT);
                            ctrl_handler.changed = 1;
                        }
                        move_x_plus = 1;
                        break;

                    case sf::Keyboard::Key::A: 
                        if (clock.getElapsedTime().asSeconds() > 0.15) {
                            change_status_sprite(clock, status_sprite, 
                                ctrl_handler, Status_sprite_index::LEFT);
                            ctrl_handler.changed = 1;
                        }
                        move_x_minus = 1;
                        break;

                    case sf::Keyboard::Key::W: 
                        if (clock.getElapsedTime().asSeconds() > 0.15) {
                            change_status_sprite(clock, status_sprite, 
                                ctrl_handler, Status_sprite_index::UP);
                            ctrl_handler.changed = 1;
                        }
                        move_y_minus = 1;
                        break;

                    case sf::Keyboard::Key::S: 
                        if (clock.getElapsedTime().asSeconds() > 0.15) {
                            change_status_sprite(clock, status_sprite, 
                                ctrl_handler, Status_sprite_index::DOWN);
                            ctrl_handler.changed = 1;
                        }
                        move_y_plus = 1;
                        break;
            
                    default:
                        break;
                }
            }
            if (const auto* key_released = event->getIf<sf::Event::KeyReleased>()) {
                switch (key_released->code) {
                    case sf::Keyboard::Key::D:
                        move_x_plus = 0;
                        ctrl_handler.sprite_status = static_cast<int>(Status_sprite_index::DOWN);
                        ctrl_handler.changed = 1;
                        break;
                    case sf::Keyboard::Key::A: 
                        move_x_minus = 0;
                        ctrl_handler.sprite_status = static_cast<int>(Status_sprite_index::DOWN);
                        break;
                    case sf::Keyboard::Key::W: 
                        move_y_minus = 0;
                        ctrl_handler.sprite_status = static_cast<int>(Status_sprite_index::DOWN);
                        ctrl_handler.changed = 1;
                        break;
                    case sf::Keyboard::Key::S: 
                        move_y_plus = 0;
                        ctrl_handler.sprite_status = static_cast<int>(Status_sprite_index::DOWN);
                        ctrl_handler.changed = 1;
                        break;
                    default:
                        break;
                }
            }
            else if (event->is<sf::Event::Closed>()) {
                window.close();
                exit(0);
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                    window.close();
                    exit(0);
                }
            }
        }
        move_x = move_x_plus - move_x_minus;
        move_y = move_y_plus - move_y_minus;

        if (move_x_old != move_x){
            move_x_old = move_x;
            ctrl_handler.move_x = move_x;
            ctrl_handler.changed = 1;
        }
        if (move_y_old != move_y) {
            move_y_old = move_y;
            ctrl_handler.move_y = move_y;
            ctrl_handler.changed = 1;
        }

        /* ----- Render ----- */
        window.clear(sf::Color::Black);
        
        global_state.global_map.render(window);

        /* ------ FPS ------- */
        game::window_info fps_info("open-sans/OpenSans-Bold.ttf");
        fps_info.update_info(delta_time.asMilliseconds(), hero.health);
        fps_info.render(window, view);
        /* ------------------ */

        for (auto obj = global_state.player_objects.begin(); obj != global_state.player_objects.end(); obj++){
            std::lock_guard<std::mutex> lock(state_mutex);

            sf::RectangleShape rect{{obj->second._hitbox.width, obj->second._hitbox.height}};
            rect.setOutlineThickness(1);
            rect.setOutlineColor({255, 0, 0});
            rect.setFillColor({0,0,0});
            rect.move(obj->second.getPosition());
            window.draw(rect);

            /* ---- Your hero ---- */
            if (index_cli == obj->first) {
                hero.set_position(obj->second.getPosition());
                hero.set_rotation(obj->second.getRotation());
                hero.set_sprite(static_cast<Status_sprite_index>(obj->second.sprite_status));
                window.draw(hero.get_sprite());
                /* ---- View ---- */
                view.setCenter(obj->second.getPosition());
                window.setView(view);
            } 
            /* ---- Another hero ---- */
            else {
                game::Mob hero_2;
                hero_2.make_sprites();
                hero_2.set_position(obj->second.getPosition());
                hero_2.set_rotation(obj->second.getRotation());
                hero_2.set_sprite(static_cast<Status_sprite_index>(obj->second.sprite_status));
                window.draw(hero_2.get_sprite());
            }
        }
        window.display();
	}
}

/* --- Recieving initial information --- */
static void get_initial_data(game::game_state_client& global_state, sf::TcpSocket& server){
    sf::Packet incoming_state;

    if (server.receive(incoming_state) != sf::Socket::Status::Done) {
        std::cerr << "Error while recieving initial information" << std::endl;
    }
    else {
        global_state.global_map.make_walls(incoming_state);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "You should write the correct index of client [from 0 to N]\n";
        return -1;
    }
    sf::TcpSocket server;
    /* IP addres of server */
    sf::IpAddress server_IP(127, 0, 0, 1);
    ushort port = 53000;

    std::cout << "connecting to " << server_IP << " on port " << port << "..." << std::endl;
    sf::Socket::Status status = server.connect(server_IP, port);

    game::game_state_client global_state;
    game::control_struct ctrl_handler = {};
    ushort player_count = 0;

    if (status != sf::Socket::Status::Done) {
        std::cerr << "Error while connecting" << std::endl;
    }
    else {
        std::cout << "Conntcted to server on " << server.getRemoteAddress().value() <<
            " and port " << server.getRemotePort() << std::endl;
    }

    //load map textures
    global_state.global_map.make_textures("Animations/map/map.png");
    global_state.global_map.make_map();

    get_initial_data(global_state, server);

    /* Thread of network module */
    std::thread network_thread(network_handler, std::ref(ctrl_handler), std::ref(global_state),
        std::ref(server), std::ref(player_count));
    /* Main thread of rendering module */
    render_window(ctrl_handler, global_state, player_count, std::stoi(argv[1]));

    network_thread.join();    

	return 0;
}

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

void resolve_collision(game::AABB& player_box, const game::AABB& obstacle, int& move_x, int& move_y) {
    // Проверяем пересечение
    if (!player_box.intersects(obstacle))
        return;

    // Рассчитываем глубину проникновения по каждой оси
    float overlap_left = player_box.right() - obstacle.left;
    float overlap_right = obstacle.right() - player_box.left;
    float overlap_top = player_box.bottom() - obstacle.top;
    float overlap_bottom = obstacle.bottom() - player_box.top;

    // Находим минимальное перекрытие по каждой оси
    float min_overlap_x = std::min(overlap_left, overlap_right);
    float min_overlap_y = std::min(overlap_top, overlap_bottom);

    // Определяем направление выталкивания
    float push_x = min_overlap_x * (player_box.left < obstacle.left ? -1 : 1);
    float push_y = min_overlap_y * (player_box.top < obstacle.top ? -1 : 1);

    // Выбираем ось с минимальным перекрытием
    if (std::abs(push_x) < std::abs(push_y)) {
        move_x += push_x;
    } else {
        move_y += push_y;
    }
}

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

static void change_position_sprite(sf::Clock& clock, int& status_sprite, game::control_struct& ctrl_handler, 
    Status_sprite_index sprite_index) {

    if (status_sprite == 1) {
        status_sprite = 0;
        ctrl_handler.sprite_status = static_cast<int>(sprite_index) + status_sprite;
    }
    else {
        status_sprite = 1;
        ctrl_handler.sprite_status = static_cast<int>(sprite_index) + status_sprite;
    }
    clock.restart();
}

static void render_window(game::control_struct& ctrl_handler, const game::game_state_client& global_state,
    ushort& player_count, int index_cli) {

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Game Shooter");
    window.setVerticalSyncEnabled(true);
    const sf::Font font("open-sans/OpenSans-Regular.ttf");
    char tag_value[100] = {};

    //----------- Map -----------
    game::Map map("Animations/map/map.png");
    map.make_map();
    //---------------------------

    //----------- Hero -----------
    game::Mob hero;
    hero.make_sprites();
    hero.set_sprite(Status_sprite_index::DOWN);
    //---------------------------

    sf::Clock clock;

    //----------- view -----------
    sf::View view (sf::Vector2f({0, 0}), sf::Vector2f({600, 400}));
    //----------------------------

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
                            change_position_sprite(clock, status_sprite, 
                                ctrl_handler, Status_sprite_index::RIGHT);
                            ctrl_handler.changed = 1;
                        }
                        move_x_plus = 1;
                        break;

                    case sf::Keyboard::Key::A: 
                        if (clock.getElapsedTime().asSeconds() > 0.15) {
                            change_position_sprite(clock, status_sprite, 
                                ctrl_handler, Status_sprite_index::LEFT);
                            ctrl_handler.changed = 1;
                        }
                        move_x_minus = 1;
                        break;

                    case sf::Keyboard::Key::W: 
                        if (clock.getElapsedTime().asSeconds() > 0.15) {
                            change_position_sprite(clock, status_sprite, 
                                ctrl_handler, Status_sprite_index::UP);
                            ctrl_handler.changed = 1;
                        }
                        move_y_minus = 1;
                        break;

                    case sf::Keyboard::Key::S: 
                        if (clock.getElapsedTime().asSeconds() > 0.15) {
                            change_position_sprite(clock, status_sprite, 
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
        /* AABB collision check */
        for (auto map_bound : map.map_bounds)
            resolve_collision(hero.mob_bounds, map_bound, move_x, move_y);
            
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

        /* ------ Map ------- */
        map.render(window);
        /* ------------------ */
        
        /* ------ FPS ------- */
        game::window_info fps_info("open-sans/OpenSans-Bold.ttf");
        fps_info.update_info(delta_time.asMilliseconds(), hero.health);
        fps_info.render(window, view);
        /* ------------------ */

        for (auto obj = global_state.player_objects.begin(); obj != global_state.player_objects.end(); obj++){
            std::lock_guard<std::mutex> lock(state_mutex);
            
            hero.set_sprite(static_cast<Status_sprite_index>(obj->second.sprite_status));
            window.draw(obj->second); 

            hero.set_position(obj->second.getPosition());
            hero.set_rotation(obj->second.getRotation());
            window.draw(hero.get_sprite());

            /* ---- View ---- */
            if (index_cli == obj->first) {
                view.setCenter(obj->second.getPosition());
                window.setView(view);
            }
        }
        window.display();
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
    /* thread of network module */
    std::thread network_thread(network_handler, std::ref(ctrl_handler), std::ref(global_state),
        std::ref(server), std::ref(player_count));
    /* main thread of rendering module */
    render_window(ctrl_handler, global_state, player_count, std::stoi(argv[1]));

    network_thread.join();    

	return 0;
}

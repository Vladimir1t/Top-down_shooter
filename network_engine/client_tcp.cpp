#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#include <iostream>
#include <string>
#include <array>
#include <thread>
#include <mutex>
#include <cstdlib>

#include "game_state.hpp"

namespace {
std::mutex state_mutex;
uint64_t index_cli;
sf::Clock clock_fps;
sf::Time delta_time;
}

static void network_handler(game::control_struct& ctrl_handler, game::game_state_client& global_state,
                            sf::TcpSocket& server, ushort& player_count) {

    sf::Packet incoming_state;
    sf::Packet outcoming_data;

    sf::Socket::Status status;

    while (!global_state.game_over.load() && !global_state.terminate.load()) {
        status = server.receive(incoming_state);
        
        switch (status)
        {
        case sf::Socket::Status::Done:
            break;
        case sf::Socket::Status::Disconnected:
            std::cout << "server disconnected" << std::endl;
            global_state.terminate.store(true);
            break;
        default:
            std::cout << "unknown problem with server" << std::endl;
            global_state.terminate.store(true);
            break;
        }

        if(status == sf::Socket::Status::Done) {
            delta_time = clock_fps.restart();

            uint64_t unique_index;
            float x, y, rot;
            int sprite_status, health;
            incoming_state >> player_count;

            std::lock_guard<std::mutex> lock(state_mutex);

            for (int i = 0; i < player_count; ++i) {
                incoming_state >> unique_index >> x >> y >> rot >> sprite_status >> health;
                #ifdef DEBUG
                std::cout << "Player: " << unique_index << "\n\tx: " << x << "\n\ty: " << y << "\n\tr: " 
                          << rot << "\n\ts: " << sprite_status << std::endl;
                #endif
                game::object& obj = global_state.player_objects[unique_index];
                /* --- information about each mob --- */
                obj.setPosition({x, y});
                obj.setRotation(sf::radians(0)); //was rot
                obj.sprite_status = sprite_status;
                obj.health = health;
            }

            uint64_t proj_count = 0;
            uint64_t proj_type = 0;
            const uint64_t game_over = 0xDEAD;
            uint64_t id = 0;
            bool is_active;
            std::unordered_map<uint64_t, std::unique_ptr<game::updatable>>::iterator found_elem;
            game::projectile* tmp = 0;

            incoming_state >> proj_count;

            for (uint64_t i = 0; i < proj_count; ++i) {
                incoming_state >> proj_type;
                switch (proj_type)
                {
                case static_cast<uint64_t>(game::updatable_type::projectile_type):
                    incoming_state >> id >> unique_index >> is_active >> x >> y >> rot;
                    if (is_active) {
                        found_elem = global_state.projectiles.find(unique_index);
                        if (found_elem == global_state.projectiles.end()) {
                            #ifdef DEBUG
                            std::cout << "creating projectile with id: " << id << " and unique index: " << unique_index << std::endl;
                            #endif // DEBUG
                            global_state.projectiles[unique_index] = global_state.factory.get_projectile(x, y, rot, id);
                            // std::cout << "done." << std::endl;
                        }
                        else {
                            tmp = dynamic_cast<game::projectile*>(found_elem->second.get());
                            tmp->base_.hitbox_.x = x;
                            tmp->base_.hitbox_.y = y;
                        }
                    }
                    else {
                        #if DEBUG
                        std::cout << "deleting projectile with unique id: " << unique_index << std::endl;
                        #endif // DEBUG
                        global_state.projectiles.erase(unique_index);
                        // std::cout << "done." << std::endl;
                    }
                    break;
                case game_over:
                    {
                        global_state.game_over = true;
                        std::cout << "END\n";
                        server.disconnect();
                        break;
                    }
                default:
                    std::cout << "error: got default updatable type" << std::endl;
                    break;
                }
            }
        }


        uint64_t change_mask = 0;
        if (ctrl_handler.move_changed) change_mask |= game::packet_type::move_bit;
        if (ctrl_handler.mouse_changed) change_mask |= game::packet_type::mouse_bit;
        

        outcoming_data << change_mask;
        if (ctrl_handler.move_changed) {
            outcoming_data << ctrl_handler.move_x << ctrl_handler.move_y 
                           << ctrl_handler.rotate << ctrl_handler.sprite_status;
            ctrl_handler.move_changed = 0;
            
        }
        if (ctrl_handler.mouse_changed) {
            outcoming_data << ctrl_handler.mouse.x << ctrl_handler.mouse.y
                           << ctrl_handler.mouse.angle << index_cli;;
            ctrl_handler.mouse_changed = 0;
        }

        if (change_mask != 0){
            if (server.send(outcoming_data) != sf::Socket::Status::Done) {
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

static void render_window(game::control_struct& ctrl_handler, const game::game_state_client& global_state) {

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Game Shooter");
    window.setVerticalSyncEnabled(true);
    const sf::Font font("open-sans/OpenSans-Regular.ttf");

    /* ------- Hero ------ */
    game::Mob hero_sprite;
    hero_sprite.make_sprites();
    hero_sprite.set_sprite(Status_sprite_index::DOWN);
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

	while (window.isOpen() && !global_state.terminate.load()) {

        if (global_state.game_over.load()) {
            game::Animated_game_over game_over("- GAME OVER -");
            game_over.end_dispay(window, font, global_state);
            return;
        }
        /* Drawing and handling key buttons */

        while (std::optional<sf::Event> event = window.pollEvent()) {
            if (const auto* key_pressed = event->getIf<sf::Event::KeyPressed>()) {
                switch (key_pressed->code) {
                    case sf::Keyboard::Key::D:
                        if (clock.getElapsedTime().asSeconds() > 0.15) {
                            change_status_sprite(clock, status_sprite, 
                                ctrl_handler, Status_sprite_index::RIGHT);
                            ctrl_handler.move_changed = 1;
                        }
                        move_x_plus = 1;
                        break;

                    case sf::Keyboard::Key::A: 
                        if (clock.getElapsedTime().asSeconds() > 0.15) {
                            change_status_sprite(clock, status_sprite, 
                                ctrl_handler, Status_sprite_index::LEFT);
                            ctrl_handler.move_changed = 1;
                        }
                        move_x_minus = 1;
                        break;

                    case sf::Keyboard::Key::W: 
                        if (clock.getElapsedTime().asSeconds() > 0.15) {
                            change_status_sprite(clock, status_sprite, 
                                ctrl_handler, Status_sprite_index::UP);
                            ctrl_handler.move_changed = 1;
                        }
                        move_y_minus = 1;
                        break;

                    case sf::Keyboard::Key::S: 
                        if (clock.getElapsedTime().asSeconds() > 0.15) {
                            change_status_sprite(clock, status_sprite, 
                                ctrl_handler, Status_sprite_index::DOWN);
                            ctrl_handler.move_changed = 1;
                        }
                        move_y_plus = 1;
                        break;
            
                    default:
                        break;
                }
            }
            else if (const auto* key_released = event->getIf<sf::Event::KeyReleased>()) {
                switch (key_released->code) {
                    case sf::Keyboard::Key::D:
                        move_x_plus = 0;
                        ctrl_handler.sprite_status = static_cast<int>(Status_sprite_index::DOWN);
                        ctrl_handler.move_changed = 1;
                        break;
                    case sf::Keyboard::Key::A: 
                        move_x_minus = 0;
                        ctrl_handler.sprite_status = static_cast<int>(Status_sprite_index::DOWN);
                        break;
                    case sf::Keyboard::Key::W: 
                        move_y_minus = 0;
                        ctrl_handler.sprite_status = static_cast<int>(Status_sprite_index::DOWN);
                        ctrl_handler.move_changed = 1;
                        break;
                    case sf::Keyboard::Key::S: 
                        move_y_plus = 0;
                        ctrl_handler.sprite_status = static_cast<int>(Status_sprite_index::DOWN);
                        ctrl_handler.move_changed = 1;
                        break;
                    default:
                        break;
                }
            }
            else if (const auto* mouse_pressed = event->getIf<sf::Event::MouseButtonPressed>()){
                if (mouse_pressed->button == sf::Mouse::Button::Left) {
                    ctrl_handler.mouse_changed = true;

                    //редкостная фигня со static_cast потому что mouse_presserd->position.x это int, а window.getSize - uint
                    ctrl_handler.mouse = {0, 0, std::atan2(static_cast<long>(mouse_pressed->position.y) -
                        static_cast<long>(window.getSize().y/2), static_cast<long>(mouse_pressed->position.x) -
                        static_cast<long>(window.getSize().x/2))};
                    #if DEBUG
                        std::cout << "mouse pressed:\n x, y:" << mouse_pressed->position.x << " " << mouse_pressed->position.y
                            << "\n wind size x, y: " << window.getSize().x << " " << window.getSize().y
                            << "\n delta x, y: " << static_cast<long>(mouse_pressed->position.x) -static_cast<long>(window.getSize().x/2)
                            << " " << static_cast<long>(mouse_pressed->position.y) - static_cast<long>(window.getSize().y/2)
                            << "angle: " << (ctrl_handler.mouse.angle)*180.0*M_1_PI << std::endl;
                    #endif // DEBUG
                }
            }
            else if (event->is<sf::Event::Closed>()) {
                window.close();
                exit(0);
            }
        }
        move_x = move_x_plus - move_x_minus;
        move_y = move_y_plus - move_y_minus;

        if (move_x_old != move_x){
            move_x_old = move_x;
            ctrl_handler.move_x = move_x;
            ctrl_handler.move_changed = 1;
        }
        if (move_y_old != move_y) {
            move_y_old = move_y;
            ctrl_handler.move_y = move_y;
            ctrl_handler.move_changed = 1;
        }

        /* ----- Render ----- */
        window.clear(sf::Color::Black);
        
        global_state.global_map.render(window);

        /* ------ FPS ------- */
        game::window_info fps_info("open-sans/OpenSans-Bold.ttf");
        fps_info.update_info(delta_time.asMilliseconds(), global_state.player_objects.at(index_cli).health);
        fps_info.render(window, view);
        /* ------------------ */

        for (auto obj = global_state.player_objects.begin(); obj != global_state.player_objects.end(); obj++) {
            std::lock_guard<std::mutex> lock(state_mutex);

            #ifdef DEBUG
                sf::RectangleShape rect{{obj->second._hitbox.width, obj->second._hitbox.height}};
                rect.setOutlineThickness(1);
                rect.setOutlineColor({255, 0, 0});
                rect.setFillColor({0,0,0});
                rect.move(obj->second.getPosition());
                window.draw(rect);
            #endif // DEBUG

            /* ---- Your hero ---- */
            if (index_cli == obj->first) {
                hero_sprite.set_position(obj->second.getPosition());
                hero_sprite.set_rotation(obj->second.getRotation());
                hero_sprite.set_sprite(static_cast<Status_sprite_index>(obj->second.sprite_status));
                window.draw(hero_sprite.get_sprite());
                /* ---- View ---- */
                view.setCenter(obj->second.getPosition());
                window.setView(view);
            } 
            /* ---- Another hero ---- */
            else {
                game::Mob hero_sprite_other;
                hero_sprite_other.make_sprites();
                hero_sprite_other.set_position(obj->second.getPosition());
                hero_sprite_other.set_rotation(obj->second.getRotation());
                hero_sprite_other.set_sprite(static_cast<Status_sprite_index>(obj->second.sprite_status));
                window.draw(hero_sprite_other.get_sprite());
            }
        }
        /* ---- projectile render ---- */
        for (auto& obj: global_state.projectiles) {
            obj.second.get()->render(window);
        }
        window.display();
	}

    std::cout << "fallig from window" << std::endl;
}

/* --- Recieving initial information --- */
static void get_initial_data(game::game_state_client& global_state, sf::TcpSocket& server) {
    sf::Packet incoming_state;

    if (server.receive(incoming_state) != sf::Socket::Status::Done) {
        std::cerr << "Error while recieving initial information" << std::endl;
    }
    else {
        global_state.global_map.make_walls(incoming_state);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2 || std::stoi(argv[1]) < game::NUM_MOBS) {
        std::cerr << "You should write the correct index of client [from " << game::NUM_MOBS << " to N]\n";
        return -1;
    }
    
    game::game_state_client global_state;

    try {
        global_state.create_from_settings("data/projectile.txt");
    }
    catch(std::ios::failure& e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
    catch(sf::Exception& e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
    
    /* --- load map textures --- */
    global_state.global_map.make_textures("Animations/map/map.png");
    global_state.global_map.make_map();

    sf::TcpSocket server;
    /* --- IP addres of server --- */
    sf::IpAddress server_IP(127, 0, 0, 1);
    ushort port = 53000;

    std::cout << "connecting to " << server_IP << " on port " << port << "..." << std::endl;
    sf::Socket::Status status = server.connect(server_IP, port);

    
    game::control_struct ctrl_handler = {};
    ushort player_count = 0;

    if (status != sf::Socket::Status::Done) {
        std::cerr << "Error while connecting" << std::endl;
    }
    else {
        std::cout << "Conntcted to server on " << server.getRemoteAddress().value() <<
            " and port " << server.getRemotePort() << std::endl;
    }
    get_initial_data(global_state, server);
    /* --- Thread of network module --- */
    std::thread network_thread(network_handler, std::ref(ctrl_handler), std::ref(global_state),
        std::ref(server), std::ref(player_count));
    /* --- Main thread of rendering module --- */
    index_cli = std::stoi(argv[1]);

    render_window(ctrl_handler, global_state);

    network_thread.join();    

	return 0;
}

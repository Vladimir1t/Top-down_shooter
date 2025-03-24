#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <cstdlib>

#include "game_state.hpp"

enum class Status_sprite_index {
    UP     = 10,
    UP2    = 11,
    DOWN   = 20,
    DOWN2  = 21,
    RIGHT  = 30,
    RIGHT2 = 31,
    LEFT   = 40,
    LEFT2  = 41,
};

static std::mutex state_mutex;
static int index_cli = 0;

static void network_handler(game::control_struct& ctrl_handler, game::game_state_client& global_state,
    sf::TcpSocket& server, ushort& player_count) {

    sf::Packet incoming_state;
    sf::Packet outcoming_data;

    while (true) {
        if (server.receive(incoming_state) != sf::Socket::Status::Done) {
            std::cerr << "Error while recieving" << std::endl;
        }
        else {
            uint64_t unique_index;
            float x, y, rot;
            int sprite_status;
            incoming_state >> player_count;
            // std::cout << "rn " << player_count << " are playing" << std::endl;

            //needs better solution, but working rn
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
    char tag_value[100];

    //----------- Map -----------
    sf::Texture texture_wood1, texture_wood2, texture_stone1;
    bool success = true;
    success = success && texture_wood1.loadFromFile ("Animations/map/map_wood1.png", false, sf::IntRect({50, 50}, {540, 540}));   
    success = success && texture_wood2.loadFromFile ("Animations/map/map_wood2.png", false, sf::IntRect({50, 50}, {540, 540}));   
    success = success && texture_stone1.loadFromFile ("Animations/map/map_stone1.png", false, sf::IntRect({0, 0}, {490, 490}));  

    if(!success) std::cout << "error while opening map files\n";

    sf::Sprite map_1(texture_wood1);
    sf::Sprite map_2(texture_wood2);
    sf::Sprite map_3(texture_stone1);
    sf::Sprite map_4(texture_stone1);
    sf::Sprite map_5(texture_wood2);
    sf::Sprite map_6(texture_wood1);

    map_1.setPosition ({0, 0});
    map_2.setPosition ({490, 0});
    map_3.setPosition ({980, 0});
    map_4.setPosition ({0, 490});
    map_5.setPosition ({490, 490});
    map_6.setPosition ({980, 490});
    //---------------------------

    //----------- Hero -----------
    success = true;
    sf::Texture texture_hero_down, texture_hero_up, texture_hero_right, texture_hero_left;
    sf::Texture texture_hero_down2, texture_hero_up2, texture_hero_right2, texture_hero_left2;
	success = success && texture_hero_down.loadFromFile ("Animations/Carry_Run/Carry_Run_Down-Sheet.png", false, sf::IntRect({0, 0}, {64, 64}));   
    success = success && texture_hero_down2.loadFromFile ("Animations/Carry_Run/Carry_Run_Down-Sheet.png", false, sf::IntRect({192, 0}, {64, 64}));   
    success = success && texture_hero_up.loadFromFile ("Animations/Carry_Run/Carry_Run_Up-Sheet.png", false, sf::IntRect({0, 0}, {64, 64})); 
    success = success && texture_hero_up2.loadFromFile ("Animations/Carry_Run/Carry_Run_Up-Sheet.png", false, sf::IntRect({192, 0}, {64, 64}));
    success = success && texture_hero_right.loadFromFile ("Animations/Carry_Run/Carry_Run_Side-Sheet.png", false, sf::IntRect({0, 0}, {64, 64}));
    success = success && texture_hero_right2.loadFromFile ("Animations/Carry_Run/Carry_Run_Side-Sheet.png", false, sf::IntRect({192, 0}, {64, 64}));
    success = success && texture_hero_left.loadFromFile ("Animations/Carry_Run/Carry_Run_LSide-Sheet.png", false, sf::IntRect({0, 0}, {64, 64}));   
    success = success && texture_hero_left2.loadFromFile ("Animations/Carry_Run/Carry_Run_LSide-Sheet.png", false, sf::IntRect({192, 0}, {64, 64}));

    if(!success) std::cout << "error while opening animation files\n";


    sf::Sprite hero_down(texture_hero_down);
    sf::Sprite hero_down2(texture_hero_down2);
    sf::Sprite hero_up(texture_hero_up);
    sf::Sprite hero_up2(texture_hero_up2);
    sf::Sprite hero_right(texture_hero_right);
    sf::Sprite hero_right2(texture_hero_right2);
    sf::Sprite hero_left(texture_hero_left);
    sf::Sprite hero_left2(texture_hero_left2);

    sf::Sprite current_state_hero = std::move(hero_down);
    current_state_hero.setPosition({1, 1});
    sf::Clock clock;
    //----------------------------

    //----------- view -----------
    sf::View view (sf::Vector2f({0, 0}), sf::Vector2f({700, 550}));
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
        // drawing and handling key buttons

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

        // Render
        window.clear(sf::Color::Black);

        /* ---- Map ---- */
        window.draw(map_1);
        window.draw(map_2);
        window.draw(map_3);
        window.draw(map_4);
        window.draw(map_5);
        window.draw(map_6);

        for (auto obj = global_state.player_objects.begin(); obj != global_state.player_objects.end(); obj++){
            std::lock_guard<std::mutex> lock(state_mutex);

            switch (static_cast<Status_sprite_index>(obj->second.sprite_status)) {
                case Status_sprite_index::UP:
                    current_state_hero = std::move(hero_up);
                    break;
                case Status_sprite_index::UP2:
                    current_state_hero = std::move(hero_up2);
                    break;
                case Status_sprite_index::DOWN:
                    current_state_hero = std::move(hero_down);
                    break;
                case Status_sprite_index::DOWN2:
                    current_state_hero = std::move(hero_down2);
                    break;
                case Status_sprite_index::RIGHT:
                    current_state_hero = std::move(hero_right);
                    break;
                case Status_sprite_index::RIGHT2:
                    current_state_hero = std::move(hero_right2);
                    break;
                case Status_sprite_index::LEFT:
                    current_state_hero = std::move(hero_left);
                    break;
                case Status_sprite_index::LEFT2:
                    current_state_hero = std::move(hero_left2);
                    break;
            }
            window.draw(obj->second); 

            current_state_hero.setPosition(obj->second.getPosition());
            current_state_hero.setRotation(obj->second.getRotation());
            window.draw(current_state_hero);

            /* ---- View ---- */
            if (index_cli == obj->first) {
                view.setCenter(obj->second.getPosition());
                window.setView(view);
            }
            // std::cout << "drawn for player " << i << std::endl;
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

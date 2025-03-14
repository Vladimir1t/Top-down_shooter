#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <cstdlib>

#include "game_state.hpp"

static std::mutex state_mutex;
static int index_cli = 0;

static void network_handler(game::control_struct& ctrl_handler, game::game_state& global_state,
    sf::TcpSocket& server, ushort& player_count) {

    sf::Packet incoming_state;
    sf::Packet outcoming_data;

    while (true) {
        if (server.receive(incoming_state) != sf::Socket::Status::Done) {
            std::cerr << "Error while recieving" << std::endl;
        }
        else {
            float x, y, rot;
            incoming_state >> player_count;
            std::cout << "rn " << player_count << " are playing" << std::endl;

            //needs better solution, but working rn
            std::lock_guard<std::mutex> lock(state_mutex);
            global_state.player_objects.clear();
            for (int i = 0; i < player_count; ++i){
                global_state.player_objects.emplace_back();
            }

            for (ushort i = 0; i < player_count; ++i){
                incoming_state >> x >> y >> rot;
                std::cout << "Player " << i << ' ' << x << ' ' << y << ' ' << rot << std::endl;
                global_state.player_objects[i].setPosition({x, y});
                global_state.player_objects[i].setRotation(sf::radians(rot));
            }
        }
        outcoming_data << ctrl_handler.move << ctrl_handler.time;
        if(server.send(outcoming_data) != sf::Socket::Status::Done) {
            std::cout << "cry about it\n";
        }
        //ctrl_handler.move = 0;
        incoming_state.clear();
        outcoming_data.clear();
    }
}

static void render_window(game::control_struct& ctrl_handler, const game::game_state& global_state,
    ushort& player_count, int index_cli) {

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Game Shooter");
    window.setVerticalSyncEnabled(true);

    const sf::Font font("open-sans/OpenSans-Regular.ttf");
    char tag_value[100];

    //----------- Map -----------
    sf::Texture texture_wood1, texture_wood2, texture_stone1;
	texture_wood1.loadFromFile ("Animations/map/map_wood1.png", false, sf::IntRect({50, 50}, {540, 540}));   
    texture_wood2.loadFromFile ("Animations/map/map_wood2.png", false, sf::IntRect({50, 50}, {540, 540}));   
    texture_stone1.loadFromFile ("Animations/map/map_stone1.png", false, sf::IntRect({0, 0}, {490, 490}));  

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

    //----------- view -----------
    sf::View view (sf::Vector2f({0, 0}), sf::Vector2f({800, 600}));
    //----------------------------

    sf::Clock clock;

	while (window.isOpen()) {
        // drawing and handling key buttons
        sf::Time time_scince_last_frame = clock.restart();
        ctrl_handler.time = time_scince_last_frame.asSeconds();

        while (std::optional<sf::Event> event = window.pollEvent()) {
            if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                if (textEntered->unicode < 128) {
                    char key = static_cast<char>(textEntered->unicode);
                    switch (key) {
                        case 'w':
                            ctrl_handler.move = 1;
                            break;
                        case 's': 
                            ctrl_handler.move = 2;
                            break;
                        case 'a': 
                            ctrl_handler.move = 3;
                            break;
                        case 'd':
                            ctrl_handler.move = 4;
                            break;
                        case 'q':
                            ctrl_handler.move = 5;
                            break;
                        case 'e': 
                            ctrl_handler.move = 6;
                            break;
                        default:
                            ctrl_handler.move = 0;
                            break;
                    }
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
        // Render
        window.clear(sf::Color::Black);

        /* ---- Map ---- */
        window.draw(map_1);
        window.draw(map_2);
        window.draw(map_3);
        window.draw(map_4);
        window.draw(map_5);
        window.draw(map_6);

        for (ushort i = 0; i < player_count; ++i){
            std::lock_guard<std::mutex> lock(state_mutex);

            window.draw(global_state.player_objects[i]);
            sprintf(tag_value, "%d", i);
            sf::Text tag(font, tag_value);
            tag.setPosition(global_state.player_objects[i].getPosition());
            tag.setRotation(global_state.player_objects[i].getRotation());
            window.draw(tag);

            /* ---- View ---- */
            if (index_cli == i) {
                view.setCenter(global_state.player_objects[i].getPosition());
                window.setView(view);
            }
            // std::cout << "drawn for player " << i << std::endl;
        }
        window.display();
	}
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "You should write the index of client [from 0 to N]";
        return -1;
    }
    sf::TcpSocket server;
    /* IP addres of server */
    sf::IpAddress server_IP(127, 0, 0, 1);
    ushort port = 53000;

    std::cout << "connecting to " << server_IP << " on port " << port << "..." << std::endl;
    sf::Socket::Status status = server.connect(server_IP, port);

    game::game_state global_state;
    game::control_struct ctrl_handler;
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

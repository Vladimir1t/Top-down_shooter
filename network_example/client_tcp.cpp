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
                std::cout << "Player " << i << " " << x << y << rot << std::endl;
                global_state.player_objects[i].setPosition({x, y});
                global_state.player_objects[i].setRotation(sf::radians(rot));
            }
        }
        outcoming_data << ctrl_handler.vert << ctrl_handler.horz << ctrl_handler.rotation;
        if(server.send(outcoming_data) != sf::Socket::Status::Done) {
            std::cout << "cry about it\n";
        }
        
        ctrl_handler.horz = ctrl_handler.rotation = ctrl_handler.vert = 0;
        incoming_state.clear();
        outcoming_data.clear();
    }
}

static void render_window(game::control_struct& ctrl_handler, const game::game_state& global_state,
    ushort& player_count) {

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Game Shooter");
    window.setVerticalSyncEnabled(true);

    const sf::Font font("open-sans/OpenSans-Regular.ttf");
    char tag_value[100];

	while(window.isOpen()){
        // drawing and handling key buttons
        while (std::optional<sf::Event> event = window.pollEvent()) {
            if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                if (textEntered->unicode < 128) {
                    char key = static_cast<char>(textEntered->unicode);
                    switch (key) {
                        case 'w':
                            ctrl_handler.vert--;
                            break;
                        case 's': 
                            ctrl_handler.vert++;
                            break;
                        case 'a': 
                            ctrl_handler.horz--;
                            break;
                        case 'd':
                            ctrl_handler.horz++;
                            break;
                        case 'q':
                            ctrl_handler.rotation++;
                            break;
                        case 'e': 
                            ctrl_handler.rotation--;
                            break;
                        default:
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

        for (ushort i = 0; i < player_count; ++i){
            std::lock_guard<std::mutex> lock(state_mutex);

            window.draw(global_state.player_objects[i]);
            sprintf(tag_value, "%d", i);
            sf::Text tag(font, tag_value);
            tag.setPosition(global_state.player_objects[i].getPosition());
            tag.setRotation(global_state.player_objects[i].getRotation());
            window.draw(tag);
            // std::cout << "drawn for player " << i << std::endl;
        }
        window.display();
	}
}

int main()
{
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
    render_window(ctrl_handler, global_state, player_count);

    network_thread.join();    

	return 0;
}

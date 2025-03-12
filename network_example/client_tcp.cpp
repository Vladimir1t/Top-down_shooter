#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <string>

#include <thread>

#include "game_state.hpp"

void Send()
{
    sf::TcpSocket      server;
    // sf::IpAddress server_IP(192, 168, 50, 164);
    sf::IpAddress server_IP(0, 0, 0, 0);
    unsigned short port = 53000;

    std::cout << "connecting to " << server_IP << " on port " << port << "..." << std::endl;
    sf::Socket::Status status = server.connect(server_IP, port);

    game_state global_state;

    if (status != sf::Socket::Status::Done) {
        std::cout << "error while connecting" << std::endl;
    }
    else {
        std::cout << "conntcted to server on " << server.getRemoteAddress().value() << " and port " << server.getRemotePort() << std::endl;
    }

    sf::Packet incoming_state;
    sf::Packet outcoming_data;

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Game");
    window.setVerticalSyncEnabled(true); // call it once after creating the window

    control_struct ctrl_handler;
    unsigned short player_count = 0;

    const sf::Font font("open-sans/OpenSans-Regular.ttf");
    char tag_value[100];

	while(window.isOpen()){
        //waiting for server's state
        if(server.receive(incoming_state) != sf::Socket::Status::Done) {
            std::cout << "error while recieving" << std::endl;
        }

        else {
            float x, y, rot;
            incoming_state >> player_count;
            std::cout << "rn " << player_count << "are playing" << std::endl;

            //needs better solution, but working rn
            global_state.player_objects.clear();
            for(int i = 0; i < player_count; ++i){
                global_state.player_objects.emplace_back();
            }

            for(ushort i = 0; i < player_count; ++i){
                incoming_state >> x >> y >> rot;
                std::cout << "player " << i << " " << x << y << rot << std::endl;
                global_state.player_objects[i].setPosition({x, y});
                global_state.player_objects[i].setRotation(sf::radians(rot));
            }
        }

        //drawing and handling key buttons

        while (std::optional<sf::Event> event = window.pollEvent()) {
            if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                if (textEntered->unicode < 128) {
                    char key = static_cast<char>(textEntered->unicode);
                    switch (key)
                    {
                    case 'w': ctrl_handler.vert++;
                        break;
                    case 's': ctrl_handler.vert--;
                        break;
                    case 'a': ctrl_handler.horz--;
                        break;
                    case 'd': ctrl_handler.horz++;
                        break;
                    case 'q': ctrl_handler.rotation--;
                        break;
                    case 'e': ctrl_handler.rotation++;
                        break;
                    default:
                        break;
                    }
                }
            }
        }
        
        // Render
        window.clear(sf::Color::Black);

        for(ushort i = 0; i < player_count; ++i){
            window.draw(global_state.player_objects[i]);

            sprintf(tag_value, "%d", i);
            sf::Text tag(font, tag_value);
            tag.setPosition(global_state.player_objects[i].getPosition());
            tag.setRotation(global_state.player_objects[i].getRotation());
            window.draw(tag);
            std::cout << "drawn for player " << i << std::endl;
        }

        window.display();

        outcoming_data << ctrl_handler.horz << ctrl_handler.vert << ctrl_handler.rotation;
        if(server.send(outcoming_data) != sf::Socket::Status::Done) {
            std::cout << "cry about it\n";
        }
        
        ctrl_handler.horz = ctrl_handler.rotation = ctrl_handler.vert = 0;
        incoming_state.clear();
        outcoming_data.clear();
	}
}

int main()
{
	std::thread send(Send); 
	send.join();
	return 0;
}

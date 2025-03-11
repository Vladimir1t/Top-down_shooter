#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <string>

#include <thread>


void Send()
{
	std::array<char, 1000> incoming_message;

    sf::TcpSocket      server;
    // sf::IpAddress server_IP(192, 168, 50, 164);
    sf::IpAddress server_IP(0, 0, 0, 0);
    unsigned short port = 53000;

    std::cout << "connecting to " << server_IP << " on port " << port << "..." << std::endl;
    sf::Socket::Status status = server.connect(server_IP, port);

    if (status != sf::Socket::Status::Done) {
        std::cout << "error while connecting" << std::endl;
    }
    else {
        std::cout << "conntcted to server on " << server.getRemoteAddress().value() << " and port " << server.getRemotePort() << std::endl;

    }

    std::string incoming;
    std::string answer;
    sf::Packet message;

	while(true){
        if(server.receive(message) != sf::Socket::Status::Done) {
            std::cout << "error while recieving" << std::endl;
        }
        else {
            message >> incoming;
            std::cout << incoming << std::endl;
            message.clear();

            //trying to send answer
            answer = "was recieved: ";
            answer.append(incoming);
            message << answer;
            if(server.send(message) != sf::Socket::Status::Done) {
                std::cout << "error while sending answer" << std::endl;
            }
        }
	}
}

int main()
{
	std::thread send(Send); 
	send.join();
	return 0;
}

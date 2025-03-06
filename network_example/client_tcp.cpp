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

    sf::Socket::Status status = server.connect(server_IP, 53000);

    if (status != sf::Socket::Status::Done) {
        std::cout << "error while connecting" << std::endl;
    }
    else {
        std::cout << "conntcted to server on " << server.getRemoteAddress().value() << " and port " << server.getRemotePort() << std::endl;

    }

    std::string str;
    sf::Packet message;

	while(true){
        if(server.receive(message) != sf::Socket::Status::Done) {
            std::cout << "error while recieving" << std::endl;
        }
        else {
            message >> str;
            std::cout << str << std::endl;
            message.clear();
        }
	}
}

int main()
{
	std::thread send(Send); 
	send.join();
	return 0;
}

#include<SFML/Network.hpp>
#include<iostream>
#include<string>

#include<thread>
#include <chrono>

void Receive()
{


    sf::TcpListener listener;
    // bind the listener to a port
    if (listener.listen(53000) != sf::Socket::Status::Done) {
        std::cout << "error while starting listening to connections" << std::endl;
    }

    // accept a new connection
    sf::TcpSocket client;
    if (listener.accept(client) != sf::Socket::Status::Done)
    {
        std::cout << "error while accepting client's socket" << std::endl;
    }
    else {
        std::cout << "accepted client on" << client.getRemoteAddress().value() << " and port " << client.getRemotePort() << std::endl;
    }

    sf::Packet message;
    std::string outcoming;
    std::string incoming;

    while (true)
	{
        std::cin >> outcoming;
        message << outcoming;
        if (client.send(message) != sf::Socket::Status::Done) {
			std::cout << "error while sending to port";
		}
        message.clear();

        if(client.receive(message) != sf::Socket::Status::Done) {
            std::cout << "don't have answer. sad(" << std::endl;
        }
        else {
            message >> incoming;
            std::cout << "got answer: <" << incoming << ">" << std::endl;
        }
        
	}
}


int main()
{
	std::cout << "bebra" << std::endl;

	std::thread receive(Receive); 
	receive.join();

	return 0;
}

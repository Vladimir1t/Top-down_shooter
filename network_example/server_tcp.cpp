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
    std::string a;

    while (true)
	{
        std::cin >> a;
        message << a;
        if (client.send(message) != sf::Socket::Status::Done) {
			std::cout << "error while sending to port";
		}
        message.clear();
	}
}


int main()
{
	std::cout << "bebra" << std::endl;

	std::thread receive(Receive); 
	receive.join();

	return 0;
}

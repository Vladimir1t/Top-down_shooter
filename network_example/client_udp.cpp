#include<SFML/Network.hpp>
#include<iostream>
#include<string>
#include<thread>

using namespace std;
using namespace sf;

void Receive()
{
	sf::UdpSocket reciever_socket;
	// bind the socket to a port
	unsigned short port = 50001;

	if (reciever_socket.bind(port) != sf::Socket::Status::Done) {
		std::cout << "binding error in reciever" << std::endl;
		return;
	}

	std::cout << "reciever port is: " << reciever_socket.getLocalPort() << std::endl;

	std::array<char, 1000> data;
	std::size_t received;
	std::optional<IpAddress> sender;
	// unsigned short port;

	while (true)
	{
		if (reciever_socket.receive(data.data(), data.size(), received, sender, port) != sf::Socket::Status::Done) {
			std::cout << "error while recieving on port" << port << std::endl;
		}

		if(sender.has_value()) {
			std::cout << "Received " << received << " bytes from " << *sender << " on port " << port << std::endl;
			std::cout << data.data() << std::endl;
		}
	}
}


int main()
{
	std::cout << "bebra" << std::endl;
	thread receive(Receive); 

	receive.join();

	return 0;
}

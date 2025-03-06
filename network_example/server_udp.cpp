#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <string>

#include <thread>
#include <chrono>

using namespace std;
using namespace sf;

// udpSocket.send(packet, recipientAddress, recipientPort);
// udpSocket.receive(packet, senderAddress, senderPort);


void Send()
{
	std::string outcoming_message;
	sf::UdpSocket socket;
	// bind the socket to a por
	const unsigned short send_port = 50001; //or sf::Socket::AnyPort
	int i = 0;
	sf::IpAddress recipient(192, 168, 50, 164);

	std::array<char, 1000> incoming_message;

	while(true){
		std::cin >> outcoming_message;
		if(outcoming_message.size() != 0){
			if (socket.send(outcoming_message.data(), outcoming_message.size(), recipient, send_port) != sf::Socket::Status::Done)
			{
				std::cout << "error while sending" << std::endl;
			}
		}
	}
	outcoming_message.clear();
}



int main()
{
	thread send(Send); 
	send.join();

	return 0;
}

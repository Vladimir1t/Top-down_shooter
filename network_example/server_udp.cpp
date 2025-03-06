#include<SFML/Network.hpp>
#include<iostream>
#include<string>

#include<thread>
#include <chrono>

using namespace std;
using namespace sf;

void Send()
{
	std::array<char, 100> data;

	sf::UdpSocket socket;

	// bind the socket to a port
	const unsigned short port = 50000; //or sf::Socket::AnyPort
	int i = 0;
	sf::IpAddress recipient(10, 55, 132, 112); //!! your IP required

	while (true)
	{
		sprintf(data.data(), "iteration: %d", i++);
		if (socket.send(data.data(), data.size(), recipient, port) != sf::Socket::Status::Done)
		{
			std::cout << "error while sending" << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}


int main()
{
	thread send(Send); 
	send.join();

	return 0;
}

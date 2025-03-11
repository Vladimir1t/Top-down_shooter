#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

#include<thread>
#include <chrono>

std::vector<sf::TcpSocket> clients;
std::vector<sf::Packet> incoming_messages;
std::vector<sf::Packet> outcoming_messages;

void Receive()
{
    sf::TcpListener listener;
    // bind the listener to a port
    if (listener.listen(53000) != sf::Socket::Status::Done) {
        std::cout << "error while starting listening to connections" << std::endl;
    }

    sf::SocketSelector selector;
    selector.add(listener);

    while (true)
	{
        if(selector.wait()){ //blocks thread untill any message on any socket appears
            if(selector.isReady(listener)){
                // accept a new connection
                sf::TcpSocket client;
                if (listener.accept(client) != sf::Socket::Status::Done) { //must not block thread.
                    std::cout << "error while accepting client's socket" << std::endl;
                    return;
                }
                else {
                    std::cout << "accepted client on" << client.getRemoteAddress().value() << " and port " << client.getRemotePort() << std::endl;
                }
                selector.add(client);
                clients.push_back(std::move(client)); //safe?
                incoming_messages.emplace_back(); //adding new message packet to same index as created client
                outcoming_messages.emplace_back();
            }
            else{
                for(int i = 0; i < clients.size(); ++i){
                    if(selector.isReady(clients[i])){
                        if(clients[i].receive(incoming_messages[i]) != sf::Socket::Status::Done) {
                            std::cout << "recieving error (maybe socket not connected)" << std::endl;
                            incoming_messages[i].clear();
                        }
                    }
                }
            }
        }
        
        //readig packets
        std::string incoming;
        for(int i = 0; i < clients.size(); ++i){
            if(incoming_messages[i].getDataSize() != 0){
                incoming_messages[i] >> incoming;
                std::cout << i << ": got message <" << incoming << ">" << std::endl;
                incoming_messages[i].clear();
            }
        }

        std::string outcoming;
        std::cin >> outcoming;

        std::string msg;
        for(int i = 0; i < clients.size(); ++i){
            msg = outcoming;
            char tmp[100];
            sprintf(tmp, "%d", i);  //abomination
            msg.append(tmp);
            std::cout << msg << std::endl;
            outcoming_messages[i] << msg;
        }

        for(int i = 0; i < clients.size(); ++i){
            if (clients[i].send(outcoming_messages[i]) != sf::Socket::Status::Done) {
                std::cout << "error while sending to " << clients[i].getRemoteAddress().value() 
                                << " " << clients[i].getLocalPort() << std::endl;
            }
            outcoming_messages[i].clear();
        }
	}
}


int main()
{                
	std::cout << "bebra started" << std::endl;

	std::thread receive(Receive); 
	receive.join();

	return 0;
}

#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

#include<thread>
#include <chrono>

class TCP_server{
    private:
    sf::TcpListener _listener;
    sf::SocketSelector _selector;


    std::vector<sf::TcpSocket> _clients;
    std::vector<sf::Packet> _incoming_messages;
    std::vector<sf::Packet> _outcoming_messages;

    unsigned short _port;
    sf::Time _timeout;

    public:
    
    //port number and timeout in milliseconds (0 for infinity)
    TCP_server(unsigned short port, sf::Time timeout): _port(port), _timeout(timeout) {}; 
    
    void init(){
        if (_listener.listen(_port) != sf::Socket::Status::Done) {
            std::cout << "error while starting listening to connections" << std::endl;
            return;
        }

        // bind the listener to a port
        _selector.add(_listener);
    }

    void wait_and_handle() {
        if(_selector.wait(_timeout)){ //blocks thread untill any message on any socket appears or time goes out
            
            //handling all connections
            if(_selector.isReady(_listener)){
                // accept a new connection
                sf::TcpSocket client;
                if (_listener.accept(client) != sf::Socket::Status::Done) { //must not block thread.
                    std::cout << "error while accepting client's socket" << std::endl;
                    return;
                }
                else {
                    std::cout << "accepted client on" << client.getRemoteAddress().value() << " and port " << client.getRemotePort() << std::endl;
                }
                _selector.add(client);
                _clients.push_back(std::move(client)); //safe?
                _incoming_messages.emplace_back(); //adding new message packet to same index as created client
                _outcoming_messages.emplace_back();
            }
            
            for(int i = 0; i < _clients.size(); ++i){
                if(_selector.isReady(_clients[i])){
                    if(_clients[i].receive(_incoming_messages[i]) != sf::Socket::Status::Done) {
                        std::cout << "recieving error (maybe socket not connected)" << std::endl;
                        _incoming_messages[i].clear();
                    }
                }
            }
        }
        else {
            std::cout << "await timeout happend. Do something with this information, or don't" << std::endl; 
        }
    }

    void read_packets(){
        std::string incoming;
        for(int i = 0; i < _clients.size(); ++i){
            if(_incoming_messages[i].getDataSize() != 0){
                _incoming_messages[i] >> incoming;
                std::cout << i << ": got message <" << incoming << ">" << std::endl;
                _incoming_messages[i].clear();
            }
        }
    }

    void create_messages(){
        std::string outcoming;
        std::cin >> outcoming;
    
        std::string msg;
        for(int i = 0; i < _clients.size(); ++i){
            msg = outcoming;
            char tmp[100];
            sprintf(tmp, "%d", i);  //abomination
            msg.append(tmp);
            std::cout << msg << std::endl;
            _outcoming_messages[i] << msg;
        }
    }

    void send_packets(){
        for(int i = 0; i < _clients.size(); ++i){
            if (_clients[i].send(_outcoming_messages[i]) != sf::Socket::Status::Done) {
                std::cout << "error while sending to " << _clients[i].getRemoteAddress().value() 
                                << " " << _clients[i].getLocalPort() << std::endl;
            }
        }
    }

    void clear_outcome(){
        for(int i = 0; i < _outcoming_messages.size(); ++i){
            _outcoming_messages[i].clear();
        }
    }
};



void Receive() {
    sf::Time timeout = sf::milliseconds(0);

    TCP_server server{53000, timeout};
    server.init();

    while (true) {
        server.wait_and_handle();
        server.read_packets();
        server.create_messages();
        server.send_packets();

        server.clear_outcome();
	}
}


int main() {
	std::cout << "bebra started" << std::endl;

	std::thread receive(Receive); 
	receive.join();

	return 0;
}

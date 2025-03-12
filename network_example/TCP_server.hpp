#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <string>

#pragma once

#include "game_state.hpp"

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
    
    void init() {
        if (_listener.listen(_port) != sf::Socket::Status::Done) {
            std::cout << "error while starting listening to connections" << std::endl;
            return;
        }

        // bind the listener to a port
        _selector.add(_listener);
    }

    void wait_and_handle(game_state& global_state) {
        if (_selector.wait(_timeout)){ //blocks thread untill any message on any socket appears or time goes out
            //handling all connections
            if (_selector.isReady(_listener)){
                // accept a new connection
                sf::TcpSocket client;
                if (_listener.accept(client) != sf::Socket::Status::Done) { //must not block thread.
                    std::cout << "error while accepting client's socket" << std::endl;
                    return;
                }
                else {
                    std::cout << "accepted client on " << client.getRemoteAddress().value() <<
                        " and port " << client.getRemotePort() << std::endl;
                    _selector.add(client);
                    _clients.push_back(std::move(client)); //safe?
                    _incoming_messages.emplace_back(); //adding new message packet to same index as created client
                    _outcoming_messages.emplace_back();
                    global_state.player_objects.emplace_back();

                    global_state.player_objects.back().set_velocity({1.0, 1.0});
                }
            }

            for (int i = 0; i < _clients.size(); ++i) {
                if (_selector.isReady(_clients[i])){
                    if (_clients[i].receive(_incoming_messages[i]) != sf::Socket::Status::Done) {
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

    void read_packets(game_state& global_state) {
        short vert, horz, rot = 0;
        for (int i = 0; i < _clients.size(); ++i){
            if (_incoming_messages[i].getDataSize() != 0){
                _incoming_messages[i] >> vert >> horz >> rot;
                std::cout << i << ": got message: vert: " << vert << " horz: " << horz << " rot: " << rot << std::endl;
                global_state.player_objects[i].update({vert, horz, rot});
                _incoming_messages[i].clear();
            }
        }
    }

    void create_messages(game_state& global_state) {
        unsigned short client_count = _clients.size();
        std::cout << "client count" << client_count << std::endl; 

        for (unsigned short i = 0; i < client_count; ++i){
            _outcoming_messages[i] << client_count;
            for (unsigned short j = 0; j < client_count; ++j){
                _outcoming_messages[i] << global_state.player_objects[j].getPosition().x
                                       << global_state.player_objects[j].getPosition().y
                                       << global_state.player_objects[j].getRotation().asRadians();
            }
        }
    }

    void send_packets() {
        for (int i = 0; i < _clients.size(); ++i){
            if (_clients[i].send(_outcoming_messages[i]) != sf::Socket::Status::Done) {
                std::cout << "error while sending to " << _clients[i].getRemoteAddress().value() 
                                << " " << _clients[i].getLocalPort() << std::endl;
            }
        }
    }

    void clear_outcome() {
        for (int i = 0; i < _outcoming_messages.size(); ++i){
            _outcoming_messages[i].clear();
        }
    }
};

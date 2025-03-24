#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <string>
#include <list>

#pragma once

#include "game_state.hpp"

namespace game {

class TCP_server final {
    
private:
    sf::TcpListener _listener;
    sf::SocketSelector _selector;

    std::list<sf::TcpSocket> _clients;
    std::vector<sf::Packet> _incoming_messages;
    std::vector<sf::Packet> _outcoming_messages;

    ushort _port;
    sf::Time _timeout;

public:
    //port number and timeout in milliseconds (0 for infinity)
    TCP_server(ushort port, sf::Time timeout): _port(port), _timeout(timeout) {}; 
    
    int init() {
        if (_listener.listen(_port) != sf::Socket::Status::Done) {
            std::cerr << "Error while starting listening to connections" << std::endl;
            return -1;
        }

        // bind the listener to a port
        _selector.add(_listener);
        return 0;
    }

    void wait_and_handle(game_state_server& global_state) {
        if (_selector.wait(_timeout)) { 
            //handling all connections
            if (_selector.isReady(_listener)){
                // accept a new connection
                sf::TcpSocket client;
                //_clients.push_back(sf::TcpSocket()); 
                if (_listener.accept(client) != sf::Socket::Status::Done) { //must not block thread.
                    std::cerr << "Error while accepting client's socket" << std::endl;
                    return;
                }
                else {
                    std::cerr << "Accepted client on " << (client).getRemoteAddress().value() <<
                        " and port " << (client).getRemotePort() << std::endl;
                    _selector.add(client);
                    _clients.push_back(std::move(client)); 
                    _incoming_messages.emplace_back(); //adding new message packet to same index as created client
                    _outcoming_messages.emplace_back();

                    global_state.add_player();
                }
            }
            int i = 0;
            for (auto& client: _clients) {
                if (_selector.isReady(client)){
                    if (client.receive(_incoming_messages[i]) != sf::Socket::Status::Done) {
                        std::cout << "recieving error (maybe socket not connected)" << std::endl;
                        _incoming_messages[i].clear();
                    }
                }
                ++i;
            }
        }
        else {
            // std::cerr << "await timeout happend. Do something with this information or don't" << std::endl; 
        }
    }

    void read_packets(game_state_server& global_state) {
        int move_x, move_y, rotate, sprite_status;
        for (int i = 0; i < _clients.size(); ++i) {
            if (_incoming_messages[i].getDataSize() != 0) {
                _incoming_messages[i] >> move_x >> move_y >> rotate >> sprite_status;

                std::cout << i << ": got message: x: " << move_x << " y: "
                     << move_y << " rot: " << rotate << " sprite_status: " << sprite_status << std::endl;
                global_state.player_objects[i].second.set_velocity_and_rot({move_x, move_y}, rotate);
                global_state.player_objects[i].second.sprite_status = sprite_status;
                _incoming_messages[i].clear();
            }
        }
    }

    void update_state(game_state_server& global_state){
        global_state.update_state();
    }

    void create_messages(game_state_server& global_state) {
        ushort client_count = _clients.size();
        // std::cout << "client count " << client_count << std::endl; 
        object* obj;
        uint64_t id;
        for (int i = 0; i < client_count; ++i) {
            _outcoming_messages[i] << client_count;
            for (int j = 0; j < client_count; ++j) {
                obj = &(global_state.player_objects[j].second);
                id = global_state.player_objects[j].first;
                _outcoming_messages[i] << id
                                       << obj->getPosition().x
                                       << obj->getPosition().y
                                       << obj->getRotation().asRadians()
                                       << obj->sprite_status;
                
                // std::cout << "player № " << i 
                // << "\n\tid:" << id
                // << "\n\tx:" << obj->getPosition().x
                // << "\n\ty:" << obj->getPosition().y
                // << "\n\tr:" << obj->getRotation().asRadians()
                // << "\n\ts:" << obj->sprite_status << std::endl;
            }
        }
    }

    void send_packets() {
        int i = 0;
        for (auto&& it_client = _clients.begin(); it_client != _clients.end(); ++it_client, ++i) {
            try {
                if (it_client->send(_outcoming_messages[i]) != sf::Socket::Status::Done) {
                    std::cerr << "Error while sending to " << it_client->getRemoteAddress().value() 
                        << " " << it_client->getLocalPort() << std::endl;
                }
            }
            catch (std::bad_optional_access& e) {
                std::cerr << e.what();
                break;
            }
        }
    }

    void clear_outcome() {
        for (int i = 0; i < _outcoming_messages.size(); ++i) {
            _outcoming_messages[i].clear();
        }
    }
};
}

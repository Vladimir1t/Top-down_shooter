#pragma once
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <string>
#include <list>

#include "game_state.hpp"

namespace game {

void resolve_collision(game::AABB& player_box, const game::AABB& obstacle, float& move_x, float& move_y) {

    if (!player_box.intersects(obstacle)){
        // std::cout << "player " << player_box.center().x << ' ' << player_box.center().y << '\n' << "does not intersect with wall "
        //             << obstacle.x << ' ' << obstacle.y << ' ' << obstacle.width << ' ' << obstacle.height << '\n';
        return;
    }

    float overlap_x = player_box.right() - obstacle.x;
    float overlap_right = obstacle.right() - player_box.x;
    float overlap_y = player_box.bottom() - obstacle.y;
    float overlap_bottom = obstacle.bottom() - player_box.y;

    float min_overlap_x = std::min(overlap_x, overlap_right);
    float min_overlap_y = std::min(overlap_y, overlap_bottom);

    float push_x = min_overlap_x * (player_box.x < obstacle.x ? -1 : 1);
    float push_y = min_overlap_y * (player_box.y < obstacle.y ? -1 : 1);

    if (std::abs(push_x) < std::abs(push_y)) {
        move_x = push_x;
    } 
    else {
        move_y = push_y;
    }
}

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
            // handling all connections
            if (_selector.isReady(_listener)){
                // accept a new connection
                sf::TcpSocket client;
                //_clients.push_back(sf::TcpSocket()); 
                if (_listener.accept(client) != sf::Socket::Status::Done) { 
                    std::cerr << "Error while accepting client's socket" << std::endl;
                    return;
                }
                else {
                    std::cerr << "Accepted client on " << (client).getRemoteAddress().value() <<
                        " and port " << (client).getRemotePort() << std::endl;
                    _selector.add(client);
                    _clients.push_back(std::move(client)); 
                    _incoming_messages.emplace_back(); // adding new message packet to same index as created client
                    _outcoming_messages.emplace_back();

                    //creating starting message for each client
                    _outcoming_messages.back() << static_cast<uint32_t>(global_state.walls.size());
                    for(auto& w: global_state.walls){
                        _outcoming_messages.back() << w.id_ << w.x << w.y << w.width << w.height;
                    }

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
        game::mouse_input mouse;
        size_t incoming_packet_type;

        for (size_t i = 0; i < _clients.size(); ++i) {
            if (_incoming_messages[i].getDataSize() != 0) {

                _incoming_messages[i] >> incoming_packet_type;

                if(incoming_packet_type & game::packet_type::move_bit){
                    _incoming_messages[i] >> move_x >> move_y >> rotate >> sprite_status;

                    #if DEBUG
                    std::cout << i << ": got move message: x: " << move_x << " y: "
                         << move_y << " rot: " << rotate << " sprite_status: " << sprite_status << std::endl;
                    #endif //DEBUG
                    global_state.player_objects[i].second.set_internal_velocity_and_rot({move_x, move_y}, rotate);
                    global_state.player_objects[i].second.sprite_status = sprite_status;
                }
                if(incoming_packet_type & game::packet_type::mouse_bit){
                    _incoming_messages[i] >> mouse.x >> mouse.y >> mouse.angle;
                    #if DEBUG
                    std::cout << i << ": got mouse message: x: " << mouse.x << " y: " << mouse.y << " angle: " << mouse.angle << std::endl;
                    #endif //DEBUG
                    global_state.create_projectile(global_state.player_objects[i].second._hitbox.x, global_state.player_objects[i].second._hitbox.y , mouse.angle);
                }

                _incoming_messages[i].clear();
            }
        }
    }

    void check_collisions(game_state_server& global_state){
        for(auto& [index, player]: global_state.player_objects){
            player._velocity_external = {0, 0};
            for(auto &wall: global_state.walls){
                resolve_collision(player._hitbox, wall, player._velocity_external.x, player._velocity_external.y);
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
            //writing players
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

            //writing projectiles
            const projectile* tmp = 0;
            _outcoming_messages[i] << global_state.objects.size();
            for(auto& x: global_state.objects){
                switch (x.get()->get_type())
                {
                case projectile_type:
                    _outcoming_messages[i] << static_cast<size_t>(projectile_type); //size_t
                    tmp = dynamic_cast<projectile*> (x.get());
                    std::cout << "writing projectile with id: " << tmp->id_ << " and unique index: " << tmp->unique_index << std::endl;
                    _outcoming_messages[i] << tmp->id_ << tmp->unique_index << tmp->active_ << tmp->base_.hitbox_.x << tmp->base_.hitbox_.y << tmp->base_.velocity_.angle().asRadians();
                    break;
                
                default:
                    _outcoming_messages[i] << other_type;
                    break;
                }
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
        for (size_t i = 0; i < _outcoming_messages.size(); ++i) {
            _outcoming_messages[i].clear();
        }
    }
};
}

#pragma once
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <mutex>

#include "game_state.hpp"

namespace game {

bool resolve_collision(game::AABB<float>& player_box, const game::AABB<float>& obstacle, float& move_x, float& move_y) {

    if (!player_box.intersects(obstacle)) {
        #ifdef DEBUG
            std::cout << "player " << player_box.center().x << ' ' << player_box.center().y << '\n' << "does not intersect with wall "
                      << obstacle.x << ' ' << obstacle.y << ' ' << obstacle.width << ' ' << obstacle.height << '\n';
        #endif
        return false;
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
    return true;
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
    // port number and timeout in milliseconds (0 for infinity)
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

    void wait_and_handle(game_state_server& global_state, std::mutex& state_mutex) {
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

                    std::lock_guard<std::mutex> lock(state_mutex);
                    // creating starting message for each client
                    _outcoming_messages.back() << static_cast<uint64_t>(global_state.walls.size());
                    for (auto& w: global_state.walls){
                        _outcoming_messages.back() << w.id_ << w.x << w.y << w.width << w.height;
                    }
                    global_state.add_player({300, 300});
                }
            }
            for (int i = 0; auto&& client: _clients) {
                if (_selector.isReady(client)){
                    sf::Socket::Status status = client.receive(_incoming_messages[i]);
                    switch (status)
                    {
                    case sf::Socket::Status::Done:
                        break;
                    case sf::Socket::Status::Disconnected:
                        std::cout << "Client " << i << " disconnected" << std::endl;
                        break;                    
                    default:
                        break;
                    }
                }
                ++i;
            }
        }
        else {
            // std::cerr << "await timeout happend. Do something with this information or don't" << std::endl; 
        }
    }

    void read_packets(game_state_server& global_state, std::mutex& state_mutex) {
        int move_x, move_y, rotate, sprite_status;
        uint64_t incoming_packet_type;
        for (uint64_t i = 0; i < _clients.size(); ++i) {
            if (_incoming_messages[i].getDataSize() != 0) {

                _incoming_messages[i] >> incoming_packet_type;

                if (incoming_packet_type & game::packet_type::move_bit) {
                    _incoming_messages[i] >> move_x >> move_y >> rotate >> sprite_status;

                    #ifdef DEBUG
                        std::cout << i << ": got move message: x: " << move_x << " y: "
                             << move_y << " rot: " << rotate << " sprite_status: " << sprite_status << std::endl;
                    #endif // DEBUG
                    std::lock_guard<std::mutex> lock(state_mutex);
                    global_state.player_objects[i].second.set_internal_velocity_and_rot({move_x, move_y}, 0);
                    global_state.player_objects[i].second.sprite_status = sprite_status;
                }
                if (incoming_packet_type & game::packet_type::mouse_bit) {
                    game::mouse_input mouse;
                    uint64_t id = 0;
                    _incoming_messages[i] >> mouse.x >> mouse.y >> mouse.angle >> id;
                    #ifdef DEBUG
                        std::cout << i << ": got mouse message: x: " << mouse.x << " y: "
                                << mouse.y << " angle: " << mouse.angle << " id: " << id << std::endl;
                    #endif // DEBUG
                    std::lock_guard<std::mutex> lock(state_mutex);
                    global_state.create_projectile(global_state.player_objects[i].second.get_center_x(),
                                                   global_state.player_objects[i].second.get_center_y(),
                                                   mouse.angle, id);
                }
                _incoming_messages[i].clear();
            }
        }
    }

    void check_collisions_players(game_state_server& global_state,
                                  std::vector<std::pair<uint64_t, object>>& player_objects, 
                                  std::mutex& state_mutex) {

        for (auto&& [index, player]: player_objects) {
            player._velocity_external = {0, 0};
            for (auto &wall: global_state.walls) {
                resolve_collision(player._hitbox, wall, player._velocity_external.x, player._velocity_external.y);
            }
            for (auto& st_obj: global_state.objects) {
                projectile* obj;
                obj = dynamic_cast<projectile*>(st_obj.get());
                /* --- hero's own bullets --- */
                if (index == obj->id_)
                    continue;

                if (resolve_collision(player._hitbox, obj->base_.hitbox_,
                                      player._velocity_external.x,
                                      player._velocity_external.y)) {
                    obj->frame_counter_ = obj->max_frames_;
                    player.health -= obj->damage_;
                }
            }
            for (auto&& [index2, player2]: player_objects) {
                if (index != index2) {
                    resolve_collision(player._hitbox, player2._hitbox, player._velocity_external.x,
                                      player._velocity_external.y);
                }
            }
        }
    }

    void check_collisions(game_state_server& global_state, std::mutex& state_mutex) {
        std::lock_guard<std::mutex> lock(state_mutex);

        check_collisions_players(global_state, global_state.player_objects, state_mutex);
        check_collisions_players(global_state, global_state.player_objects_mobs, state_mutex);

        for (auto&& [index2, mob]: global_state.player_objects_mobs) {
            for (auto&& [index, player]: global_state.player_objects) {
                resolve_collision(player._hitbox, mob._hitbox, player._velocity_external.x,
                                  player._velocity_external.y);
                if (player.health <= 0) {
                    /* --- game over --- */
                    create_messages(global_state, state_mutex, true);
                    return;
                }
            }
        }
        for (auto&& st_obj: global_state.objects) {
            projectile* obj;
            obj = dynamic_cast<projectile*>(st_obj.get());
            for (auto &wall: global_state.walls) {
                /* --- collision walls with bullets --- */
                if (wall.intersects(obj->base_.hitbox_)) {
                    obj->frame_counter_ = obj->max_frames_;
                }
            }
        }
    }

    void update_state(game_state_server& global_state, std::mutex& state_mutex) {
        std::lock_guard<std::mutex> lock(state_mutex);
        global_state.update_state();
    }

    void create_messages(game_state_server& global_state, std::mutex& state_mutex, int game_over = false) {
    
        ushort client_count = _clients.size();
        ushort players_count = client_count + global_state.player_objects_mobs.size();
        object* obj;
        uint64_t id;
        for (int i = 0; i < client_count; ++i) {
            //std::lock_guard<std::mutex> lock(state_mutex);
            // writing players
            _outcoming_messages[i] << players_count;
            for (int j = 0; j < client_count; ++j) {
                obj = &(global_state.player_objects[j].second);
                id = global_state.player_objects[j].first;
                _outcoming_messages[i] << id
                                       << obj->getPosition().x
                                       << obj->getPosition().y
                                       << obj->getRotation().asRadians()
                                       << obj->sprite_status
                                       << obj->health;
                
                #ifdef DEBUG
                    std::cout << "player № " << j 
                    << "\n\tid:"<< id
                    << "\n\tx:" << obj->getPosition().x
                    << "\n\ty:" << obj->getPosition().y
                    << "\n\tr:" << obj->getRotation().asRadians()
                    << "\n\ts:" << obj->sprite_status << std::endl;
                #endif
            }
            /* --- for mobs --- */
            for (int j = client_count; j < players_count; ++j) {
                obj = &(global_state.player_objects_mobs[j - client_count].second);
                id = global_state.player_objects_mobs[j - client_count].first;
                _outcoming_messages[i] << id
                                       << obj->getPosition().x
                                       << obj->getPosition().y
                                       << obj->getRotation().asRadians()
                                       << obj->sprite_status
                                       << obj->health;
                #ifdef DEBUG
                std::cout << "mob № " << j
                << "\n\tid:"<< id
                << "\n\tx:" << obj->getPosition().x
                << "\n\ty:" << obj->getPosition().y
                << "\n\tr:" << obj->getRotation().asRadians()
                << "\n\ts:" << obj->sprite_status << std::endl;
                #endif //DEBUG
            }
            // writing projectiles
            const projectile* tmp = 0;
            _outcoming_messages[i] << global_state.objects.size();
            /* -- game over --- */
            if (game_over == true) {
                const uint64_t flag_game_over = 0xDEAD;
                _outcoming_messages[i] << flag_game_over;
                continue;
            }
            for (auto&& x: global_state.objects) {
                switch (x.get()->get_type())
                {
                case projectile_type:
                    _outcoming_messages[i] << static_cast<uint64_t>(projectile_type);
                    tmp = dynamic_cast<projectile*> (x.get());
                    #ifdef DEBUG
                    std::cout << "writing projectile with id: " << tmp->id_ << " and unique index: " << tmp->unique_index << std::endl;
                    #endif //DEBUG
                    _outcoming_messages[i] << tmp->id_ << tmp->unique_index << tmp->active_ << tmp->base_.hitbox_.x 
                                           << tmp->base_.hitbox_.y << tmp->base_.velocity_.angle().asRadians();
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
                    std::optional<sf::IpAddress> addr = it_client->getRemoteAddress();
                    std::cerr << "Error while sending to " << static_cast<std::string>(addr.has_value()?(addr.value().toString()):("disconnected"))
                        << " " << it_client->getLocalPort() << std::endl;
                }
            }
            catch (std::bad_optional_access& e) {
                std::cerr << e.what() << std::endl;
                std::cout << "aborting..." << std::endl;
                abort();
            }
        }
    }

    void clear_outcome() {
        for (uint64_t i = 0; i < _outcoming_messages.size(); ++i) {
            _outcoming_messages[i].clear();
        }
    }
};
}

#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>

#include "TCP_server.hpp"

namespace {
std::mutex state_mutex;
const int PORT = 53000;
}

int Receive(game::game_state_server& global_state) {

    try {
        global_state.create_from_settings("data/projectile.txt");
    }
    catch(std::ios::failure& e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
    catch(sf::Exception& e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    sf::Time timeout = sf::milliseconds(15);
    /* Set port of tcp server */
    game::TCP_server server{PORT, timeout};
    if (server.init() == -1) {
        return -1;
    }

    while (true) {
        server.wait_and_handle(global_state, state_mutex);
        // std::cout << "waiting done" << std::endl;

        server.read_packets(global_state, state_mutex);
        // std::cout << "packets read" << std::endl;

        server.check_collisions(global_state, state_mutex);

        server.update_state(global_state, state_mutex);
        // std::cout << "state updated" << std::endl;

        server.create_messages(global_state, state_mutex);
        // std::cout << "messages created" << std::endl;

        server.send_packets();
        // std::cout << "packets sent" << std::endl;

        server.clear_outcome();
        // std::cout << "outcome cleared" << std::endl;
	}
}

/** Thread for handling mobs */
int Run_mobs(game::game_state_server& global_state) {

    for (float i = 0; i < game::NUM_MOBS; ++i) {
        std::lock_guard<std::mutex> lock(state_mutex);
        global_state.add_mob({10 + i * 10, 10 + i * 10}); 
    }

    sf::Clock clock_shoot;
    sf::Clock clock_move;
    const float shoot_interval = 1.0f;  
    const float move_interval = 0.04f;  

    while (true) {
        if (clock_move.getElapsedTime().asSeconds() > move_interval) {
            for (auto&& mob : global_state.player_objects_mobs) {
                std::lock_guard<std::mutex> lock(state_mutex);
                if (mob.second.health > 0)
                    global_state.move_mob(mob);
            }
            clock_move.restart();
        }

        if (clock_shoot.getElapsedTime().asSeconds() > shoot_interval) {
            for (auto&& mob : global_state.player_objects_mobs) {
                std::lock_guard<std::mutex> lock(state_mutex);
                if (mob.second.health > 0)
                    global_state.shoot_mob(mob);
            }
            clock_shoot.restart();
        }
        sf::sleep(sf::milliseconds(10));
    }
}

int main() {
	std::cout << "Server started" << std::endl;
    game::game_state_server global_state;

    std::thread run_mobs(Run_mobs, std::ref(global_state));
	std::thread receive(Receive, std::ref(global_state)); 
    run_mobs.join();
	receive.join();

	return 0;
}

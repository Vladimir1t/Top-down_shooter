#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "TCP_server.hpp"

int Receive() {

    game::game_state_server global_state;

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
    game::TCP_server server{53000, timeout};
    if (server.init() == -1) {
        return -1;
    }

    while (true) {
        server.wait_and_handle(global_state);
        // std::cout << "waiting done" << std::endl;

        server.read_packets(global_state);
        // std::cout << "packets read" << std::endl;

        server.check_collisions(global_state);

        server.update_state(global_state);
        // std::cout << "state updated" << std::endl;

        server.create_messages(global_state);
        // std::cout << "messages created" << std::endl;

        server.send_packets();
        // std::cout << "packets sent" << std::endl;

        server.clear_outcome();
        // std::cout << "outcome cleared" << std::endl;
	}
}

int main() {
	std::cout << "Server started" << std::endl;

	std::thread receive(Receive); 
	receive.join();

	return 0;
}

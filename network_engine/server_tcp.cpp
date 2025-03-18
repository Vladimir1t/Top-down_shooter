#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "TCP_server.hpp"

void Receive() {
    sf::Time timeout = sf::milliseconds(15);
    /* Set port of tcp server */
    game::TCP_server server{53000, timeout};
    server.init();

    game::game_state global_state;

    while (true) {
        server.wait_and_handle(global_state);
        // std::cout << "waiting done" << std::endl;

        server.read_packets(global_state);
        // std::cout << "packets read" << std::endl;

        server.update_state(global_state);

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

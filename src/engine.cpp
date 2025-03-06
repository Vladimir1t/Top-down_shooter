#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <SFML/Network.hpp>
#include <cmath>
#include <iostream>
#include <chrono>
#include <thread>

void move_enemy(sf::CircleShape& my, sf::Sprite& enemy) {
    sf::Vector2f my_coord = my.getPosition();
    sf::Vector2f enemy_coord = enemy.getPosition();
    if (std::fabs(my_coord.x - enemy_coord.x) > 1) {
        if (my_coord.x < enemy_coord.x) {
            enemy_coord.x -= 0.2f;
        }
        else {
            enemy_coord.x += 0.2f;
        }
    }
    if (std::fabs(my_coord.y - enemy_coord.y) > 1) {
        if (my_coord.y < enemy_coord.y) {
            enemy_coord.y -= 0.2f;
        }
        else {
            enemy_coord.y += 0.2f;
        }
    }
    enemy.setPosition(enemy_coord);
    std::cout << "move\n";
}

int main() {
    //-------------------------------------------------

    sf::CircleShape circle(20.0f);
    // circle.setOrigin(circle.getLocalBounds().width / 2, circle.getLocalBounds().height / 2);

    // circle.setOrigin(sf::Vector2f(circle.getLocalBounds().size.x/2, circle.getLocalBounds().size.y/2));

    std::cout << circle.getLocalBounds().size.x << circle.getLocalBounds().size.y << std::endl;

    float width = 800 / 2, height = 600 / 2;
    circle.setPosition({width, height});
    circle.setFillColor(sf::Color::Yellow);
    circle.setOutlineColor(sf::Color::Black);
    circle.setOutlineThickness(5.f);
    //-------------------------------------------------
    sf::Texture texture1;
	texture1.loadFromFile ("pictures/1.png");
    sf::Sprite enemy(texture1);
    enemy.setPosition({200, 200});
    //-------------------------------------------------
    //sf::View view(sf::FloatRect({width, height}, {500, 400}));
    //-------------------------------------------------
    sf::Texture texture2;
	texture2.loadFromFile ("pictures/table.png");
    sf::Sprite s(texture2);
	s.setPosition ({0, 0});
    //-------------------------------------------------

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Game");
    window.setVerticalSyncEnabled(true); // call it once after creating the window

    // run the program as long as the window is open
    while (window.isOpen()) {
        while (std::optional<sf::Event> event = window.pollEvent()) {

            if (const auto& resized = event->getIf<sf::Event::Resized>()) {
                std::cout << "new width: " << resized->size.x << std::endl;
                std::cout << "new height: " << resized->size.y << std::endl;
            }
            else if (event->is<sf::Event::FocusLost>())
                //myGame.pause();
                std::cout << "pause\n";

            else if (event->is<sf::Event::FocusGained>())
                //myGame.resume();
                std::cout << "resume\n";
            else if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            else if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                if (textEntered->unicode < 128)
                    std::cout << "ASCII character typed: " << 
                        static_cast<char>(textEntered->unicode) << std::endl;
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
                std::cout << "move left\n";
                width -= 5;
                circle.move({-5, 0});
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                std::cout << "move right\n";
                width += 5;
                circle.setPosition({width, height});
                circle.move({5, 0});
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
                std::cout << "move up\n";
                height -= 5;
                // circle.setPosition({width, height});
                circle.move({0, -5});
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
                std::cout << "move down\n";
                height += 5;
                // circle.setPosition({width, height});
                circle.move({0, 5});
            }
            else if (const auto& mouseWheelScrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
                switch (mouseWheelScrolled->wheel) {
                    case sf::Mouse::Wheel::Vertical:
                        std::cout << "wheel type: vertical" << std::endl;
                        break;
                    case sf::Mouse::Wheel::Horizontal:
                        std::cout << "wheel type: horizontal" << std::endl;
                        break;
                }
                std::cout << "wheel movement: " << mouseWheelScrolled->delta << std::endl;
                std::cout << "mouse x: " << mouseWheelScrolled->position.x << std::endl;
                std::cout << "mouse y: " << mouseWheelScrolled->position.y << std::endl;
            }
            else if (const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseButtonPressed->button == sf::Mouse::Button::Right) {

                    std::cout << "the right button was pressed" << std::endl;
                    std::cout << "mouse x: " << mouseButtonPressed->position.x << std::endl;
                    std::cout << "mouse y: " << mouseButtonPressed->position.y << std::endl;
                }
            }
        }

        if (circle.getPosition() == enemy.getPosition()) {
            window.clear(sf::Color::Red);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            window.close();
        }
        // Render
        window.clear(sf::Color::Black);

        window.draw ( s );
        window.draw(circle);
        window.draw(enemy);
        window.display();

        move_enemy(circle, enemy);
    }
}
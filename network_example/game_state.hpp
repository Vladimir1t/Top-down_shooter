#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace game {

class control_struct final {
public:
    short vert = 0;
    short horz = 0;
    short rotation = 0;
};

class object: public sf::RectangleShape{
    sf::Vector2f velocity;

public:
    void update(control_struct ctrl){
        this->move({velocity.x * ctrl.horz, velocity.y*ctrl.vert});
        this->rotate(sf::radians(0.01*ctrl.rotation));
    }

    void set_velocity(sf::Vector2f new_velocity){
        velocity = new_velocity;
    }
};

class game_state final {
public:
    std::vector<object> player_objects;
};
}
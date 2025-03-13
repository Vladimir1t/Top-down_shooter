#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace game {

class control_struct final {
public:
    float vert = 0;
    float horz = 0;
    float rotation = 0;
};

class object: public sf::RectangleShape{
    sf::Vector2f velocity;

public:
    void update(control_struct ctrl) {
        move({velocity.x * ctrl.horz, velocity.y*ctrl.vert});
        rotate(sf::radians(0.01*ctrl.rotation));
    }

    void set_velocity(sf::Vector2f new_velocity){
        velocity = new_velocity;
    }
};

class game_state final {
public:
    std::vector<object> player_objects;
};

class Mob {
public:
    size_t index = 0;
    typename sf::Vector2f coords = {0, 0};
    Mob(sf::Vector2f coords) : coords(coords) {};
    Mob() = default;

    // add sprite
};

}
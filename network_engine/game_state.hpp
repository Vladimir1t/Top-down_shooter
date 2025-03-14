#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace game {

class control_struct final {
public:
    int move;
    float time;
};

class object: public sf::RectangleShape{
    sf::Vector2f velocity;

public:
    void update(float horz, float vert, float rot) {
        //std::cout << velocity.x << ' ' << velocity.y << '\n'; 
        move({velocity.x * horz, velocity.y * vert});
        rotate(sf::radians(0.01 * rot));
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
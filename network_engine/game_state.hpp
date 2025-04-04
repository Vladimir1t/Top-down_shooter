#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace game {

class control_struct final {
public:
    int move_x;
    int move_y;
    int rotate;
    bool changed;
    int sprite_status;
};

class object: public sf::RectangleShape{
    sf::Vector2f _velocity_coeff;
    float _rotation_coeff;

    sf::Vector2i _velocity;
    int _rotation;

public:
    int sprite_status;

    void update() {
        move({_velocity_coeff.x * _velocity.x, _velocity_coeff.y * _velocity.y});
        rotate(sf::radians(_rotation_coeff * _rotation));
    }

    void set_coeff_velocity_and_rot(sf::Vector2f velocity_coeff, float rotation_coeff){
        _velocity_coeff = velocity_coeff;
    }

    void set_velocity_and_rot(sf::Vector2i new_velocity, int new_rotation){
        _velocity = new_velocity;
        _rotation = new_rotation;
    }
};

class game_state_client final {
public:
    std::unordered_map<uint64_t, object> player_objects;
};

class game_state_server final{
    public:
        uint64_t next_player_unique_id = 0;
        std::vector<std::pair<uint64_t, object>> player_objects;

        void add_player(){
            player_objects.emplace_back();
            player_objects.back().first = next_player_unique_id++;
            player_objects.back().second.set_coeff_velocity_and_rot({1.0, 1.0}, 0.01);
        }

        void update_state(){
            for(int i = 0; i < player_objects.size(); ++i){
                player_objects[i].second.update();
            }
        }
};

class Mob {

public:
    sf::Sprite _sprite;
    sf::Vector2f _coords;
    sf::Texture _texture;

    uint32_t _health;
    uint32_t _speed;

    Mob() = default;
    Mob(const sf::Sprite& sprite, uint32_t health = 100) : _sprite(sprite), _health(health) {};

    void set_graphics(const sf::Sprite& sprite) {
       
        _sprite = sprite;
    }
    void set_position(sf::Vector2f coords) {
        _sprite.setPosition(coords);
    }
    void set_sprite(sf::Sprite& sprite) {
        _sprite = sprite;
    }
    void set_rotation(sf::Angle rot) {
        _sprite.setRotation(rot);
    }
    sf::Sprite get_sprite() {
        return _sprite;
    }
};

class window_info {
    sf::Font font;
    float fps;
    std::string string_brief;

public:

    window_info() = default;

    window_info(std::string file_name) {
        if (!font.openFromFile(file_name)) {
            std::cerr << "reading file error\n";
        }
    }

    void update_info(float delta_time, int health) {
        fps = 1000.0f / delta_time;
        string_brief = "FPS = " + std::to_string(fps) + "\nHealth = " + std::to_string(health);
        std::cout << string_brief << '\n';
    }

    void render(sf::RenderWindow& window, sf::View& view) {
        sf::Text frame_rate_text(font);
        //frame_rate_text.setFont(font);
        frame_rate_text.setString(string_brief);
        frame_rate_text.setCharacterSize(10); 
        frame_rate_text.setFillColor(sf::Color::White); 
       
        sf::Vector2f view_center = view.getCenter();
        sf::Vector2f view_size = view.getSize();

        frame_rate_text.setPosition(
            // {global_state.player_objects.at(index_cli).getPosition()}   
            {view_center.x - view_size.x / 2 + 20,
            view_center.y - view_size.y / 2 + 20}
        );

        window.draw(frame_rate_text);
    }

};
}


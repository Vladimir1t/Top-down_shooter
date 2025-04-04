#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <array>

enum class Status_sprite_index {
    UP     = 10,
    UP2    = 11,
    DOWN   = 20,
    DOWN2  = 21,
    RIGHT  = 30,
    RIGHT2 = 31,
    LEFT   = 40,
    LEFT2  = 41,
};

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

class game_state_server final {
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

    std::vector<sf::Sprite> _sprites;
    sf::Vector2f _coords;
    sf::Angle _rot;
    std::array<sf::Texture, 8> _textures;
    Status_sprite_index _ind;

public:
    uint32_t health;
    uint32_t speed;

    Mob(uint32_t health = 100) : health(health) {

        bool success = true;
	    success = success && _textures[0].loadFromFile ("Animations/Carry_Run/Carry_Run_Down-Sheet.png",
                                                            false, sf::IntRect({0, 0}, {64, 64}));   
        success = success && _textures[1].loadFromFile ("Animations/Carry_Run/Carry_Run_Down-Sheet.png", 
                                                            false, sf::IntRect({192, 0}, {64, 64}));   
        success = success && _textures[2].loadFromFile ("Animations/Carry_Run/Carry_Run_Up-Sheet.png", 
                                                            false, sf::IntRect({0, 0}, {64, 64})); 
        success = success && _textures[3].loadFromFile ("Animations/Carry_Run/Carry_Run_Up-Sheet.png", 
                                                            false, sf::IntRect({192, 0}, {64, 64}));
        success = success && _textures[4].loadFromFile ("Animations/Carry_Run/Carry_Run_Side-Sheet.png", 
                                                            false, sf::IntRect({0, 0}, {64, 64}));
        success = success && _textures[5].loadFromFile ("Animations/Carry_Run/Carry_Run_Side-Sheet.png", 
                                                            false, sf::IntRect({192, 0}, {64, 64}));
        success = success && _textures[6].loadFromFile ("Animations/Carry_Run/Carry_Run_LSide-Sheet.png", 
                                                            false, sf::IntRect({0, 0}, {64, 64}));   
        success = success && _textures[7].loadFromFile ("Animations/Carry_Run/Carry_Run_LSide-Sheet.png", 
                                                        false, sf::IntRect({192, 0}, {64, 64}));
        if (!success) 
            std::cout << "error while opening animation files\n";
    };

    void make_sprites() {
        for (int i = 0; i < 8; ++i) {
            sf::Sprite sprite(_textures[i]);
            _sprites.push_back(sprite);
        }
    }
    void set_position(sf::Vector2f coords) {
        _coords = coords;
    }
    void set_sprite(Status_sprite_index ind) {

        _ind = ind;
    }
    void set_rotation(sf::Angle rot) {
        _rot = rot;
    }
    sf::Sprite get_sprite() {
        sf::Sprite current_sprite = _sprites[0];
        switch(_ind) {
            case Status_sprite_index::DOWN:
                current_sprite = _sprites[0];
                break;
            case Status_sprite_index::DOWN2:
                current_sprite = _sprites[1];
                break;
            case Status_sprite_index::UP:
                current_sprite = _sprites[2];
                break;
            case Status_sprite_index::UP2:
                current_sprite = _sprites[3];
                break;
            case Status_sprite_index::RIGHT:
                current_sprite = _sprites[4];
                break;
            case Status_sprite_index::RIGHT2:
                current_sprite = _sprites[5];
                break;
            case Status_sprite_index::LEFT:
                current_sprite = _sprites[6];
                break;
            case Status_sprite_index::LEFT2:
                current_sprite = _sprites[7];
                break;
        }
        current_sprite.setPosition(_coords);
        current_sprite.setRotation(_rot);

        return current_sprite;
    }
};

class window_info {

    sf::Font _font;
    float _fps;
    std::string _string_info;

public:

    window_info() = default;

    window_info(std::string file_name) {
        if (!_font.openFromFile(file_name)) {
            std::cerr << "reading file error\n";
        }
    }

    void update_info(float delta_time, int health) {
        _fps = 1000.0f / delta_time;
        _string_info = "FPS = " + std::to_string(_fps) + "\nHealth = " + std::to_string(health);
    }

    void render(sf::RenderWindow& window, sf::View& view) {
        sf::Text frame_rate_text(_font);
        frame_rate_text.setString(_string_info);
        frame_rate_text.setCharacterSize(10); 
        frame_rate_text.setFillColor(sf::Color::White); 
       
        sf::Vector2f view_center = view.getCenter();
        sf::Vector2f view_size = view.getSize();

        frame_rate_text.setPosition({view_center.x - view_size.x / 2 + 20,
                                     view_center.y - view_size.y / 2 + 20});

        window.draw(frame_rate_text);
    }
};

class Map {

private:
    std::vector<sf::Sprite> _sprites;
    std::array<sf::Texture, 5> _textures;

public: 

    Map() = default;

    Map(std::string file_name) {
        bool success = true;
        success = success && _textures[0].loadFromFile (file_name, false, sf::IntRect({224, 0}, {48, 48}));   
        success = success && _textures[1].loadFromFile (file_name, false, sf::IntRect({272, 0}, {48, 48}));   
        success = success && _textures[2].loadFromFile (file_name, false, sf::IntRect({64, 0}, {48, 48}));  
        success = success && _textures[3].loadFromFile (file_name, false, sf::IntRect({0, 304}, {48, 48}));  
        success = success && _textures[4].loadFromFile (file_name, false, sf::IntRect({112, 0}, {48, 48}));  

        if (!success) 
            std::cout << "error while opening map file\n";
    }

    void make_map() {

        for (int i = 0; i < 110; ++i) {
            sf::Sprite sprite(_textures[i % 5]);
            _sprites.push_back(sprite);
        }
        for (float y = 0; y < 10; ++y) {
            for (float x = 0; x < 10; ++x) {
                int index = y * 10 + x;
                _sprites[index].setPosition({x * 46, y * 46});
            }
        }
    }

    void render(sf::RenderWindow& window) {
        std::cout << "size = " << _sprites.size() << '\n';
        for (auto block : _sprites) {
            window.draw(block);
        }
    }
};
}


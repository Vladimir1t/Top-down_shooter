#pragma once
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <array>
#include <vector>
#include <atomic>

#include "game_objects.hpp"

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

#ifdef 0
namespace sf {
/* --- overloading operator << for size_t */
inline Packet& operator<<(Packet& packet, size_t value) {
    return packet << static_cast<uint64_t>(value);
}
}
#endif

namespace game {

enum packet_type {
    move_bit = 0x1,
    mouse_bit = 0x10
};

struct mouse_input final {
    double x;
    double y;
    double angle;
};

struct control_struct final {
    bool move_changed;
    bool mouse_changed;
    
    int move_x;
    int move_y;
    int rotate;
    int sprite_status;

    mouse_input mouse;
};

class object: public sf::RectangleShape {

    sf::Vector2f _velocity_coeff;
    float _rotation_coeff;

public:
    sf::Vector2f _velocity_external; // all external factors modify this
    sf::Vector2i _velocity_internal; // velocity from internal movement 
    int sprite_status;
    int _rotation;
    int health;

    AABB<float> _hitbox;

    float get_center_x() {
        return _hitbox.x + _hitbox.width / 2;
    }
    float get_center_y() {
        return _hitbox.y + _hitbox.height / 2;
    }

    object(int health = 100): health(health), /* hitbox of mob */ _hitbox({16, 12, 32, 38}) { 
        /* Start coords */
        move({11 * 46, 11 * 46}); 
    }

    void move(sf::Vector2f offset) {
        object::RectangleShape::move(offset);
        _hitbox.x += offset.x;
        _hitbox.y += offset.y;
    }

    void update() {
        move({_velocity_coeff.x * (_velocity_internal.x + _velocity_external.x),
              _velocity_coeff.y * (_velocity_internal.y + _velocity_external.y)});
        rotate(sf::radians(_rotation_coeff * _rotation));
    }

    void set_coeff_velocity_and_rot(sf::Vector2f velocity_coeff, float rotation_coeff){
        _velocity_coeff = velocity_coeff;
        _rotation_coeff = rotation_coeff;
    }

    void set_internal_velocity_and_rot(sf::Vector2i new_velocity, int new_rotation){
        _velocity_internal = new_velocity;
        _rotation = new_rotation;
    }
};

template <typename T>
class Wall: public AABB<T> {
public:
    uint64_t id_;

    Wall(uint64_t id, T x, T y, T width, T height) {
        AABB<T>::set_bounds(x, y, width, height);
        id_ = id;
    };
};

class window_info final {
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

class Map final {

private:
    std::vector<sf::Sprite> _sprites;
    std::array<sf::Texture, 5> _textures;
    const uint64_t map_size = 100;
    const float map_block_size = 46;
    const float wall_size = 10;
    const float map_hole_size = 6;
    const float bloks_num_max = 3;

public: 
    std::vector<Wall<float>> walls;

    void make_textures(std::string file_name) {
        bool success = true;
        /* wood */
        success = success && _textures[0].loadFromFile (file_name, false, sf::IntRect({224, 0}, {48, 48}));   
        success = success && _textures[1].loadFromFile (file_name, false, sf::IntRect({272, 0}, {48, 48}));   
        /* stone */
        success = success && _textures[2].loadFromFile (file_name, false, sf::IntRect({64, 0}, {48, 48}));  
        success = success && _textures[3].loadFromFile (file_name, false, sf::IntRect({0, 304}, {48, 48}));  
        success = success && _textures[4].loadFromFile (file_name, false, sf::IntRect({112, 0}, {48, 48}));  

        if (!success) 
            std::cout << "error while opening map file\n";
    }

    void make_map() {

        /* --- wood map --- */
        for (uint64_t i = 0; i < map_size * map_size; ++i) {
            sf::Sprite sprite(_textures[0]);
            _sprites.push_back(sprite);
        }
        for (float y = 0; y < map_size; ++y) {
            for (float x = 0; x < map_size; ++x) {
                int index = y * map_size + x;
                _sprites[index].setPosition({x * 46, y * 46});
            }
        }
        /* --- stone map --- */
        build_stone_map();
    }

    void make_walls(sf::Packet& message){
        /* Bounds of map */
        uint64_t wall_count;
        
        uint64_t id;
        float x, y, width, height;

        message >> wall_count;
        walls.reserve(wall_count);

        for (uint64_t i = 0; i < wall_count; ++i){
            message >> id >> x >> y >> width >> height;
            std::cout << "wall: " << id << ' ' << x << ' ' << ' '
                      << y << ' ' << width << ' ' << height << std::endl;

            game::Wall<float> w = {id, x, y, width, height};
            walls.push_back(w);
        }
    }

    void render(sf::RenderWindow& window) const {
        for (auto block : _sprites) {
            window.draw(block);
        }
        #ifdef DEBUG
            for(auto& x: walls){
                sf::RectangleShape rect{{x.width, x.height}};
                rect.setFillColor({255,255,255});
                rect.move({x.x, x.y});
                window.draw(rect);
            }
        #endif
    }

private:
    void build_stone_map() {
        /* --- stone map --- */
        for (float block_num = 0; block_num < bloks_num_max; ++block_num) {
            for (float x = 10; x < wall_size + 10; ++x) {
                if (block_num != 0 && x > 13 && x < 17)
                    continue;
                sf::Sprite sprite(_textures[4]);
                sprite.setPosition({x * map_block_size, 10 * map_block_size + block_num * 
                        (map_block_size * 10 + map_hole_size * map_block_size)});
                _sprites.push_back(sprite);
            }
            for (float y = 10; y < wall_size + 10; ++y) {
                sf::Sprite sprite(_textures[4]);
                sprite.setPosition({10 * map_block_size, y * map_block_size + block_num * 
                        (map_block_size * 10 + map_hole_size * map_block_size)});
                _sprites.push_back(sprite);
            }
            for (float y = 10; y < wall_size + 10; ++y) {
                sf::Sprite sprite(_textures[4]);
                sprite.setPosition({(wall_size + 10) * map_block_size, y * map_block_size + block_num * 
                        (map_block_size * 10 + map_hole_size * map_block_size)});
                _sprites.push_back(sprite);
            }
            for (float x = 10; x < wall_size + 11; ++x) {
                if (block_num != bloks_num_max - 1 && x > 12 && x < 17)
                    continue;
                sf::Sprite sprite(_textures[4]);
                sprite.setPosition({x * map_block_size, (wall_size + 10) * map_block_size + block_num * 
                        (map_block_size * 10 + map_hole_size * map_block_size)});
                _sprites.push_back(sprite);
            }
            for (float block_num = 0; block_num < bloks_num_max - 1; ++block_num) {
                for (float y = 0; y < 6; ++y) {
                    sf::Sprite sprite(_textures[4]);
                    sprite.setPosition({13 * map_block_size, (wall_size + 10 + y) * map_block_size + block_num * 
                            (map_block_size * 10 + map_hole_size * map_block_size)});
                    _sprites.push_back(sprite);
                    sprite.setPosition({17 * map_block_size, (wall_size + 10 + y) * map_block_size + block_num * 
                            (map_block_size * 10 + map_hole_size * map_block_size)});
                    _sprites.push_back(sprite);
                }
            }
        }
    }
};

class game_state_client final {
public:
    std::unordered_map<uint64_t, object> player_objects;

    std::unordered_map<uint64_t, std::unique_ptr<updatable>> projectiles;
    projectile_factory factory;

    Map global_map;  
    std::atomic<bool> game_over{false};

    void create_from_settings(std::string proj_settings_filename){
        factory.read_settings(proj_settings_filename);
    }
};

class game_state_server final {
public:
    uint64_t next_player_unique_id = 0;
    std::vector<std::pair<uint64_t, object>> player_objects;
    std::vector<Wall<float>> walls;

    std::vector<std::unique_ptr<updatable>> objects;
    projectile_factory factory;

    void create_from_settings(std::string proj_settings_filename){
        factory.read_settings(proj_settings_filename);

        // magick nubers right now -> need to read from file
        const uint64_t map_size = 100;
        const float map_block_size = 46;
        const float map_hole_size = 6;
        const float bloks_num_max = 3;

        Wall<float> w = {0, 0, 0, map_size * map_block_size + 60, 5};
        walls.push_back(w);

        w = {0, 0, 0, 5, map_size * map_block_size + 60};
        walls.push_back(w);

        w = {0, -60, map_size * map_block_size, map_size * map_block_size + 60, 5};
        walls.push_back(w);

        w = {0, map_size * map_block_size, 0, 5, map_size * map_block_size + 60};
        walls.push_back(w);
    
        /* --- stone map --- */
        for (float block_num = 0; block_num < bloks_num_max; ++block_num) {
            w = {0, 10 * map_block_size, 10 * map_block_size + block_num * (map_block_size * 10 + map_hole_size * map_block_size),
                    map_block_size, 10 * map_block_size};
            walls.push_back(w);
            w = {0, 20 * map_block_size, 10 * map_block_size + block_num * (map_block_size * 10 + map_hole_size * map_block_size),
                    map_block_size, 10 * map_block_size};
            walls.push_back(w);
            if (block_num != 0) {
                w = {0, 10 * map_block_size, 10 * map_block_size + block_num * (map_block_size * 10 + map_hole_size * map_block_size), 
                        4 * map_block_size, map_block_size};
                walls.push_back(w);
                w = {0, 17 * map_block_size, 10 * map_block_size + block_num * (map_block_size * 10 + map_hole_size * map_block_size), 
                        4 * map_block_size, map_block_size};
                walls.push_back(w);
            } 
            else {
                w = {0, 10 * map_block_size, 10 * map_block_size + block_num * (map_block_size * 10 + map_hole_size * map_block_size), 
                        10 * map_block_size, map_block_size};
                walls.push_back(w);
            }
            if (block_num != bloks_num_max - 1) {
                w = {0, 10 * map_block_size, 20 * map_block_size + block_num * (map_block_size * 10 + map_hole_size * map_block_size), 
                        4 * map_block_size, map_block_size};
                walls.push_back(w);
                w = {0, 17 * map_block_size, 20 * map_block_size + block_num * (map_block_size * 10 + map_hole_size * map_block_size), 
                        4 * map_block_size, map_block_size};
                walls.push_back(w);
            }
            else {
                w = {0, 10 * map_block_size, 20 * map_block_size + block_num * (map_block_size * 10 + map_hole_size * map_block_size), 
                        10 * map_block_size, map_block_size};
                walls.push_back(w);
            }
        }
        for (float block_num = 0; block_num < bloks_num_max - 1; ++block_num) {
            w = {0, 13 * map_block_size, 20 * map_block_size + block_num * (map_block_size * 10 + map_hole_size * map_block_size), 
                    map_block_size, map_hole_size * map_block_size};
            walls.push_back(w);
            w = {0, 17 * map_block_size, 20 * map_block_size + block_num * (map_block_size * 10 + map_hole_size * map_block_size),
                    map_block_size, map_hole_size * map_block_size};
            walls.push_back(w);
        }
    };

    void add_player() {
        player_objects.emplace_back();
        player_objects.back().first = next_player_unique_id++;
        float start_move_coeff = static_cast<float>(next_player_unique_id);
        player_objects.back().second.set_coeff_velocity_and_rot({1.0, 1.0}, 0.01);
        player_objects.back().second.move({start_move_coeff, start_move_coeff});
    }

    void update_state() {
        for (uint64_t i = 0; i < player_objects.size(); ++i) {
            player_objects[i].second.update();
        }

        for (auto&& it = objects.begin(); it != objects.end();) {
            /*
             * iterating trough updatable objects.
             * if update invalidates object it will be count as inactive on next update_status iteration 
             * and will be send to clients once with parameter active = -1
             */
            if (it->get()->is_active()) {
                it->get()->update();
                it++;
            }
            else {
                it = objects.erase(it);
            }
        }
    }

    void create_projectile(float x, float y, float angle, uint64_t id = 1){
        objects.push_back(factory.get_projectile(x, y, angle, id));
    }
};

class Mob final {

    std::vector<sf::Sprite> _sprites;
    sf::Vector2f _coords;
    sf::Angle _rot;
    std::array<sf::Texture, 8> _textures;
    Status_sprite_index _ind;

public:
    uint64_t health;
    uint64_t speed;
    AABB<float> mob_bounds;

    Mob(uint64_t health = 100) : health(health) {

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
    };
    void set_position(sf::Vector2f coords) {
        _coords = coords;
        mob_bounds.set_bounds(_coords.x - 32, _coords.y - 32, 64, 64);
    };
    void set_sprite(Status_sprite_index ind) {
        _ind = ind;
    };
    void set_rotation(sf::Angle rot) {
        _rot = rot;
    };
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

class Animated_game_over {
public:
    Animated_game_over(std::string message) : _message(message) {}

    void end_dispay(sf::RenderWindow& window, const sf::Font& font, const game_state_client& global_state) {

        sf::Clock delayClock;
        while (window.isOpen() && delayClock.getElapsedTime().asSeconds() < wait_time) {
            while (auto event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                }
            }
            window.clear();
            global_state.global_map.render(window);
            draw(window, font);
            window.display();
        }
        window.close();
        abort();
        return;
    }

private:
    std::string _message; 
    const float wait_time = 3.0;

    void draw(sf::RenderWindow& window, const sf::Font& font) {
        sf::Text text(font);
        text.setString(_message);
        text.setCharacterSize(30); 
        text.setFillColor(sf::Color::Red);
        const float approx_text_width = 300.0f; 
        const float approx_text_height = 75.0f;

        sf::Vector2f center = window.getView().getCenter();
        text.setPosition({center.x - approx_text_width / 2, 
                          center.y - approx_text_height / 2});
        window.draw(text);
    }
};
}

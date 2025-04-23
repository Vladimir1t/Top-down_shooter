#pragma once
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <math.h>
#include <exception>

namespace game {

class AABB {
public:
    float x;
    float y;
    float width;
    float height;
    
    AABB(float x, float y, float width, float height) 
        : x(x), y(y), width(width), height(height) {}
        
    AABB() = default;
        
    void set_bounds(float x_, float y_, float width_, float height_) {
        x = x_;
        y = y_;
        width = width_;
        height = height_;
    }

    bool contains(const sf::Vector2f& point) const {
        return (point.x >= x) && 
                (point.x <= x + width) &&
                (point.y >= y) &&
                (point.y <= y + height);
    }
    
    bool intersects(const AABB& other) const {
        return (x < other.x + other.width) &&
                (x + width > other.x) &&
                (y < other.y + other.height) &&
                (y + height > other.y);
    }
    
    void clamp(sf::Vector2f& position, const sf::Vector2f& size) const {
        position.x = std::max(x, std::min(position.x, x + width - size.x));
        position.y = std::max(y, std::min(position.y, y + height - size.y));
    }
    
    float right() const { 
        return x + width; 
    }
    float bottom() const { 
        return y + height; 
    }
    sf::Vector2f center() const { 
        return {x + width/2, y + height/2}; 
    }

    void move(sf::Vector2f velocity){
        x += velocity.x;
        y += velocity.y;
    }
};

struct projectile_settings {
    AABB hitbox;
    float velocity; // vx^2 + vy^2 = const
    int damage;
};

struct object_base{
    AABB hitbox_;
    
    float angle;
    sf::Vector2f velocity_;
};

enum updatable_type {
    projectile_type = 0,
    other_type = 1
};

class updatable{
public:
    virtual bool update() = 0;
    virtual void render(sf::RenderWindow&) const = 0;
    virtual bool is_active() const = 0;
    virtual updatable_type get_type() const = 0;
    virtual ~updatable() {};
};


static size_t unique_index_counter = 0;

class projectile: public updatable{
    public:
    object_base base_;

    bool active_ = true;
    int damage_ = 100;
    size_t frame_counter_ = 0; //updates every tick
    size_t max_frames_ = 300;
    size_t unique_index = 0;
    size_t id_ = 0;

    std::vector<sf::Sprite>* base_sprites_;


    projectile(size_t id, float start_x, float start_y, float angle, projectile_settings settings, std::vector<sf::Sprite>* base_sprites):
        base_{{start_x, start_y, settings.hitbox.width, settings.hitbox.height}, 
               angle, 
              {settings.velocity*std::cos(angle), settings.velocity*std::sin(angle)}}, 
        damage_(settings.damage), unique_index(unique_index_counter++), id_(id), base_sprites_(base_sprites)
    {};

    bool update(){
        if(active_){
            base_.hitbox_.move(base_.velocity_);
            frame_counter_++;
        }

        if(frame_counter_ >= max_frames_) active_ = false;

        // std::cout << "updating " << unique_index << std::endl;

        return active_;
    };

    bool is_active() const{
        return active_;
    }

    updatable_type get_type() const {
        return projectile_type;
    }

    void render (sf::RenderWindow& window) const{
        sf::Sprite tmp_sprite = (*base_sprites_)[0];
        tmp_sprite.setPosition({base_.hitbox_.x, base_.hitbox_.y});
        tmp_sprite.setOrigin({tmp_sprite.getLocalBounds().size.x/2, tmp_sprite.getLocalBounds().size.y/2});
        tmp_sprite.setRotation(sf::radians(base_.angle));
        window.draw(tmp_sprite);
    };
};

class projectile_factory{
    std::unordered_map<size_t, projectile_settings> settings;

    std::unordered_map<size_t, std::vector<sf::Texture>> textures;
    std::unordered_map<size_t, std::vector<sf::Sprite>> sprites;

    public:

    void read_settings(std::string filename){
        std::ifstream file(filename);

        if(!file){
            throw std::ios::failure("error while opening file");
        }

        size_t count = 0;
        file >> count;

        size_t id = 0;
        float width, height, velocity = 0;
        int damage = 0;
        std::string sprite_path;
        size_t sprite_count = 0; 

        sf::Texture tmp_texture;
        std::vector<sf::Texture> tmp_texture_vector;

        std::vector<sf::Sprite> tmp_sprites;

        for(size_t i = 0; i < count; ++i){
            file >> id;
            file >> width >> height >> velocity >> damage;
            file >> sprite_count;
            std::cout << id << ' ' << width << ' ' << height << ' ' << velocity << ' ' << damage << ' ' << sprite_count << std::endl;
            for(size_t j = 0; j < sprite_count; ++j){
                file >> sprite_path;
                std::cout << '<' << sprite_path << '>' << std::endl;
                if(!tmp_texture.loadFromFile(sprite_path)) {
                    throw sf::Exception("can't load texture from file");
                }
                tmp_texture_vector.emplace_back(tmp_texture);
            }

            settings[id] = {{0, 0, width, height}, velocity, damage};
            textures[id] = tmp_texture_vector;

            for(auto& x: textures[id]){
                tmp_sprites.emplace_back(x);
            }
            sprites[id] = tmp_sprites;

            std::cout << "created projectile settings: id: " << id << " {0, 0, " << width << " " << height << "}, " << velocity << " " << damage << std::endl;
            std::cout << "sprite amount: " << sprites[id].size() << std::endl;
        }
    }

    //must return unique pointer, that contains pointer to "updatable" base of the new object
    //then it can be stored in std::vector<std::unique_ptr<updatable>>
    std::unique_ptr<updatable> get_projectile(float start_x, float start_y, float angle, size_t projectile_id){
        return std::unique_ptr<updatable>(new projectile {projectile_id, start_x, start_y, angle, settings[projectile_id], &(sprites[projectile_id])});
    };
};

};
#pragma once
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <array>
#include <vector>

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


class updatable{
public:
    virtual void update() const = 0;
    virtual ~updatable() {};
};

struct object_base{
    AABB hitbox;
    float angle; // angle = atan2(v.y / v.x)
    sf::Vector2f velocity;

    std::vector<sf::Sprite> base_sprites;
};

class bullet: public updatable{
    object_base base;

    int damage;
    size_t frame_counter; //updates every tick

public:
    void update(){
        base.hitbox.move(base.velocity);
        frame_counter++;
    };
};
}
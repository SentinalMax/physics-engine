#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <deque>
#include <chrono>

enum class ShapeType {
    CIRCLE,
    RECTANGLE,
    TRIANGLE
};

enum class BodyType {
    RIGID,
    SOFT
};

struct PhysicsProperties {
    float mass = 1.0f;
    float gravity = 9.81f;
    float friction = 0.1f;
    float restitution = 0.8f;
    bool isStatic = false;
};

struct BoundingBox {
    glm::vec2 min;
    glm::vec2 max;
    
    bool intersects(const BoundingBox& other) const;
    void update(const glm::vec2& position, float width, float height);
};

class Shape {
protected:
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec2 acceleration;
    glm::vec3 color;
    float rotation;
    float angularVelocity;
    PhysicsProperties physics;
    BoundingBox boundingBox;
    bool isSelected;
    bool isDragging;
    glm::vec2 dragOffset;
    // For drag momentum
    std::deque<std::pair<glm::vec2, double>> dragHistory; // position, time
    static constexpr size_t DRAG_HISTORY_SIZE = 5;
    bool useGlobalGravity = true;

public:
    Shape(const glm::vec2& pos, const glm::vec3& col = glm::vec3(1.0f));
    virtual ~Shape() = default;

    // Physics
    virtual void update(float deltaTime);
    virtual void applyForce(const glm::vec2& force);
    virtual void applyImpulse(const glm::vec2& impulse);
    
    // Collision
    virtual bool checkCollision(const Shape* other) const = 0;
    void resolveCollision(Shape* other, const glm::vec2& normal, float penetration);
    virtual BoundingBox getBoundingBox() const = 0;
    
    // Rendering
    virtual void render() const = 0;
    virtual void renderBoundingBox() const;
    
    // Interaction
    virtual bool containsPoint(const glm::vec2& point) const = 0;
    void startDrag(const glm::vec2& mousePos);
    void updateDrag(const glm::vec2& mousePos);
    void endDrag();
    
    // Getters and setters
    glm::vec2 getPosition() const { return position; }
    void setPosition(const glm::vec2& pos) { position = pos; }
    glm::vec2 getVelocity() const { return velocity; }
    void setVelocity(const glm::vec2& vel) { velocity = vel; }
    glm::vec3 getColor() const { return color; }
    void setColor(const glm::vec3& col) { color = col; }
    float getMass() const { return physics.mass; }
    void setMass(float m) { physics.mass = m; }
    float getGravity() const { return physics.gravity; }
    void setGravity(float g) { physics.gravity = g; }
    bool getIsSelected() const { return isSelected; }
    void setIsSelected(bool selected) { isSelected = selected; }
    bool getIsDragging() const { return isDragging; }
    PhysicsProperties& getPhysics() { return physics; }
    
    virtual ShapeType getType() const = 0;
    virtual BodyType getBodyType() const = 0;
    void setUseGlobalGravity(bool use) { useGlobalGravity = use; }
    bool getUseGlobalGravity() const { return useGlobalGravity; }
    
    // Bounding radius for collision detection
    virtual float getBoundingRadius() const = 0;
};

class Circle : public Shape {
private:
    float radius;

public:
    Circle(const glm::vec2& pos, float r, const glm::vec3& col = glm::vec3(1.0f));
    
    bool checkCollision(const Shape* other) const;
    void resolveCollision(Shape* other, const glm::vec2& normal, float penetration);
    BoundingBox getBoundingBox() const;
    void render() const;
    bool containsPoint(const glm::vec2& point) const;
    
    float getRadius() const { return radius; }
    void setRadius(float r) { radius = r; }
    
    ShapeType getType() const { return ShapeType::CIRCLE; }
    BodyType getBodyType() const { return BodyType::RIGID; }
    
    // Bounding radius for collision detection
    float getBoundingRadius() const;
};

class Rectangle : public Shape {
private:
    float width, height;

public:
    Rectangle(const glm::vec2& pos, float w, float h, const glm::vec3& col = glm::vec3(1.0f));
    
    bool checkCollision(const Shape* other) const;
    void resolveCollision(Shape* other, const glm::vec2& normal, float penetration);
    BoundingBox getBoundingBox() const;
    void render() const;
    bool containsPoint(const glm::vec2& point) const;
    
    float getWidth() const { return width; }
    float getHeight() const { return height; }
    void setWidth(float w) { width = w; }
    void setHeight(float h) { height = h; }
    
    ShapeType getType() const { return ShapeType::RECTANGLE; }
    BodyType getBodyType() const { return BodyType::RIGID; }
    
    // Bounding radius for collision detection
    float getBoundingRadius() const;
};

class Triangle : public Shape {
private:
    std::vector<glm::vec2> vertices;
    float sideLength;

public:
    Triangle(const glm::vec2& pos, float side, const glm::vec3& col = glm::vec3(1.0f));
    
    bool checkCollision(const Shape* other) const;
    void resolveCollision(Shape* other, const glm::vec2& normal, float penetration);
    BoundingBox getBoundingBox() const;
    void render() const;
    bool containsPoint(const glm::vec2& point) const;
    
    float getSideLength() const { return sideLength; }
    void setSideLength(float side);
    void updateVertices();
    
    ShapeType getType() const { return ShapeType::TRIANGLE; }
    BodyType getBodyType() const { return BodyType::RIGID; }
    
    // Bounding radius for collision detection
    float getBoundingRadius() const;
}; 
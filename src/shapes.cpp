#ifdef Rectangle
#undef Rectangle
#endif
#ifdef Circle
#undef Circle
#endif
#ifdef Triangle
#undef Triangle
#endif
#include "shapes.h"
#include "config.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <algorithm>
#include <chrono>

// BoundingBox implementation
bool BoundingBox::intersects(const BoundingBox& other) const {
    return (min.x <= other.max.x && max.x >= other.min.x) &&
           (min.y <= other.max.y && max.y >= other.min.y);
}

void BoundingBox::update(const glm::vec2& position, float width, float height) {
    min = position - glm::vec2(width * 0.5f, height * 0.5f);
    max = position + glm::vec2(width * 0.5f, height * 0.5f);
}

// Shape base class implementation
Shape::Shape(const glm::vec2& pos, const glm::vec3& col)
    : position(pos), velocity(0.0f), acceleration(0.0f), color(col), rotation(0.0f), angularVelocity(0.0f),
      isSelected(false), isDragging(false), dragOffset(0.0f), useGlobalGravity(true),
      currentLOD(100.0f, 1.0f, true, true), lastUpdateTime(0.0f), updateAccumulator(0.0f),
      needsDetailedUpdate(true), isOptimized(false) {
    physics.position = pos;
    physics.velocity = glm::vec2(0.0f);
    physics.rotation = 0.0f;
}

void Shape::update(float deltaTime) {
    // LOD-based update frequency
    if (!shouldUpdate(deltaTime)) {
        return;
    }
    
    // Apply gravity
    if (!physics.isStatic && useGlobalGravity) {
        acceleration.y -= physics.gravity;
    }
    
    // Update velocity
    velocity += acceleration * deltaTime;
    
    // Apply friction
    velocity *= (1.0f - physics.friction * deltaTime);
    
    // Update position
    position += velocity * deltaTime;
    
    // Update rotation
    rotation += angularVelocity * deltaTime;
    
    // Reset acceleration
    acceleration = glm::vec2(0.0f);
    
    // Update bounding box
    updateBoundingBox();
}

bool Shape::shouldUpdate(float deltaTime) {
    updateAccumulator += deltaTime;
    
    // Check if enough time has passed for this LOD level
    if (updateAccumulator >= currentLOD.updateInterval) {
        updateAccumulator = 0.0f;
        lastUpdateTime = deltaTime;
        return true;
    }
    
    return false;
}

void Shape::applyForce(const glm::vec2& force) {
    if (!physics.isStatic) {
        acceleration += force / physics.mass;
    }
}

void Shape::applyImpulse(const glm::vec2& impulse) {
    if (!physics.isStatic) {
        velocity += impulse / physics.mass;
    }
}

void Shape::renderBoundingBox() const {
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(boundingBox.min.x, boundingBox.min.y);
    glVertex2f(boundingBox.max.x, boundingBox.min.y);
    glVertex2f(boundingBox.max.x, boundingBox.max.y);
    glVertex2f(boundingBox.min.x, boundingBox.max.y);
    glEnd();
    glLineWidth(1.0f);
}

void Shape::startDrag(const glm::vec2& mousePos) {
    isDragging = true;
    dragOffset = mousePos - getPosition();
    setVelocity(glm::vec2(0.0f));
    angularVelocity = 0.0f;
    dragHistory.clear();
    double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    dragHistory.push_back({mousePos, now});
}

void Shape::updateDrag(const glm::vec2& mousePos) {
    if (isDragging) {
        setPosition(mousePos - dragOffset);
        boundingBox = getBoundingBox();
        double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        dragHistory.push_back({mousePos, now});
        if (dragHistory.size() > DRAG_HISTORY_SIZE) dragHistory.pop_front();
    }
}

void Shape::endDrag() {
    isDragging = false;
    // Calculate momentum (velocity) from last drag positions
    if (dragHistory.size() >= 2) {
        auto& last = dragHistory.back();
        auto& prev = dragHistory[dragHistory.size() - 2];
        double dt = last.second - prev.second;
        if (dt > 0.0001) {
            glm::vec2 v = (last.first - prev.first) / static_cast<float>(dt);
            setVelocity(v);
        }
    }
    dragHistory.clear();
}

void Shape::updateBoundingBox() {
    boundingBox = getBoundingBox();
}

// Circle implementation
Circle::Circle(const glm::vec2& pos, float r, const glm::vec3& col) 
    : Shape(pos, col), radius(r) {
}

bool Circle::checkCollision(const Shape* other) const {
    switch (other->getType()) {
        case ShapeType::CIRCLE: {
            const Circle* circle = static_cast<const Circle*>(other);
            glm::vec2 diff = getPosition() - circle->getPosition();
            float distance = glm::length(diff);
            return distance < (radius + circle->radius);
        }
        case ShapeType::RECTANGLE: {
            const Rectangle* rect = static_cast<const Rectangle*>(other);
            glm::vec2 closest = getPosition();
            closest.x = std::max(rect->getPosition().x - rect->getWidth() * 0.5f, 
                                std::min(getPosition().x, rect->getPosition().x + rect->getWidth() * 0.5f));
            closest.y = std::max(rect->getPosition().y - rect->getHeight() * 0.5f, 
                                std::min(getPosition().y, rect->getPosition().y + rect->getHeight() * 0.5f));
            
            glm::vec2 diff = getPosition() - closest;
            return glm::length(diff) < radius;
        }
        case ShapeType::TRIANGLE: {
            // Simplified circle-triangle collision using bounding box
            return getBoundingBox().intersects(other->getBoundingBox());
        }
        default:
            return false;
    }
}

void Circle::resolveCollision(Shape* other, const glm::vec2& normal, float penetration) {
    if (physics.isStatic && other->getPhysics().isStatic) return;
    
    // Only handle velocity resolution here - positional correction is handled by physics engine
    glm::vec2 relativeVel = getVelocity() - other->getVelocity();
    float velAlongNormal = glm::dot(relativeVel, normal);
    
    if (velAlongNormal > 0) return; // Objects are separating
    
    float restitution = std::min(physics.restitution, other->getPhysics().restitution);
    float j = -(1.0f + restitution) * velAlongNormal;
    j /= 1.0f / physics.mass + 1.0f / other->getPhysics().mass;
    
    glm::vec2 impulse = j * normal;
    
    if (!physics.isStatic) {
        setVelocity(getVelocity() + impulse / physics.mass);
    }
    if (!other->getPhysics().isStatic) {
        other->setVelocity(other->getVelocity() - impulse / other->getPhysics().mass);
    }
}

BoundingBox Circle::getBoundingBox() const {
    BoundingBox box;
    box.min = getPosition() - glm::vec2(radius);
    box.max = getPosition() + glm::vec2(radius);
    return box;
}

void Circle::render() const {
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(getPosition().x, getPosition().y);
    for (int i = 0; i <= 32; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / 32.0f;
        glVertex2f(getPosition().x + radius * cos(angle), 
                   getPosition().y + radius * sin(angle));
    }
    glEnd();
    
    if (isSelected) {
        renderBoundingBox();
    }
}

bool Circle::containsPoint(const glm::vec2& point) const {
    glm::vec2 diff = point - getPosition();
    return glm::length(diff) <= radius;
}

// Rectangle implementation
Rectangle::Rectangle(const glm::vec2& pos, float w, float h, const glm::vec3& col) 
    : Shape(pos, col), width(w), height(h) {
}

bool Rectangle::checkCollision(const Shape* other) const {
    switch (other->getType()) {
        case ShapeType::CIRCLE: {
            const Circle* circle = static_cast<const Circle*>(other);
            glm::vec2 closest = circle->getPosition();
            closest.x = std::max(getPosition().x - width * 0.5f, 
                                std::min(circle->getPosition().x, getPosition().x + width * 0.5f));
            closest.y = std::max(getPosition().y - height * 0.5f, 
                                std::min(circle->getPosition().y, getPosition().y + height * 0.5f));
            
            glm::vec2 diff = circle->getPosition() - closest;
            return glm::length(diff) < circle->getRadius();
        }
        case ShapeType::RECTANGLE: {
            const Rectangle* rect = static_cast<const Rectangle*>(other);
            return getBoundingBox().intersects(rect->getBoundingBox());
        }
        case ShapeType::TRIANGLE: {
            return getBoundingBox().intersects(other->getBoundingBox());
        }
        default:
            return false;
    }
}

void Rectangle::resolveCollision(Shape* other, const glm::vec2& normal, float penetration) {
    if (physics.isStatic && other->getPhysics().isStatic) return;
    
    // Only handle velocity resolution here - positional correction is handled by physics engine
    glm::vec2 relativeVel = getVelocity() - other->getVelocity();
    float velAlongNormal = glm::dot(relativeVel, normal);
    
    if (velAlongNormal > 0) return;
    
    float restitution = std::min(physics.restitution, other->getPhysics().restitution);
    float j = -(1.0f + restitution) * velAlongNormal;
    j /= 1.0f / physics.mass + 1.0f / other->getPhysics().mass;
    
    glm::vec2 impulse = j * normal;
    
    if (!physics.isStatic) {
        setVelocity(getVelocity() + impulse / physics.mass);
    }
    if (!other->getPhysics().isStatic) {
        other->setVelocity(other->getVelocity() - impulse / other->getPhysics().mass);
    }
}

BoundingBox Rectangle::getBoundingBox() const {
    BoundingBox box;
    box.update(getPosition(), width, height);
    return box;
}

void Rectangle::render() const {
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_QUADS);
    glVertex2f(getPosition().x - width * 0.5f, getPosition().y - height * 0.5f);
    glVertex2f(getPosition().x + width * 0.5f, getPosition().y - height * 0.5f);
    glVertex2f(getPosition().x + width * 0.5f, getPosition().y + height * 0.5f);
    glVertex2f(getPosition().x - width * 0.5f, getPosition().y + height * 0.5f);
    glEnd();
    
    if (isSelected) {
        renderBoundingBox();
    }
}

bool Rectangle::containsPoint(const glm::vec2& point) const {
    return point.x >= getPosition().x - width * 0.5f && 
           point.x <= getPosition().x + width * 0.5f &&
           point.y >= getPosition().y - height * 0.5f && 
           point.y <= getPosition().y + height * 0.5f;
}

// Triangle implementation
Triangle::Triangle(const glm::vec2& pos, float side, const glm::vec3& col) 
    : Shape(pos, col), sideLength(side) {
    updateVertices();
}

void Triangle::setSideLength(float side) {
    sideLength = side;
    updateVertices();
}

void Triangle::updateVertices() {
    vertices.clear();
    float height = sideLength * static_cast<float>(sqrt(3.0f)) / 2.0f;
    
    vertices.push_back(getPosition() + glm::vec2(0, height / 3.0f)); // Top
    vertices.push_back(getPosition() + glm::vec2(-sideLength / 2.0f, -height / 3.0f)); // Bottom left
    vertices.push_back(getPosition() + glm::vec2(sideLength / 2.0f, -height / 3.0f)); // Bottom right
}

bool Triangle::checkCollision(const Shape* other) const {
    // Simplified collision using bounding box for now
    return getBoundingBox().intersects(other->getBoundingBox());
}

void Triangle::resolveCollision(Shape* other, const glm::vec2& normal, float penetration) {
    if (physics.isStatic && other->getPhysics().isStatic) return;
    
    // Only handle velocity resolution here - positional correction is handled by physics engine
    glm::vec2 relativeVel = getVelocity() - other->getVelocity();
    float velAlongNormal = glm::dot(relativeVel, normal);
    
    if (velAlongNormal > 0) return;
    
    float restitution = std::min(physics.restitution, other->getPhysics().restitution);
    float j = -(1.0f + restitution) * velAlongNormal;
    j /= 1.0f / physics.mass + 1.0f / other->getPhysics().mass;
    
    glm::vec2 impulse = j * normal;
    
    if (!physics.isStatic) {
        setVelocity(getVelocity() + impulse / physics.mass);
        updateVertices(); // Update triangle vertices after position change
    }
    if (!other->getPhysics().isStatic) {
        other->setVelocity(other->getVelocity() - impulse / other->getPhysics().mass);
    }
}

BoundingBox Triangle::getBoundingBox() const {
    BoundingBox box;
    float height = sideLength * static_cast<float>(sqrt(3.0f)) / 2.0f;
    box.min = getPosition() - glm::vec2(sideLength / 2.0f, height / 3.0f);
    box.max = getPosition() + glm::vec2(sideLength / 2.0f, height * 2.0f / 3.0f);
    return box;
}

void Triangle::render() const {
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_TRIANGLES);
    for (const auto& vertex : vertices) {
        glVertex2f(vertex.x, vertex.y);
    }
    glEnd();
    
    if (isSelected) {
        renderBoundingBox();
    }
}

bool Triangle::containsPoint(const glm::vec2& point) const {
    // Barycentric coordinate test
    glm::vec2 v0 = vertices[1] - vertices[0];
    glm::vec2 v1 = vertices[2] - vertices[0];
    glm::vec2 v2 = point - vertices[0];
    
    float dot00 = glm::dot(v0, v0);
    float dot01 = glm::dot(v0, v1);
    float dot02 = glm::dot(v0, v2);
    float dot11 = glm::dot(v1, v1);
    float dot12 = glm::dot(v1, v2);
    
    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    
    return (u >= 0) && (v >= 0) && (u + v <= 1);
}

// Bounding radius implementations
float Circle::getBoundingRadius() const {
    return radius;
}

float Rectangle::getBoundingRadius() const {
    return sqrt(width * width + height * height) * 0.5f;
}

float Triangle::getBoundingRadius() const {
    return sideLength * 0.577f; // Approximate radius of equilateral triangle
}

// Collision resolution implementation
void Shape::resolveCollision(Shape* other, const glm::vec2& normal, float penetration) {
    if (physics.isStatic || other->getPhysics().isStatic) return;
    
    // Calculate relative velocity
    glm::vec2 relativeVel = other->getVelocity() - getVelocity();
    
    // Calculate impulse
    float velAlongNormal = glm::dot(relativeVel, normal);
    if (velAlongNormal > 0) return; // Objects are moving apart
    
    float restitution = std::min(physics.restitution, other->getPhysics().restitution);
    float j = -(1.0f + restitution) * velAlongNormal;
    j /= 1.0f / getMass() + 1.0f / other->getMass();
    
    // Apply impulse
    glm::vec2 impulse = j * normal;
    setVelocity(getVelocity() - impulse / getMass());
    other->setVelocity(other->getVelocity() + impulse / other->getMass());
} 
#pragma once
#include "shapes.h"
#include <vector>
#include <memory>
#include <functional>
#include <glm/glm.hpp>

struct CollisionInfo {
    Shape* shapeA;
    Shape* shapeB;
    glm::vec2 normal;
    float penetration;
    
    CollisionInfo(Shape* a, Shape* b, const glm::vec2& n, float p)
        : shapeA(a), shapeB(b), normal(n), penetration(p) {}
};

// Forward declarations
class Shape;

// Spatial partitioning cell
struct SpatialCell {
    glm::vec2 min, max;
    std::vector<Shape*> objects;
    float energyDensity;
    int lastUpdateFrame;
    
    SpatialCell(const glm::vec2& min, const glm::vec2& max) 
        : min(min), max(max), energyDensity(0.0f), lastUpdateFrame(0) {}
    
    bool contains(const glm::vec2& point) const {
        return point.x >= min.x && point.x <= max.x && 
               point.y >= min.y && point.y <= max.y;
    }
    
    bool overlaps(const glm::vec2& center, float radius) const {
        glm::vec2 closest = glm::clamp(center, min, max);
        return glm::distance(center, closest) <= radius;
    }
};

// Neighbor tracking for each object
struct NeighborInfo {
    Shape* object;
    std::vector<Shape*> neighbors;
    int lastUpdateFrame;
    float lastUpdateTime;
    
    NeighborInfo(Shape* obj) : object(obj), lastUpdateFrame(0), lastUpdateTime(0.0f) {}
};

// Collision prediction using vector analysis
struct CollisionPrediction {
    Shape* obj1, *obj2;
    float predictedTime;
    bool willCollide;
    
    CollisionPrediction(Shape* a, Shape* b, float time, bool collide) 
        : obj1(a), obj2(b), predictedTime(time), willCollide(collide) {}
};

class PhysicsEngine {
private:
    std::vector<std::unique_ptr<Shape>> shapes;
    std::vector<CollisionInfo> collisions;
    glm::vec2 worldGravity;
    glm::vec2 worldBounds;
    float timeStep;
    int iterations;
    
    // Mouse interaction
    Shape* selectedShape;
    glm::vec2 mousePosition;
    bool mousePressed;
    
    // Optimization structures
    std::vector<SpatialCell> spatialGrid;
    std::vector<NeighborInfo> neighborTracking;
    std::vector<CollisionPrediction> collisionPredictions;
    
    // Optimization parameters
    int currentFrame;
    int neighborUpdateInterval;
    int spatialUpdateInterval;
    float maxNeighborDistance;
    int targetCellsPerObject;
    float energyThreshold;
    
    // Performance tracking
    int collisionChecksThisFrame;
    int actualCollisionsThisFrame;
    float averageCollisionTime;
    
    // Collision detection
    bool checkCollision(const Shape* a, const Shape* b, glm::vec2& normal, float& penetration) const;
    CollisionInfo detectCollision(Shape* a, Shape* b);
    void resolveCollisions();
    
    // Utility functions
    glm::vec2 screenToWorld(const glm::vec2& screenPos) const;
    glm::vec2 worldToScreen(const glm::vec2& worldPos) const;
    
    // Optimization methods
    void updateSpatialPartitioning();
    void updateNeighborTracking();
    void predictCollisions();
    std::vector<Shape*> getPotentialCollisions(Shape* shape);
    float calculateEnergyDensity(const SpatialCell& cell);
    bool shouldUpdateNeighbors(Shape* shape);
    bool shouldUpdateSpatialGrid();
    void optimizeCollisionDetection();
    std::vector<SpatialCell> createAdaptiveGrid();
    float predictCollisionTime(Shape* a, Shape* b);
    bool willCollideInTimeframe(Shape* a, Shape* b, float timeframe);

public:
    PhysicsEngine();
    ~PhysicsEngine() = default;
    
    // World management
    void setWorldBounds(const glm::vec2& bounds) { worldBounds = bounds; }
    void setGravity(const glm::vec2& gravity) { 
        worldGravity = gravity; 
        // Update all shapes that use global gravity
        for (auto& shape : shapes) {
            if (shape->getUseGlobalGravity()) {
                shape->setGravity(gravity.y);
            }
        }
    }
    void setTimeStep(float dt) { timeStep = dt; }
    void setIterations(int iter) { iterations = iter; }
    
    // Shape management
    void addShape(std::unique_ptr<Shape> shape);
    void removeShape(Shape* shape);
    void clearShapes();
    
    // Physics simulation
    void update(float deltaTime);
    void render() const;
    void renderVelocityVectors() const;
    
    // Mouse interaction
    void handleMousePress(const glm::vec2& mousePos);
    void handleMouseRelease();
    void handleMouseMove(const glm::vec2& mousePos);
    Shape* getSelectedShape() const { return selectedShape; }
    
    // Utility
    glm::vec2 getWorldBounds() const { return worldBounds; }
    glm::vec2 getGravity() const { return worldGravity; }
    
    // Shape access
    const std::vector<std::unique_ptr<Shape>>& getShapes() const { return shapes; }
    
    // Performance monitoring
    int getCollisionChecksThisFrame() const { return collisionChecksThisFrame; }
    int getActualCollisionsThisFrame() const { return actualCollisionsThisFrame; }
    float getAverageCollisionTime() const { return averageCollisionTime; }
    int getSpatialCellCount() const { return static_cast<int>(spatialGrid.size()); }
    int getNeighborTrackingCount() const { return static_cast<int>(neighborTracking.size()); }
    
    // Optimization control
    void setNeighborUpdateInterval(int frames) { neighborUpdateInterval = frames; }
    void setSpatialUpdateInterval(int frames) { spatialUpdateInterval = frames; }
    void setMaxNeighborDistance(float distance) { maxNeighborDistance = distance; }
    void setTargetCellsPerObject(int count) { targetCellsPerObject = count; }
    void setEnergyThreshold(float threshold) { energyThreshold = threshold; }
}; 
#include "physics_engine.h"
#include "config.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <glm/gtx/rotate_vector.hpp>

using namespace std;

// Helper function for 2D rotation
glm::vec2 rotate2D(const glm::vec2& vec, float angle) {
    float cos_a = cos(angle);
    float sin_a = sin(angle);
    return glm::vec2(
        vec.x * cos_a - vec.y * sin_a,
        vec.x * sin_a + vec.y * cos_a
    );
}

PhysicsEngine::PhysicsEngine() 
    : selectedShape(nullptr), worldBounds(1200.0f, 800.0f), worldGravity(0.0f, -9.81f),
      mousePosition(0.0f, 0.0f), mousePressed(false),
      currentFrame(0), neighborUpdateInterval(10), spatialUpdateInterval(5),
      maxNeighborDistance(100.0f), targetCellsPerObject(100), energyThreshold(0.1f),
      collisionChecksThisFrame(0), actualCollisionsThisFrame(0), averageCollisionTime(0.0f) {
    
    // Initialize spatial grid
    updateSpatialPartitioning();
}

void PhysicsEngine::addShape(std::unique_ptr<Shape> shape) {
    shapes.push_back(std::move(shape));
}

void PhysicsEngine::removeShape(Shape* shape) {
    shapes.erase(std::remove_if(shapes.begin(), shapes.end(),
        [shape](const std::unique_ptr<Shape>& s) { return s.get() == shape; }), shapes.end());
}

void PhysicsEngine::clearShapes() {
    shapes.clear();
    selectedShape = nullptr;
    spatialGrid.clear();
    neighborTracking.clear();
    collisionPredictions.clear();
}

// getShapes method removed - using the const reference version from header

void PhysicsEngine::update(float deltaTime) {
    currentFrame++;
    
    // Reset performance counters
    collisionChecksThisFrame = 0;
    actualCollisionsThisFrame = 0;
    
    // Update optimization systems
    if (shouldUpdateSpatialGrid()) {
        updateSpatialPartitioning();
    }
    
    if (currentFrame % neighborUpdateInterval == 0) {
        updateNeighborTracking();
    }
    
    // Predict collisions for next few frames
    predictCollisions();
    
    // Update physics with optimized collision detection
    optimizeCollisionDetection();
    
    // Update all shapes
    for (auto& shape : shapes) {
        if (!shape->getPhysics().isStatic) {
            shape->update(deltaTime);
        }
    }
    
    // Update average collision time
    if (actualCollisionsThisFrame > 0) {
        averageCollisionTime = (averageCollisionTime * 0.9f) + (deltaTime * 0.1f);
    }
}

// Old collision detection methods removed - replaced with optimized version

bool PhysicsEngine::checkCollision(const Shape* a, const Shape* b, glm::vec2& normal, float& penetration) const {
    if (a->getType() == ShapeType::CIRCLE && b->getType() == ShapeType::CIRCLE) {
        const Circle* circleA = static_cast<const Circle*>(a);
        const Circle* circleB = static_cast<const Circle*>(b);
        
        glm::vec2 diff = circleB->getPosition() - circleA->getPosition();
        float distance = glm::length(diff);
        float radiusSum = circleA->getRadius() + circleB->getRadius();
        
        // Handle overlapping circles
        if (distance < radiusSum) {
            if (distance < 0.001f) {
                // Circles are very close or overlapping - use a default normal
                normal = glm::vec2(1.0f, 0.0f);
                penetration = radiusSum;
            } else {
                normal = glm::normalize(diff);
                penetration = radiusSum - distance;
            }
            return true;
        }
    }
    else if (a->getType() == ShapeType::RECTANGLE && b->getType() == ShapeType::RECTANGLE) {
        const Rectangle* rectA = static_cast<const Rectangle*>(a);
        const Rectangle* rectB = static_cast<const Rectangle*>(b);
        
        BoundingBox boxA = rectA->getBoundingBox();
        BoundingBox boxB = rectB->getBoundingBox();
        
        float overlapX = std::min(boxA.max.x - boxB.min.x, boxB.max.x - boxA.min.x);
        float overlapY = std::min(boxA.max.y - boxB.min.y, boxB.max.y - boxA.min.y);
        
        if (overlapX > 0 && overlapY > 0) {
            if (overlapX < overlapY) {
                normal = glm::vec2(boxA.max.x < boxB.max.x ? -1.0f : 1.0f, 0.0f);
                penetration = overlapX;
            } else {
                normal = glm::vec2(0.0f, boxA.max.y < boxB.max.y ? -1.0f : 1.0f);
                penetration = overlapY;
            }
            return true;
        }
    }
    else if ((a->getType() == ShapeType::CIRCLE && b->getType() == ShapeType::RECTANGLE) ||
             (a->getType() == ShapeType::RECTANGLE && b->getType() == ShapeType::CIRCLE)) {
        
        const Circle* circle = (a->getType() == ShapeType::CIRCLE) ? 
                              static_cast<const Circle*>(a) : static_cast<const Circle*>(b);
        const Rectangle* rect = (a->getType() == ShapeType::RECTANGLE) ? 
                               static_cast<const Rectangle*>(a) : static_cast<const Rectangle*>(b);
        
        glm::vec2 closest = circle->getPosition();
        closest.x = std::max(rect->getPosition().x - rect->getWidth() * 0.5f, 
                            std::min(circle->getPosition().x, rect->getPosition().x + rect->getWidth() * 0.5f));
        closest.y = std::max(rect->getPosition().y - rect->getHeight() * 0.5f, 
                            std::min(circle->getPosition().y, rect->getPosition().y + rect->getHeight() * 0.5f));
        
        glm::vec2 diff = circle->getPosition() - closest;
        float distance = glm::length(diff);
        
        if (distance < circle->getRadius()) {
            if (distance < 0.001f) {
                // Circle is very close to rectangle edge - use a default normal
                normal = glm::vec2(1.0f, 0.0f);
                penetration = circle->getRadius();
            } else {
                normal = glm::normalize(diff);
                penetration = circle->getRadius() - distance;
            }
            return true;
        }
    }
    
    return false;
}

CollisionInfo PhysicsEngine::detectCollision(Shape* a, Shape* b) {
    glm::vec2 normal;
    float penetration;
    
    if (checkCollision(a, b, normal, penetration)) {
        return CollisionInfo(a, b, normal, penetration);
    }
    
    return CollisionInfo(nullptr, nullptr, glm::vec2(0), 0);
}

void PhysicsEngine::resolveCollisions() {
    // Positional correction parameters - less aggressive to prevent sticking
    const float percent = 0.2f; // Reduced from 0.8f to 0.2f
    const float slop = 0.05f;   // Increased from 0.01f to 0.05f

    for (const auto& collision : collisions) {
        collision.shapeA->resolveCollision(collision.shapeB, collision.normal, collision.penetration);

        // --- Positional correction ---
        float penetration = std::max(collision.penetration - slop, 0.0f);
        float totalMass = collision.shapeA->getMass() + collision.shapeB->getMass();
        if (totalMass == 0) continue;
        glm::vec2 correction = (penetration / totalMass) * percent * collision.normal;
        if (!collision.shapeA->getPhysics().isStatic) {
            collision.shapeA->setPosition(collision.shapeA->getPosition() - correction * (collision.shapeB->getMass() / totalMass));
        }
        if (!collision.shapeB->getPhysics().isStatic) {
            collision.shapeB->setPosition(collision.shapeB->getPosition() + correction * (collision.shapeA->getMass() / totalMass));
        }
    }
}

void PhysicsEngine::render() const {
    // Render all shapes
    for (const auto& shape : shapes) {
        shape->render();
    }
}

void PhysicsEngine::renderVelocityVectors() const {
    // Render velocity vectors for all shapes
    for (const auto& shape : shapes) {
        glm::vec2 velocity = shape->getVelocity();
        float speed = glm::length(velocity);
        
        // Only draw if there's significant velocity
        if (speed > 0.1f) {
            glm::vec2 position = shape->getPosition();
            glm::vec2 direction = glm::normalize(velocity);
            
            // Make the vector length more sensitive to speed
            float maxLength = 120.0f;
            float length = std::min(speed * 6.0f, maxLength);
            glm::vec2 endPoint = position + direction * length;
            
            // Draw the velocity vector
            glColor3f(1.0f, 1.0f, 0.0f); // Yellow for velocity
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glVertex2f(position.x, position.y);
            glVertex2f(endPoint.x, endPoint.y);
            glEnd();
            
            // Draw an arrowhead
            float arrowSize = 12.0f;
            glm::vec2 arrowDir1 = rotate2D(direction, -0.3f) * arrowSize;
            glm::vec2 arrowDir2 = rotate2D(direction, 0.3f) * arrowSize;
            
            glBegin(GL_TRIANGLES);
            glVertex2f(endPoint.x, endPoint.y);
            glVertex2f(endPoint.x - arrowDir1.x, endPoint.y - arrowDir1.y);
            glVertex2f(endPoint.x - arrowDir2.x, endPoint.y - arrowDir2.y);
            glEnd();
            
            glLineWidth(1.0f);
        }
    }
}

void PhysicsEngine::handleMousePress(const glm::vec2& mousePos) {
    mousePosition = screenToWorld(mousePos);
    mousePressed = true;
    
    cout << "Mouse press at screen: (" << mousePos.x << ", " << mousePos.y << ")" << endl;
    cout << "Mouse press at world: (" << mousePosition.x << ", " << mousePosition.y << ")" << endl;
    
    // Find shape under mouse
    selectedShape = nullptr;
    cout << "Checking " << shapes.size() << " shapes for selection..." << endl;
    
    for (auto& shape : shapes) {
        glm::vec2 shapePos = shape->getPosition();
        cout << "Shape at (" << shapePos.x << ", " << shapePos.y << ") - ";
        
        if (shape->containsPoint(mousePosition)) {
            selectedShape = shape.get();
            selectedShape->setIsSelected(true);
            selectedShape->startDrag(mousePosition);
            cout << "SELECTED!" << endl;
            cout << "Selected shape at world position: (" << selectedShape->getPosition().x << ", " << selectedShape->getPosition().y << ")" << endl;
            break;
        } else {
            cout << "not selected" << endl;
        }
    }
    
    if (!selectedShape) {
        cout << "No shape selected" << endl;
    }
}

void PhysicsEngine::handleMouseRelease() {
    mousePressed = false;
    if (selectedShape) {
        selectedShape->setIsSelected(false);
        selectedShape->endDrag();
        selectedShape = nullptr;
    }
}

void PhysicsEngine::handleMouseMove(const glm::vec2& mousePos) {
    mousePosition = screenToWorld(mousePos);
    if (mousePressed && selectedShape) {
        selectedShape->updateDrag(mousePosition);
    }
}

glm::vec2 PhysicsEngine::screenToWorld(const glm::vec2& screenPos) const {
    // Convert screen coordinates to world coordinates
    // Both screen and world use top-left origin with glOrtho(0, width, height, 0, -1, 1)
    // So no conversion is needed
    return screenPos;
}

glm::vec2 PhysicsEngine::worldToScreen(const glm::vec2& worldPos) const {
    // Convert world coordinates to screen coordinates
    // Both use the same coordinate system
    return worldPos;
}

// Optimization Methods Implementation

void PhysicsEngine::updateSpatialPartitioning() {
    // Clear existing grid
    spatialGrid.clear();
    
    // Create adaptive grid based on object distribution
    spatialGrid = createAdaptiveGrid();
    
    // Assign objects to cells
    for (auto& shape : shapes) {
        glm::vec2 pos = shape->getPosition();
        float radius = shape->getBoundingRadius();
        
        for (auto& cell : spatialGrid) {
            if (cell.overlaps(pos, radius)) {
                cell.objects.push_back(shape.get());
            }
        }
    }
    
    // Calculate energy density for each cell
    for (auto& cell : spatialGrid) {
        cell.energyDensity = calculateEnergyDensity(cell);
        cell.lastUpdateFrame = currentFrame;
    }
}

std::vector<SpatialCell> PhysicsEngine::createAdaptiveGrid() {
    std::vector<SpatialCell> grid;
    
    if (shapes.empty()) {
        // Default grid if no objects
        float cellSize = 100.0f;
        for (int x = 0; x < worldBounds.x; x += cellSize) {
            for (int y = 0; y < worldBounds.y; y += cellSize) {
                grid.emplace_back(
                    glm::vec2(x, y),
                    glm::vec2(x + cellSize, y + cellSize)
                );
            }
        }
        return grid;
    }
    
    // Calculate optimal cell size based on object density
    float totalArea = worldBounds.x * worldBounds.y;
    float objectArea = shapes.size() * 50.0f * 50.0f; // Approximate object area
    float density = objectArea / totalArea;
    
    // Adaptive cell size: smaller cells for higher density
    float cellSize = std::max(50.0f, std::min(200.0f, 100.0f / (1.0f + density * 10.0f)));
    
    // Create grid
    for (int x = 0; x < worldBounds.x; x += static_cast<int>(cellSize)) {
        for (int y = 0; y < worldBounds.y; y += static_cast<int>(cellSize)) {
            grid.emplace_back(
                glm::vec2(x, y),
                glm::vec2(x + cellSize, y + cellSize)
            );
        }
    }
    
    return grid;
}

float PhysicsEngine::calculateEnergyDensity(const SpatialCell& cell) {
    if (cell.objects.empty()) return 0.0f;
    
    float totalEnergy = 0.0f;
    for (auto obj : cell.objects) {
        glm::vec2 vel = obj->getVelocity();
        float kineticEnergy = 0.5f * obj->getMass() * glm::dot(vel, vel);
        totalEnergy += kineticEnergy;
    }
    
    float cellArea = (cell.max.x - cell.min.x) * (cell.max.y - cell.min.y);
    return totalEnergy / cellArea;
}

void PhysicsEngine::updateNeighborTracking() {
    // Clear old neighbor tracking
    neighborTracking.clear();
    
    // Create neighbor tracking for each object
    for (auto& shape : shapes) {
        NeighborInfo neighborInfo(shape.get());
        
        // Find neighbors within maxNeighborDistance
        for (auto& other : shapes) {
            if (shape.get() == other.get()) continue;
            
            float distance = glm::distance(shape->getPosition(), other->getPosition());
            if (distance <= maxNeighborDistance) {
                neighborInfo.neighbors.push_back(other.get());
            }
        }
        
        neighborInfo.lastUpdateFrame = currentFrame;
        neighborTracking.push_back(neighborInfo);
    }
}

void PhysicsEngine::predictCollisions() {
    collisionPredictions.clear();
    
    // Only predict for high-energy objects or dense areas
    for (auto& neighborInfo : neighborTracking) {
        if (neighborInfo.neighbors.size() > 3) { // Only if object has many neighbors
            for (auto neighbor : neighborInfo.neighbors) {
                float predictedTime = predictCollisionTime(neighborInfo.object, neighbor);
                bool willCollide = willCollideInTimeframe(neighborInfo.object, neighbor, 0.1f); // 100ms timeframe
                
                if (willCollide) {
                    collisionPredictions.emplace_back(neighborInfo.object, neighbor, predictedTime, true);
                }
            }
        }
    }
}

float PhysicsEngine::predictCollisionTime(Shape* a, Shape* b) {
    glm::vec2 posA = a->getPosition();
    glm::vec2 posB = b->getPosition();
    glm::vec2 velA = a->getVelocity();
    glm::vec2 velB = b->getVelocity();
    
    glm::vec2 relPos = posB - posA;
    glm::vec2 relVel = velB - velA;
    
    float radiusSum = a->getBoundingRadius() + b->getBoundingRadius();
    
    // Quadratic equation for collision time
    float a_coeff = glm::dot(relVel, relVel);
    float b_coeff = 2.0f * glm::dot(relPos, relVel);
    float c_coeff = glm::dot(relPos, relPos) - radiusSum * radiusSum;
    
    float discriminant = b_coeff * b_coeff - 4.0f * a_coeff * c_coeff;
    
    if (discriminant < 0) return -1.0f; // No collision
    
    float t1 = (-b_coeff - sqrt(discriminant)) / (2.0f * a_coeff);
    float t2 = (-b_coeff + sqrt(discriminant)) / (2.0f * a_coeff);
    
    return (t1 >= 0) ? t1 : t2;
}

bool PhysicsEngine::willCollideInTimeframe(Shape* a, Shape* b, float timeframe) {
    float collisionTime = predictCollisionTime(a, b);
    return collisionTime >= 0 && collisionTime <= timeframe;
}

std::vector<Shape*> PhysicsEngine::getPotentialCollisions(Shape* shape) {
    std::vector<Shape*> potentialCollisions;
    
    // First check neighbor tracking
    for (auto& neighborInfo : neighborTracking) {
        if (neighborInfo.object == shape) {
            potentialCollisions = neighborInfo.neighbors;
            break;
        }
    }
    
    // If no neighbors found, use spatial grid
    if (potentialCollisions.empty()) {
        glm::vec2 pos = shape->getPosition();
        float radius = shape->getBoundingRadius();
        
        for (auto& cell : spatialGrid) {
            if (cell.overlaps(pos, radius)) {
                for (auto obj : cell.objects) {
                    if (obj != shape) {
                        potentialCollisions.push_back(obj);
                    }
                }
            }
        }
    }
    
    return potentialCollisions;
}

bool PhysicsEngine::shouldUpdateNeighbors(Shape* shape) {
    // Update neighbors if object has moved significantly
    glm::vec2 pos = shape->getPosition();
    glm::vec2 vel = shape->getVelocity();
    
    // Check if velocity is high enough to warrant update
    float speed = glm::length(vel);
    return speed > 50.0f; // Update if moving faster than 50 units/frame
}

bool PhysicsEngine::shouldUpdateSpatialGrid() {
    return currentFrame % spatialUpdateInterval == 0;
}

void PhysicsEngine::optimizeCollisionDetection() {
    // Use optimized collision detection instead of O(n²)
    for (auto& shape : shapes) {
        if (shape->getPhysics().isStatic) continue;
        
        // Get potential collisions using spatial partitioning
        auto potentialCollisions = getPotentialCollisions(shape.get());
        
        for (auto other : potentialCollisions) {
            if (other->getPhysics().isStatic) continue;
            
            collisionChecksThisFrame++;
            
            // Check if collision prediction exists
            bool predictedCollision = false;
            for (auto& prediction : collisionPredictions) {
                if ((prediction.obj1 == shape.get() && prediction.obj2 == other) ||
                    (prediction.obj1 == other && prediction.obj2 == shape.get())) {
                    predictedCollision = prediction.willCollide;
                    break;
                }
            }
            
            // Only do detailed collision check if prediction suggests collision
            if (predictedCollision || currentFrame % 5 == 0) { // Or every 5 frames
                CollisionInfo collision = detectCollision(shape.get(), other);
                if (collision.shapeA != nullptr) {
                    actualCollisionsThisFrame++;
                    collision.shapeA->resolveCollision(collision.shapeB, collision.normal, collision.penetration);
                }
            }
        }
    }
    
    // Handle world bounds
    for (auto& shape : shapes) {
        if (shape->getPhysics().isStatic) continue;
        
        glm::vec2 pos = shape->getPosition();
        glm::vec2 vel = shape->getVelocity();
        float radius = shape->getBoundingRadius();
        
        // Check horizontal bounds
        if (pos.x - radius < 0) {
            pos.x = radius;
            vel.x = -vel.x * shape->getPhysics().restitution;
        } else if (pos.x + radius > worldBounds.x) {
            pos.x = worldBounds.x - radius;
            vel.x = -vel.x * shape->getPhysics().restitution;
        }
        
        // Check vertical bounds
        if (pos.y - radius < 0) {
            pos.y = radius;
            vel.y = -vel.y * shape->getPhysics().restitution;
        } else if (pos.y + radius > worldBounds.y) {
            pos.y = worldBounds.y - radius;
            vel.y = -vel.y * shape->getPhysics().restitution;
        }
        
        shape->setPosition(pos);
        shape->setVelocity(vel);
    }
}

// setGravity and setWorldBounds methods removed - using inline versions from header 
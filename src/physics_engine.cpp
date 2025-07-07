#include "physics_engine.h"
#include "config.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <glm/gtx/rotate_vector.hpp>
#include <unordered_set>
#include <unordered_map>

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

// Quadtree Implementation
Quadtree::Quadtree(const glm::vec2& center, float width, float height, int lvl)
    : bounds(center, width, height), level(lvl) {
    objects.reserve(MAX_OBJECTS);
}

void Quadtree::clear() {
    objects.clear();
    children.clear();
}

void Quadtree::split() {
    float subWidth = bounds.halfWidth;
    float subHeight = bounds.halfHeight;
    float x = bounds.center.x;
    float y = bounds.center.y;
    
    children.push_back(std::make_unique<Quadtree>(glm::vec2(x - subWidth, y - subHeight), subWidth * 2, subHeight * 2, level + 1));
    children.push_back(std::make_unique<Quadtree>(glm::vec2(x + subWidth, y - subHeight), subWidth * 2, subHeight * 2, level + 1));
    children.push_back(std::make_unique<Quadtree>(glm::vec2(x - subWidth, y + subHeight), subWidth * 2, subHeight * 2, level + 1));
    children.push_back(std::make_unique<Quadtree>(glm::vec2(x + subWidth, y + subHeight), subWidth * 2, subHeight * 2, level + 1));
}

int Quadtree::getIndex(const glm::vec2& point) const {
    int index = -1;
    bool topQuadrant = point.y > bounds.center.y;
    bool rightQuadrant = point.x > bounds.center.x;
    
    if (rightQuadrant) {
        if (topQuadrant) index = 3;
        else index = 1;
    } else {
        if (topQuadrant) index = 2;
        else index = 0;
    }
    
    return index;
}

void Quadtree::insert(Shape* object) {
    if (!children.empty()) {
        int index = getIndex(object->getPosition());
        if (index != -1) {
            children[index]->insert(object);
            return;
        }
    }
    
    objects.push_back(object);
    
    if (objects.size() > MAX_OBJECTS && level < MAX_LEVELS) {
        if (children.empty()) {
            split();
        }
        
        // Redistribute objects to children
        for (auto it = objects.begin(); it != objects.end();) {
            int index = getIndex((*it)->getPosition());
            if (index != -1) {
                children[index]->insert(*it);
                it = objects.erase(it);
            } else {
                ++it;
            }
        }
    }
}

std::vector<Shape*> Quadtree::retrieve(const glm::vec2& point, float radius) {
    std::vector<Shape*> result;
    QuadtreeBounds searchBounds(point, radius * 2, radius * 2);
    return retrieve(searchBounds);
}

std::vector<Shape*> Quadtree::retrieve(const QuadtreeBounds& bounds) {
    std::vector<Shape*> result;
    
    if (!this->bounds.intersects(bounds)) {
        return result;
    }
    
    // Add objects in this node
    for (auto obj : objects) {
        result.push_back(obj);
    }
    
    // Add objects from children
    for (auto& child : children) {
        auto childObjects = child->retrieve(bounds);
        result.insert(result.end(), childObjects.begin(), childObjects.end());
    }
    
    return result;
}

void Quadtree::remove(Shape* object) {
    auto it = std::find(objects.begin(), objects.end(), object);
    if (it != objects.end()) {
        objects.erase(it);
    }
    
    for (auto& child : children) {
        child->remove(object);
    }
}

void Quadtree::update() {
    // Remove all objects and reinsert them
    std::vector<Shape*> allObjects = objects;
    for (auto& child : children) {
        auto childObjects = child->retrieve(static_cast<const QuadtreeBounds&>(child->bounds));
        allObjects.insert(allObjects.end(), childObjects.begin(), childObjects.end());
    }
    
    clear();
    for (auto obj : allObjects) {
        insert(obj);
    }
}

// ParticlePool Implementation
ParticlePool::ParticlePool(size_t size) : poolSize(size), freeList(nullptr) {
    resize(size);
}

Shape* ParticlePool::acquire() {
    if (freeList) {
        ParticleNode* node = freeList;
        freeList = freeList->next;
        node->active = true;
        return node->particle;
    }
    return nullptr;
}

void ParticlePool::release(Shape* particle) {
    for (auto& node : nodes) {
        if (node->particle == particle) {
            node->active = false;
            node->next = freeList;
            freeList = node.get();
            break;
        }
    }
}

void ParticlePool::resize(size_t newSize) {
    poolSize = newSize;
    nodes.clear();
    nodes.reserve(newSize);
    
    // Create new nodes
    for (size_t i = 0; i < newSize; ++i) {
        nodes.push_back(std::make_unique<ParticleNode>());
    }
    
    // Build free list
    freeList = nodes[0].get();
    for (size_t i = 0; i < newSize - 1; ++i) {
        nodes[i]->next = nodes[i + 1].get();
    }
    nodes[newSize - 1]->next = nullptr;
}

size_t ParticlePool::getActiveCount() const {
    size_t count = 0;
    for (const auto& node : nodes) {
        if (node->active) count++;
    }
    return count;
}

// BroadPhaseDetector Implementation
void BroadPhaseDetector::update(const std::vector<std::unique_ptr<Shape>>& objects) {
    potentialPairs.clear();

    if (objects.size() <= 200) {
        // O(n^2) for small numbers
        for (size_t i = 0; i < objects.size(); ++i) {
            for (size_t j = i + 1; j < objects.size(); ++j) {
                if (objects[i] && objects[j]) {
                    potentialPairs.emplace_back(objects[i].get(), objects[j].get());
                }
            }
        }
        return;
    }

    // --- Spatial grid ---
    const float cellSize = 100.0f; // Adjust as needed for the typical object size
    std::unordered_map<std::pair<int, int>, std::vector<Shape*>, GridHash> grid;

    // Assign objects to grid cells
    for (const auto& obj : objects) {
        if (!obj) continue;
        BoundingBox bbox = obj->getBoundingBox();
        int minX = static_cast<int>(bbox.min.x / cellSize);
        int maxX = static_cast<int>(bbox.max.x / cellSize);
        int minY = static_cast<int>(bbox.min.y / cellSize);
        int maxY = static_cast<int>(bbox.max.y / cellSize);

        for (int x = minX; x <= maxX; ++x) {
            for (int y = minY; y <= maxY; ++y) {
                grid[{x, y}].push_back(obj.get());
            }
        }
    }

    // Generate unique unordered pairs from each cell
    std::unordered_set<std::pair<Shape*, Shape*>, PairHash> uniquePairs;
    for (const auto& cell : grid) {
        const auto& cellObjects = cell.second;
        for (size_t i = 0; i < cellObjects.size(); ++i) {
            for (size_t j = i + 1; j < cellObjects.size(); ++j) {
                Shape* a = cellObjects[i];
                Shape* b = cellObjects[j];
                if (a > b) std::swap(a, b); // Ensure (a,b) == (b,a)
                uniquePairs.emplace(a, b);
            }
        }
    }

    // Copy to vector
    potentialPairs.reserve(uniquePairs.size());
    for (const auto& pair : uniquePairs) {
        potentialPairs.push_back(pair);
    }
}

void BroadPhaseDetector::clear() {
    potentialPairs.clear();
}

// PhysicsEngine Implementation
PhysicsEngine::PhysicsEngine(int width, int height) 
    : worldGravity(0.0f, 981.0f), worldBounds(width, height), timeStep(1.0f/60.0f), iterations(4),
      selectedShape(nullptr), mousePressed(false), showSpatialGrid(false),
      currentFrame(0), neighborUpdateInterval(10), spatialUpdateInterval(5),
      maxNeighborDistance(100.0f), targetCellsPerObject(5), energyThreshold(0.1f),
      collisionChecksThisFrame(0), actualCollisionsThisFrame(0), averageCollisionTime(0.0f),
      useMultiThreading(false), threadCount(4), useSIMD(true), // Disable multi-threading for now
      cameraPosition(width * 0.5f, height * 0.5f) {
    
    // Initialize advanced optimization structures
    quadtree = std::make_unique<Quadtree>(glm::vec2(worldBounds.x * 0.5f, worldBounds.y * 0.5f), 
                                         worldBounds.x, worldBounds.y);
    particlePool = std::make_unique<ParticlePool>(1000);
    broadPhase = std::make_unique<BroadPhaseDetector>();
    renderer = std::make_unique<ModernRenderer>();
    
    // Initialize LOD levels
    lodLevels.emplace_back(100.0f, 1.0f, true, true);   // Close objects: full detail
    lodLevels.emplace_back(300.0f, 2.0f, true, false);  // Medium distance: no detailed collision
    lodLevels.emplace_back(600.0f, 4.0f, false, false); // Far objects: minimal detail
    
    // Initialize legacy spatial grid
    updateSpatialPartitioning();
}

PhysicsEngine::~PhysicsEngine() {
    // Wait for worker threads to finish
    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
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
    
    // Simple physics update - update all non-static shapes
    for (auto& shape : shapes) {
        if (!shape->getPhysics().isStatic) {
            // Apply gravity
            glm::vec2 gravity = worldGravity;
            if (shape->getUseGlobalGravity()) {
                gravity = worldGravity;
            } else {
                gravity = glm::vec2(0.0f, shape->getGravity());
            }
            
            // Update velocity with gravity
            glm::vec2 velocity = shape->getVelocity();
            velocity += gravity * deltaTime;
            shape->setVelocity(velocity);
            
            // Update position
            glm::vec2 position = shape->getPosition();
            position += velocity * deltaTime;
            shape->setPosition(position);
            
            // --- Synchronize Shape's own members with physics struct ---
            shape->setInternalPosition(shape->getPhysics().position);
            shape->setInternalVelocity(shape->getPhysics().velocity);
        }
    }
    
    // Handle boundary collisions
    for (auto& shape : shapes) {
        if (!shape->getPhysics().isStatic) {
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
            
            // Synchronize internal members after boundary correction
            shape->setInternalPosition(shape->getPhysics().position);
            shape->setInternalVelocity(shape->getPhysics().velocity);
        }
    }
    
    // Update advanced optimization systems
    updateQuadtree();
    updateBroadPhase();
    updateLOD();
    
    // Update legacy optimization systems (for compatibility)
    if (shouldUpdateSpatialGrid()) {
        updateSpatialPartitioning();
    }
    
    if (currentFrame % neighborUpdateInterval == 0) {
        updateNeighborTracking();
    }
    
    // Predict collisions for next few frames
    predictCollisions();
    
    // Process collisions
    processCollisionsMultiThreaded();
    
    // Update average collision time
    if (actualCollisionsThisFrame > 0) {
        averageCollisionTime = (averageCollisionTime * 0.9f) + (deltaTime * 0.1f);
    }
}

void PhysicsEngine::updateQuadtree() {
    quadtree->clear();
    for (auto& shape : shapes) {
        quadtree->insert(shape.get());
    }
}

void PhysicsEngine::updateBroadPhase() {
    broadPhase->update(shapes);
}

void PhysicsEngine::updateLOD() {
    for (auto& shape : shapes) {
        float distance = glm::distance(shape->getPosition(), cameraPosition);
        
        // Find appropriate LOD level
        for (const auto& lod : lodLevels) {
            if (distance <= lod.distance) {
                // Apply LOD settings
                shape->setLODLevel(lod);
                break;
            }
        }
    }
}

void PhysicsEngine::processCollisionsMultiThreaded() {
    // Fast path: for small numbers of objects, do direct O(n^2) collision checks
    if (shapes.size() <= 200) {
        collisions.clear();
        for (size_t i = 0; i < shapes.size(); ++i) {
            for (size_t j = i + 1; j < shapes.size(); ++j) {
                Shape* a = shapes[i].get();
                Shape* b = shapes[j].get();
                if (a && b) {
                    glm::vec2 normal;
                    float penetration;
                    collisionChecksThisFrame++;
                    if (checkCollision(a, b, normal, penetration)) {
                        actualCollisionsThisFrame++;
                        collisions.emplace_back(a, b, normal, penetration);
                    }
                }
            }
        }
        resolveCollisions();
        return;
    }
    // Otherwise, use broad phase as before
    const auto& potentialPairs = broadPhase->getPotentialPairs();
    if (useMultiThreading && threadCount > 1) {
        // Multi-threaded collision processing
        std::vector<std::vector<CollisionInfo>> threadCollisions(threadCount);
        std::vector<std::thread> collisionThreads;
        size_t pairsPerThread = potentialPairs.size() / threadCount;
        for (int i = 0; i < threadCount; ++i) {
            size_t start = i * pairsPerThread;
            size_t end = (i == threadCount - 1) ? potentialPairs.size() : (i + 1) * pairsPerThread;
            collisionThreads.emplace_back([this, &potentialPairs, &threadCollisions, i, start, end]() {
                for (size_t j = start; j < end; ++j) {
                    const auto& pair = potentialPairs[j];
                    if (pair.first && pair.second) {
                        glm::vec2 normal;
                        float penetration;
                        collisionChecksThisFrame++;
                        if (checkCollision(pair.first, pair.second, normal, penetration)) {
                            actualCollisionsThisFrame++;
                            threadCollisions[i].emplace_back(pair.first, pair.second, normal, penetration);
                        }
                    }
                }
            });
        }
        for (auto& thread : collisionThreads) {
            thread.join();
        }
        collisions.clear();
        for (const auto& threadCollision : threadCollisions) {
            collisions.insert(collisions.end(), threadCollision.begin(), threadCollision.end());
        }
    } else {
        collisions.clear();
        for (const auto& pair : potentialPairs) {
            if (pair.first && pair.second) {
                glm::vec2 normal;
                float penetration;
                collisionChecksThisFrame++;
                if (checkCollision(pair.first, pair.second, normal, penetration)) {
                    actualCollisionsThisFrame++;
                    collisions.emplace_back(pair.first, pair.second, normal, penetration);
                }
            }
        }
    }
    resolveCollisions();
}

void PhysicsEngine::updatePhysicsMultiThreaded(float deltaTime) {
    if (shapes.empty()) return;
    
    // Clear existing threads
    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    workerThreads.clear();
    
    // Split shapes among threads
    size_t shapesPerThread = shapes.size() / threadCount;
    
    for (int i = 0; i < threadCount; ++i) {
        size_t start = i * shapesPerThread;
        size_t end = (i == threadCount - 1) ? shapes.size() : (i + 1) * shapesPerThread;
        
        workerThreads.emplace_back([this, start, end, deltaTime]() {
            for (size_t j = start; j < end; ++j) {
                if (!shapes[j]->getPhysics().isStatic) {
                    shapes[j]->update(deltaTime);
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : workerThreads) {
        thread.join();
    }
}

// Particle pool management methods
size_t PhysicsEngine::getParticlePoolSize() const {
    return particlePool ? particlePool->getPoolSize() : 0;
}

size_t PhysicsEngine::getActiveParticleCount() const {
    return particlePool ? particlePool->getActiveCount() : 0;
}

void PhysicsEngine::resizeParticlePool(size_t size) {
    if (particlePool) {
        particlePool->resize(size);
    }
}

bool PhysicsEngine::initializeRenderer(int width, int height) {
    if (!renderer) {
        renderer = std::make_unique<ModernRenderer>();
    }
    
    if (!renderer->initialize()) {
        std::cerr << "Failed to initialize modern renderer" << std::endl;
        return false;
    }
    
    renderer->setupProjection(width, height);
    std::cout << "Modern GPU renderer initialized for " << width << "x" << height << std::endl;
    return true;
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
    if (!renderer) return;
    
    renderer->beginFrame();
    
    // Add all shapes to the renderer for batch rendering
    for (const auto& shape : shapes) {
        glm::vec2 position = shape->getPosition();
        glm::vec3 color = shape->getColor();
        float rotation = shape->getRotation();
        
        switch (shape->getType()) {
            case ShapeType::CIRCLE: {
                const Circle* circle = static_cast<const Circle*>(shape.get());
                renderer->addCircle(position, circle->getRadius(), color, rotation);
                break;
            }
            case ShapeType::RECTANGLE: {
                const Rectangle* rect = static_cast<const Rectangle*>(shape.get());
                renderer->addRectangle(position, rect->getWidth(), rect->getHeight(), color, rotation);
                break;
            }
            case ShapeType::TRIANGLE: {
                const Triangle* tri = static_cast<const Triangle*>(shape.get());
                renderer->addTriangle(position, tri->getSideLength(), color, rotation);
                break;
            }
            default:
                break;
        }
    }
    
    renderer->renderAllShapes();
}

void PhysicsEngine::renderVelocityVectors() const {
    glUseProgram(0);

    // Set up legacy orthographic projection for immediate mode
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, worldBounds.x, worldBounds.y, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    int vectorsRendered = 0;
    for (const auto& shape : shapes) {
        glm::vec2 velocity = shape->getVelocity();
        float speed = glm::length(velocity);
        if (speed > 0.1f) {
            glm::vec2 position = shape->getPosition();
            glm::vec2 direction = glm::normalize(velocity);
            float maxLength = 120.0f;
            float length = std::min(speed * 6.0f, maxLength);
            glm::vec2 endPoint = position + direction * length;
            glColor3f(1.0f, 1.0f, 0.0f);
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glVertex2f(position.x, position.y);
            glVertex2f(endPoint.x, endPoint.y);
            glEnd();
            float arrowSize = 12.0f;
            glm::vec2 arrowDir1 = rotate2D(direction, -0.3f) * arrowSize;
            glm::vec2 arrowDir2 = rotate2D(direction, 0.3f) * arrowSize;
            glBegin(GL_TRIANGLES);
            glVertex2f(endPoint.x, endPoint.y);
            glVertex2f(endPoint.x - arrowDir1.x, endPoint.y - arrowDir1.y);
            glVertex2f(endPoint.x - arrowDir2.x, endPoint.y - arrowDir2.y);
            glEnd();
            glLineWidth(1.0f);
            vectorsRendered++;
        }
    }

    // Restore matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    if (renderer) {
        glUseProgram(renderer->getShaderProgram());
    }
}

void PhysicsEngine::renderSpatialGrid() const {
    if (!showSpatialGrid || shapes.size() <= 200) return;

    glUseProgram(0);

    // Set up legacy orthographic projection for immediate mode
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, worldBounds.x, worldBounds.y, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    const float cellSize = 100.0f;
    std::unordered_map<std::pair<int, int>, std::vector<Shape*>, GridHash> grid;
    for (const auto& obj : shapes) {
        if (!obj) continue;
        BoundingBox bbox = obj->getBoundingBox();
        int minX = static_cast<int>(bbox.min.x / cellSize);
        int maxX = static_cast<int>(bbox.max.x / cellSize);
        int minY = static_cast<int>(bbox.min.y / cellSize);
        int maxY = static_cast<int>(bbox.max.y / cellSize);
        for (int x = minX; x <= maxX; ++x) {
            for (int y = minY; y <= maxY; ++y) {
                grid[{x, y}].push_back(obj.get());
            }
        }
    }
    glColor3f(0.3f, 0.3f, 0.8f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    int minGridX = INT_MAX, maxGridX = INT_MIN;
    int minGridY = INT_MAX, maxGridY = INT_MIN;
    for (const auto& cell : grid) {
        minGridX = std::min(minGridX, cell.first.first);
        maxGridX = std::max(maxGridX, cell.first.first);
        minGridY = std::min(minGridY, cell.first.second);
        maxGridY = std::max(maxGridY, cell.first.second);
    }
    for (int x = minGridX; x <= maxGridX + 1; ++x) {
        float worldX = static_cast<float>(x) * cellSize;
        glVertex2f(worldX, static_cast<float>(minGridY) * cellSize);
        glVertex2f(worldX, static_cast<float>(maxGridY + 1) * cellSize);
    }
    for (int y = minGridY; y <= maxGridY + 1; ++y) {
        float worldY = static_cast<float>(y) * cellSize;
        glVertex2f(static_cast<float>(minGridX) * cellSize, worldY);
        glVertex2f(static_cast<float>(maxGridX + 1) * cellSize, worldY);
    }
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    for (const auto& cell : grid) {
        if (cell.second.size() > 1) {
            float cellX = static_cast<float>(cell.first.first) * cellSize + cellSize * 0.5f;
            float cellY = static_cast<float>(cell.first.second) * cellSize + cellSize * 0.5f;
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(cellX, cellY);
            for (int i = 0; i <= 16; ++i) {
                float angle = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / 16.0f;
                glVertex2f(cellX + 8.0f * cos(angle), cellY + 8.0f * sin(angle));
            }
            glEnd();
        }
    }

    // Restore matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    if (renderer) {
        glUseProgram(renderer->getShaderProgram());
    }
}

void PhysicsEngine::handleMousePress(const glm::vec2& mousePos) {
    mousePosition = screenToWorld(mousePos);
    mousePressed = true;
    
    // Find shape under mouse
    selectedShape = nullptr;
    
    for (auto& shape : shapes) {
        if (shape->containsPoint(mousePosition)) {
            selectedShape = shape.get();
            selectedShape->setIsSelected(true);
            selectedShape->startDrag(mousePosition);
            break;
        }
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
        for (int x = 0; x < static_cast<int>(worldBounds.x); x += static_cast<int>(cellSize)) {
            for (int y = 0; y < static_cast<int>(worldBounds.y); y += static_cast<int>(cellSize)) {
                grid.emplace_back(
                    glm::vec2(static_cast<float>(x), static_cast<float>(y)),
                    glm::vec2(static_cast<float>(x) + cellSize, static_cast<float>(y) + cellSize)
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
    for (int x = 0; x < static_cast<int>(worldBounds.x); x += static_cast<int>(cellSize)) {
        for (int y = 0; y < static_cast<int>(worldBounds.y); y += static_cast<int>(cellSize)) {
            grid.emplace_back(
                glm::vec2(static_cast<float>(x), static_cast<float>(y)),
                glm::vec2(static_cast<float>(x) + cellSize, static_cast<float>(y) + cellSize)
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

// setGravity and setWorldBounds methods removed - using inline versions from header 
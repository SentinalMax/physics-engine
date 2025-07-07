#pragma once
#include "shapes.h"
#include "renderer.h"
#include <vector>
#include <memory>
#include <functional>
#include <glm/glm.hpp>
#include <unordered_map>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>

// SIMD optimization support
#ifdef _MSC_VER
#include <intrin.h>
#endif

// Forward declarations
struct LODLevel;
struct CollisionInfo {
    Shape* shapeA;
    Shape* shapeB;
    glm::vec2 normal;
    float penetration;
    
    CollisionInfo(Shape* a, Shape* b, const glm::vec2& n, float p)
        : shapeA(a), shapeB(b), normal(n), penetration(p) {}
};

// Hash function for Shape* pairs
struct PairHash {
    std::size_t operator()(const std::pair<Shape*, Shape*>& p) const {
        return std::hash<Shape*>()(p.first) ^ (std::hash<Shape*>()(p.second) << 1);
    }
};

// Hash function for grid coordinates
struct GridHash {
    std::size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};

// Forward declarations
class Shape;

// Advanced Quadtree for hierarchical spatial partitioning
class Quadtree {
private:
    static const int MAX_OBJECTS = 10;
    static const int MAX_LEVELS = 8;
    
    struct QuadtreeBounds {
        glm::vec2 center;
        float halfWidth;
        float halfHeight;
        
        QuadtreeBounds(const glm::vec2& c, float w, float h) 
            : center(c), halfWidth(w * 0.5f), halfHeight(h * 0.5f) {}
        
        bool contains(const glm::vec2& point) const {
            return point.x >= center.x - halfWidth && point.x <= center.x + halfWidth &&
                   point.y >= center.y - halfHeight && point.y <= center.y + halfHeight;
        }
        
        bool intersects(const QuadtreeBounds& other) const {
            return !(center.x - halfWidth > other.center.x + other.halfWidth ||
                    center.x + halfWidth < other.center.x - other.halfWidth ||
                    center.y - halfHeight > other.center.y + other.halfHeight ||
                    center.y + halfHeight < other.center.y - other.halfHeight);
        }
    };
    
    QuadtreeBounds bounds;
    std::vector<Shape*> objects;
    std::vector<std::unique_ptr<Quadtree>> children;
    int level;
    
public:
    Quadtree(const glm::vec2& center, float width, float height, int lvl = 0);
    ~Quadtree() = default;
    
    void clear();
    void split();
    int getIndex(const glm::vec2& point) const;
    void insert(Shape* object);
    std::vector<Shape*> retrieve(const glm::vec2& point, float radius);
    std::vector<Shape*> retrieve(const QuadtreeBounds& bounds);
    void remove(Shape* object);
    void update();
};

// Particle Pool for efficient memory management
class ParticlePool {
private:
    struct ParticleNode {
        Shape* particle;
        bool active;
        ParticleNode* next;
        
        ParticleNode() : particle(nullptr), active(false), next(nullptr) {}
    };
    
    std::vector<std::unique_ptr<ParticleNode>> nodes;
    ParticleNode* freeList;
    size_t poolSize;
    
public:
    ParticlePool(size_t size = 1000);
    ~ParticlePool() = default;
    
    Shape* acquire();
    void release(Shape* particle);
    void resize(size_t newSize);
    size_t getActiveCount() const;
    size_t getPoolSize() const { return poolSize; }
};

// Broad Phase Collision Detection using Sweep and Prune
class BroadPhaseDetector {
private:
    struct AxisProjection {
        Shape* object;
        float min, max;
        bool isStart;
        
        AxisProjection(Shape* obj, float mn, float mx, bool start) 
            : object(obj), min(mn), max(mx), isStart(start) {}
    };
    
    std::vector<AxisProjection> xAxis;
    std::vector<AxisProjection> yAxis;
    std::vector<std::pair<Shape*, Shape*>> potentialPairs;
    
public:
    BroadPhaseDetector() = default;
    ~BroadPhaseDetector() = default;
    
    void update(const std::vector<std::unique_ptr<Shape>>& objects);
    const std::vector<std::pair<Shape*, Shape*>>& getPotentialPairs() const { return potentialPairs; }
    void clear();
};

// Spatial partitioning cell (legacy - kept for compatibility)
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
    
    // Advanced optimization structures
    std::unique_ptr<Quadtree> quadtree;
    std::unique_ptr<ParticlePool> particlePool;
    std::unique_ptr<BroadPhaseDetector> broadPhase;
    std::unique_ptr<ModernRenderer> renderer;
    
    // Legacy optimization structures (for compatibility)
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
    
    // Multi-threading support
    std::atomic<bool> useMultiThreading;
    std::vector<std::thread> workerThreads;
    std::mutex physicsMutex;
    int threadCount;
    
    // LOD system
    std::vector<LODLevel> lodLevels;
    glm::vec2 cameraPosition;
    
    // SIMD optimization flags
    bool useSIMD;
    
    // Visualization flags
    bool showSpatialGrid;
    
    // Collision detection
    bool checkCollision(const Shape* a, const Shape* b, glm::vec2& normal, float& penetration) const;
    CollisionInfo detectCollision(Shape* a, Shape* b);
    void resolveCollisions();
    
    // Utility functions
    glm::vec2 screenToWorld(const glm::vec2& screenPos) const;
    glm::vec2 worldToScreen(const glm::vec2& worldPos) const;
    
    // Advanced optimization methods
    void updateQuadtree();
    void updateBroadPhase();
    void processCollisionsMultiThreaded();
    void updatePhysicsMultiThreaded(float deltaTime);
    void updateLOD();
    
    // Legacy optimization methods (kept for compatibility)
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
    // Constructor and destructor
    PhysicsEngine(int width = 800, int height = 600);
    ~PhysicsEngine();
    
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
    void renderSpatialGrid() const;
    
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
    
    // Advanced optimization control
    void setMultiThreading(bool enabled) { useMultiThreading = enabled; }
    void setThreadCount(int count) { threadCount = count; }
    void setSIMD(bool enabled) { useSIMD = enabled; }
    void setCameraPosition(const glm::vec2& pos) { cameraPosition = pos; }
    glm::vec2 getCameraPosition() const { return cameraPosition; }
    
    // Visualization control
    void setShowSpatialGrid(bool show) { showSpatialGrid = show; }
    bool getShowSpatialGrid() const { return showSpatialGrid; }
    
    // Legacy optimization control
    void setNeighborUpdateInterval(int frames) { neighborUpdateInterval = frames; }
    void setSpatialUpdateInterval(int frames) { spatialUpdateInterval = frames; }
    void setMaxNeighborDistance(float distance) { maxNeighborDistance = distance; }
    void setTargetCellsPerObject(int count) { targetCellsPerObject = count; }
    void setEnergyThreshold(float threshold) { energyThreshold = threshold; }
    
    // Particle pool management
    size_t getParticlePoolSize() const;
    size_t getActiveParticleCount() const;
    void resizeParticlePool(size_t size);
    
    // Rendering
    bool initializeRenderer(int width, int height);
    ModernRenderer* getRenderer() const { return renderer.get(); }
}; 
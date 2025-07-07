#pragma once
#include "physics_engine.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <memory>

// Forward declaration
struct GLFWwindow;

// Spawning modes
enum class SpawnMode {
    SINGLE_CLICK,
    MATRIX,
    GOLDEN_RATIO
};

class UIManager {
private:
    PhysicsEngine* physicsEngine;
    bool showDemoWindow;
    bool showPropertyPanel;
    bool showDebugPanel;
    
    // UI state
    int selectedShapeType;
    float newShapeSize[2];
    float newShapeColor[3];
    float newShapeMass;
    float newShapeGravity;
    float newShapeRestitution;
    
    // Property panel state
    Shape* selectedShape;
    float selectedColor[3];
    float selectedSize[2];
    float selectedMass;
    float selectedGravity;
    float selectedRestitution;
    bool selectedStatic;
    
    // Debug panel state
    float currentFPS;
    int objectCount;
    bool showVelocityVectors;
    int uiScale;
    float smoothedFPS;
    
    // FPS smoothing
    static const int FPS_SAMPLE_COUNT = 60;
    float frameTimes[FPS_SAMPLE_COUNT];
    int frameTimeIndex;
    
    // Enhanced spawn panel state
    SpawnMode spawnMode;
    bool clickToSpawnMode;
    int matrixRows;
    int matrixColumns;
    float matrixSpacing;
    int goldenRatioCount;
    float goldenRatioRadius;
    float goldenRatioSpacing;
    int spawnObjectCount;
    bool useGlobalGravityForNewShapes;
    
    void renderMainMenu();
    void renderPropertyPanel();
    void renderShapeCreation();
    void renderWorldSettings();
    void renderDebugPanel();
    void updateSelectedShapeProperties();

public:
    UIManager(PhysicsEngine* engine);
    ~UIManager() = default;
    
    void render();
    void setSelectedShape(Shape* shape);
    Shape* getSelectedShape() const { return selectedShape; }
    
    // Debug info setters
    void setFPS(float fps) { currentFPS = fps; }
    void setDeltaTime(float deltaTime);
    void setObjectCount(int count) { objectCount = count; }
    
    // Debug info getters
    float getSmoothedFPS() const { return smoothedFPS; }
    bool getShowVelocityVectors() const { return showVelocityVectors; }
    bool getShowSpatialGrid() const { return physicsEngine ? physicsEngine->getShowSpatialGrid() : false; }
    int getUIScale() const { return uiScale; }
    void setUIScale(int scale) { uiScale = scale; }
    
    // Spawn panel methods
    bool isClickToSpawnMode() const { return clickToSpawnMode; }
    void handleClickToSpawn(const glm::vec2& worldPos);
    void spawnSingleObject();
    void spawnMatrix();
    void spawnGoldenRatio();
    
    // Copyright display
    void renderCopyright();
    
    // ImGui initialization
    static bool init(GLFWwindow* window);
    static void shutdown();
    void newFrame();
    static void renderFrame();
    
    // UI state setters
    void setShowVelocityVectors(bool show) { showVelocityVectors = show; }
    void setShowSpatialGrid(bool show) { if (physicsEngine) physicsEngine->setShowSpatialGrid(show); }
    
    // Performance monitoring
}; 
#include "ui_manager.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

UIManager::UIManager(PhysicsEngine* engine) 
    : physicsEngine(engine), showDemoWindow(false), showPropertyPanel(true), showDebugPanel(true),
      selectedShapeType(0), selectedShape(nullptr), currentFPS(0.0f), objectCount(0), showVelocityVectors(false), uiScale(2),
      spawnMode(SpawnMode::SINGLE_CLICK), clickToSpawnMode(false), matrixRows(3), matrixColumns(3), matrixSpacing(50.0f),
      goldenRatioCount(10), goldenRatioRadius(100.0f), goldenRatioSpacing(20.0f), spawnObjectCount(1) {
    
    // Initialize default values
    newShapeSize[0] = 50.0f;
    newShapeSize[1] = 50.0f;
    newShapeColor[0] = 1.0f;
    newShapeColor[1] = 0.5f;
    newShapeColor[2] = 0.2f;
    newShapeMass = 1.0f;
    newShapeGravity = 9.81f;
    newShapeRestitution = 0.8f;
    
    selectedColor[0] = 1.0f;
    selectedColor[1] = 1.0f;
    selectedColor[2] = 1.0f;
    selectedSize[0] = 50.0f;
    selectedSize[1] = 50.0f;
    selectedMass = 1.0f;
    selectedGravity = 9.81f;
    selectedRestitution = 0.8f;
    selectedStatic = false;
}

bool UIManager::init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "Failed to initialize ImGui GLFW implementation" << std::endl;
        return false;
    }
    
    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
        std::cerr << "Failed to initialize ImGui OpenGL3 implementation" << std::endl;
        return false;
    }
    
    return true;
}

void UIManager::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Apply UI scaling - convert integer scale to float
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = static_cast<float>(uiScale);
}

void UIManager::renderFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::render() {
    static int frameCount = 0;
    frameCount++;
    
    // Debug output every 60 frames
    if (frameCount % 60 == 0) {
        std::cout << "UI Manager rendering - Selected shape: " << (selectedShape ? "Yes" : "No") << std::endl;
    }
    
    renderMainMenu();
    renderShapeCreation();
    renderWorldSettings();
    
    // Render property panel last to ensure it stays on top
    renderPropertyPanel();
    
    // Render debug panel
    renderDebugPanel();
    
    // Render copyright at bottom left
    renderCopyright();
    
    if (showDemoWindow) {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
}

void UIManager::renderMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Clear All", "Ctrl+N")) {
                physicsEngine->clearShapes();
                selectedShape = nullptr;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // Handle exit
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Property Panel", nullptr, &showPropertyPanel);
            ImGui::MenuItem("Debug Panel", nullptr, &showDebugPanel);
            ImGui::MenuItem("Demo Window", nullptr, &showDemoWindow);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                ImGui::OpenPopup("About Physics Engine");
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    
    // About popup
    if (ImGui::BeginPopupModal("About Physics Engine", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("2D Physics Engine");
        ImGui::Text("Version 1.0.0");
        ImGui::Separator();
        ImGui::Text("A real-time 2D physics simulation engine");
        ImGui::Text("built with OpenGL, GLFW, and ImGui.");
        ImGui::Separator();
        ImGui::Text("Features:");
        ImGui::BulletText("Rigid body physics simulation");
        ImGui::BulletText("Collision detection and response");
        ImGui::BulletText("Interactive shape creation and editing");
        ImGui::BulletText("Advanced spawning modes (Matrix, Golden Ratio)");
        ImGui::BulletText("Real-time property editing");
        ImGui::BulletText("Velocity visualization");
        ImGui::Separator();
        ImGui::Text("Copyright (c) 2025 Alex's Physics Engine");
        ImGui::Text("All rights reserved.");
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void UIManager::renderPropertyPanel() {
    if (!showPropertyPanel) return;
    
    // Set window position and size
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 300, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Properties", &showPropertyPanel)) {
        if (selectedShape) {
            ImGui::Text("Selected Shape Properties");
            ImGui::Separator();
            
            // Color picker
            if (ImGui::ColorEdit3("Color", selectedColor)) {
                selectedShape->setColor(glm::vec3(selectedColor[0], selectedColor[1], selectedColor[2]));
            }
            
            // Size controls
            switch (selectedShape->getType()) {
                case ShapeType::CIRCLE: {
                    Circle* circle = static_cast<Circle*>(selectedShape);
                    if (ImGui::SliderFloat("Radius", &selectedSize[0], 10.0f, 200.0f)) {
                        circle->setRadius(selectedSize[0]);
                    }
                    break;
                }
                case ShapeType::RECTANGLE: {
                    Rectangle* rect = static_cast<Rectangle*>(selectedShape);
                    if (ImGui::SliderFloat2("Size", selectedSize, 10.0f, 200.0f)) {
                        rect->setWidth(selectedSize[0]);
                        rect->setHeight(selectedSize[1]);
                    }
                    break;
                }
                case ShapeType::TRIANGLE: {
                    Triangle* triangle = static_cast<Triangle*>(selectedShape);
                    if (ImGui::SliderFloat("Side Length", &selectedSize[0], 10.0f, 200.0f)) {
                        triangle->setSideLength(selectedSize[0]);
                    }
                    break;
                }
            }
            
            // Physics properties
            if (ImGui::SliderFloat("Mass", &selectedMass, 0.1f, 10.0f)) {
                selectedShape->setMass(selectedMass);
            }

            bool useGlobalGravity = selectedShape->getUseGlobalGravity();
            if (ImGui::Checkbox("Use Global Gravity", &useGlobalGravity)) {
                selectedShape->setUseGlobalGravity(useGlobalGravity);
                if (useGlobalGravity) {
                    selectedGravity = physicsEngine->getGravity().y;
                    selectedShape->setGravity(selectedGravity);
                }
            }
            ImGui::BeginDisabled(selectedShape->getUseGlobalGravity());
            if (ImGui::SliderFloat("Gravity", &selectedGravity, -50.0f, 50.0f)) {
                selectedShape->setGravity(selectedGravity);
                selectedShape->setUseGlobalGravity(false);
            }
            ImGui::EndDisabled();
            
            if (ImGui::SliderFloat("Restitution", &selectedRestitution, 0.0f, 1.0f)) {
                selectedShape->getPhysics().restitution = selectedRestitution;
            }
            
            if (ImGui::Checkbox("Static", &selectedStatic)) {
                selectedShape->getPhysics().isStatic = selectedStatic;
            }
            
            ImGui::Separator();
            
            // Position display
            glm::vec2 pos = selectedShape->getPosition();
            ImGui::Text("Position: (%.1f, %.1f)", pos.x, pos.y);
            
            glm::vec2 vel = selectedShape->getVelocity();
            ImGui::Text("Velocity: (%.1f, %.1f)", vel.x, vel.y);
            
            // Delete button
            if (ImGui::Button("Delete Shape", ImVec2(-1, 0))) {
                physicsEngine->removeShape(selectedShape);
                selectedShape = nullptr;
            }
        } else {
            ImGui::Text("No shape selected");
            ImGui::Text("Click on a shape to edit its properties");
        }
    }
    ImGui::End();
}

void UIManager::renderShapeCreation() {
    ImGui::SetNextWindowPos(ImVec2(10, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Create Shapes")) {
        // Mode Selection
        ImGui::Text("Spawn Mode:");
        const char* spawnModes[] = { "Single Object", "Matrix", "Golden Ratio" };
        int currentMode = static_cast<int>(spawnMode);
        if (ImGui::Combo("##SpawnMode", &currentMode, spawnModes, IM_ARRAYSIZE(spawnModes))) {
            spawnMode = static_cast<SpawnMode>(currentMode);
            // Reset click mode when changing spawn modes
            if (spawnMode != SpawnMode::SINGLE_CLICK) {
                clickToSpawnMode = false;
            }
        }
        
        ImGui::Separator();
        
        // Shape Type Selection
        ImGui::Text("Shape Type:");
        const char* shapeTypes[] = { "Circle", "Rectangle", "Triangle" };
        ImGui::Combo("##ShapeType", &selectedShapeType, shapeTypes, IM_ARRAYSIZE(shapeTypes));
        
        ImGui::Separator();
        
        // Size controls
        switch (selectedShapeType) {
            case 0: // Circle
                ImGui::SliderFloat("Radius", &newShapeSize[0], 10.0f, 100.0f);
                break;
            case 1: // Rectangle
                ImGui::SliderFloat2("Size", newShapeSize, 10.0f, 100.0f);
                break;
            case 2: // Triangle
                ImGui::SliderFloat("Side Length", &newShapeSize[0], 10.0f, 100.0f);
                break;
        }
        
        ImGui::Separator();
        
        // Color picker
        ImGui::ColorEdit3("Color", newShapeColor);
        
        ImGui::Separator();
        
        // Physics properties
        ImGui::SliderFloat("Mass", &newShapeMass, 0.1f, 10.0f);
        ImGui::SliderFloat("Gravity", &newShapeGravity, -50.0f, 50.0f);
        ImGui::SliderFloat("Restitution", &newShapeRestitution, 0.0f, 1.0f);
        
        ImGui::Separator();
        
        // Mode-specific controls
        switch (spawnMode) {
            case SpawnMode::SINGLE_CLICK: {
                ImGui::Text("Single Object Mode");
                if (ImGui::Checkbox("Click to Spawn Mode", &clickToSpawnMode)) {
                    // Toggle was changed
                }
                if (clickToSpawnMode) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Click anywhere to spawn objects!");
                    ImGui::Text("Click count: %d", spawnObjectCount);
                } else {
                    ImGui::SliderInt("Object Count", &spawnObjectCount, 1, 50);
                    if (ImGui::Button("Spawn Single", ImVec2(-1, 0))) {
                        spawnSingleObject();
                    }
                }
                break;
            }
            
            case SpawnMode::MATRIX: {
                ImGui::Text("Matrix Spawning Mode");
                ImGui::SliderInt("Rows", &matrixRows, 1, 20);
                ImGui::SliderInt("Columns", &matrixColumns, 1, 20);
                ImGui::SliderFloat("Spacing", &matrixSpacing, 10.0f, 100.0f);
                
                int totalObjects = matrixRows * matrixColumns;
                ImGui::Text("Total objects: %d", totalObjects);
                
                if (ImGui::Button("Spawn Matrix", ImVec2(-1, 0))) {
                    spawnMatrix();
                }
                break;
            }
            
            case SpawnMode::GOLDEN_RATIO: {
                ImGui::Text("Golden Ratio Spawning Mode");
                ImGui::SliderInt("Object Count", &goldenRatioCount, 1, 100);
                ImGui::SliderFloat("Base Radius", &goldenRatioRadius, 20.0f, 200.0f);
                ImGui::SliderFloat("Spacing Factor", &goldenRatioSpacing, 5.0f, 50.0f);
                
                if (ImGui::Button("Spawn Golden Ratio", ImVec2(-1, 0))) {
                    spawnGoldenRatio();
                }
                break;
            }
        }
        
        ImGui::Separator();
        
        // Clear all button
        if (ImGui::Button("Clear All Shapes", ImVec2(-1, 0))) {
            physicsEngine->clearShapes();
            selectedShape = nullptr;
        }
    }
    ImGui::End();
}

void UIManager::renderWorldSettings() {
    ImGui::SetNextWindowPos(ImVec2(10, 340), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 150), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("World Settings")) {
        glm::vec2 gravity = physicsEngine->getGravity();
        float gravityY = gravity.y;
        
        if (ImGui::SliderFloat("Gravity", &gravityY, -50.0f, 50.0f)) {
            physicsEngine->setGravity(glm::vec2(0.0f, gravityY));
            // Update all objects using global gravity
            for (const auto& shape : physicsEngine->getShapes()) {
                if (shape->getUseGlobalGravity()) {
                    shape->setGravity(gravityY);
                }
            }
        }
        
        ImGui::Separator();
        
        // Shape count
        ImGui::Text("Shapes: %zu", physicsEngine->getShapes().size());
        
        // Clear all button
        if (ImGui::Button("Clear All Shapes", ImVec2(-1, 0))) {
            physicsEngine->clearShapes();
            selectedShape = nullptr;
        }
    }
    ImGui::End();
}

void UIManager::setSelectedShape(Shape* shape) {
    selectedShape = shape;
    if (selectedShape) {
        updateSelectedShapeProperties();
    }
}

void UIManager::updateSelectedShapeProperties() {
    if (!selectedShape) return;
    
    glm::vec3 color = selectedShape->getColor();
    selectedColor[0] = color.r;
    selectedColor[1] = color.g;
    selectedColor[2] = color.b;
    
    selectedMass = selectedShape->getMass();
    selectedGravity = selectedShape->getGravity();
    selectedRestitution = selectedShape->getPhysics().restitution;
    selectedStatic = selectedShape->getPhysics().isStatic;
    
    switch (selectedShape->getType()) {
        case ShapeType::CIRCLE: {
            const Circle* circle = static_cast<const Circle*>(selectedShape);
            selectedSize[0] = circle->getRadius();
            break;
        }
        case ShapeType::RECTANGLE: {
            const Rectangle* rect = static_cast<const Rectangle*>(selectedShape);
            selectedSize[0] = rect->getWidth();
            selectedSize[1] = rect->getHeight();
            break;
        }
        case ShapeType::TRIANGLE: {
            const Triangle* triangle = static_cast<const Triangle*>(selectedShape);
            selectedSize[0] = triangle->getSideLength();
            break;
        }
    }
}

void UIManager::renderDebugPanel() {
    if (!showDebugPanel) return;
    
    ImGui::SetNextWindowPos(ImVec2(10, 510), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Debug Info", &showDebugPanel)) {
        ImGui::Text("Performance");
        ImGui::Separator();
        
        // FPS display with color coding
        ImGui::Text("FPS: ");
        ImGui::SameLine();
        if (currentFPS >= 55.0f) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%.1f", currentFPS);
        } else if (currentFPS >= 30.0f) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.1f", currentFPS);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%.1f", currentFPS);
        }
        
        // Object count
        ImGui::Text("Objects: %d", objectCount);
        
        // Memory usage (estimated)
        size_t estimatedMemory = objectCount * 256; // Rough estimate per object
        ImGui::Text("Est. Memory: %.1f KB", estimatedMemory / 1024.0f);
        
        ImGui::Separator();
        ImGui::Text("Physics");
        ImGui::Separator();
        
        // Physics engine info
        glm::vec2 gravity = physicsEngine->getGravity();
        ImGui::Text("Gravity: (%.1f, %.1f)", gravity.x, gravity.y);
        
        glm::vec2 worldBounds = physicsEngine->getWorldBounds();
        ImGui::Text("World Size: %.0f x %.0f", worldBounds.x, worldBounds.y);
        
        // Optimization performance
        ImGui::Separator();
        ImGui::Text("Optimization Performance");
        ImGui::Separator();
        
        ImGui::Text("Collision Checks: %d", physicsEngine->getCollisionChecksThisFrame());
        ImGui::Text("Actual Collisions: %d", physicsEngine->getActualCollisionsThisFrame());
        ImGui::Text("Spatial Cells: %d", physicsEngine->getSpatialCellCount());
        ImGui::Text("Neighbor Tracking: %d", physicsEngine->getNeighborTrackingCount());
        
        float efficiency = (physicsEngine->getCollisionChecksThisFrame() > 0) ? 
            (float)physicsEngine->getActualCollisionsThisFrame() / physicsEngine->getCollisionChecksThisFrame() * 100.0f : 0.0f;
        ImGui::Text("Collision Efficiency: %.1f%%", efficiency);
        
        // Selected object info
        if (selectedShape) {
            ImGui::Separator();
            ImGui::Text("Selected Object");
            ImGui::Separator();
            
            glm::vec2 pos = selectedShape->getPosition();
            glm::vec2 vel = selectedShape->getVelocity();
            
            ImGui::Text("Position: (%.1f, %.1f)", pos.x, pos.y);
            ImGui::Text("Velocity: (%.1f, %.1f)", vel.x, vel.y);
            ImGui::Text("Mass: %.2f", selectedShape->getMass());
            ImGui::Text("Static: %s", selectedShape->getPhysics().isStatic ? "Yes" : "No");
        }
        
        ImGui::Separator();
        
        // Controls
        if (ImGui::Button("Reset Physics", ImVec2(-1, 0))) {
            physicsEngine->setGravity(glm::vec2(0.0f, -9.81f));
        }
        
        ImGui::Separator();
        ImGui::Text("Visualization");
        ImGui::Separator();
        
        if (ImGui::Checkbox("Show Velocity Vectors", &showVelocityVectors)) {
            // Toggle was changed
        }
        if (showVelocityVectors) {
            ImGui::TextWrapped("Velocity vectors are displayed as lines from object centers");
        }
        
        ImGui::Separator();
        ImGui::Text("UI Settings");
        ImGui::Separator();
        
        ImGui::Text("UI Scale: %dx", this->uiScale);
        ImGui::SameLine();
        
        // Scale buttons in a horizontal layout
        if (ImGui::Button("1x")) {
            this->uiScale = 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("2x")) {
            this->uiScale = 2;
        }
        ImGui::SameLine();
        if (ImGui::Button("4x")) {
            this->uiScale = 4;
        }
        ImGui::SameLine();
        if (ImGui::Button("8x")) {
            this->uiScale = 8;
        }
    }
    ImGui::End();
}

void UIManager::handleClickToSpawn(const glm::vec2& worldPos) {
    if (!clickToSpawnMode) return;
    
    // Create shape at click position
    glm::vec3 color(newShapeColor[0], newShapeColor[1], newShapeColor[2]);
    std::unique_ptr<Shape> newShape;
    
    switch (selectedShapeType) {
        case 0: // Circle
            newShape = std::make_unique<Circle>(worldPos, newShapeSize[0], color);
            break;
        case 1: // Rectangle
            newShape = std::make_unique<Rectangle>(worldPos, newShapeSize[0], newShapeSize[1], color);
            break;
        case 2: // Triangle
            newShape = std::make_unique<Triangle>(worldPos, newShapeSize[0], color);
            break;
    }
    
    if (newShape) {
        newShape->setMass(newShapeMass);
        newShape->setGravity(newShapeGravity);
        newShape->getPhysics().restitution = newShapeRestitution;
        physicsEngine->addShape(std::move(newShape));
        spawnObjectCount++;
    }
}

void UIManager::spawnSingleObject() {
    // Get screen center for single object spawn
    glm::vec2 worldBounds = physicsEngine->getWorldBounds();
    glm::vec2 screenCenter(worldBounds.x * 0.5f, worldBounds.y * 0.5f);
    glm::vec3 color(newShapeColor[0], newShapeColor[1], newShapeColor[2]);
    
    for (int i = 0; i < spawnObjectCount; ++i) {
        std::unique_ptr<Shape> newShape;
        
        switch (selectedShapeType) {
            case 0: // Circle
                newShape = std::make_unique<Circle>(screenCenter, newShapeSize[0], color);
                break;
            case 1: // Rectangle
                newShape = std::make_unique<Rectangle>(screenCenter, newShapeSize[0], newShapeSize[1], color);
                break;
            case 2: // Triangle
                newShape = std::make_unique<Triangle>(screenCenter, newShapeSize[0], color);
                break;
        }
        
        if (newShape) {
            newShape->setMass(newShapeMass);
            newShape->setGravity(newShapeGravity);
            newShape->getPhysics().restitution = newShapeRestitution;
            physicsEngine->addShape(std::move(newShape));
        }
    }
}

void UIManager::spawnMatrix() {
    glm::vec3 color(newShapeColor[0], newShapeColor[1], newShapeColor[2]);
    glm::vec2 worldBounds = physicsEngine->getWorldBounds();
    glm::vec2 screenCenter(worldBounds.x * 0.5f, worldBounds.y * 0.5f);
    
    // Calculate grid dimensions
    float totalWidth = (matrixColumns - 1) * matrixSpacing;
    float totalHeight = (matrixRows - 1) * matrixSpacing;
    glm::vec2 startPos = screenCenter - glm::vec2(totalWidth * 0.5f, totalHeight * 0.5f);
    
    for (int row = 0; row < matrixRows; ++row) {
        for (int col = 0; col < matrixColumns; ++col) {
            glm::vec2 position = startPos + glm::vec2(col * matrixSpacing, row * matrixSpacing);
            
            std::unique_ptr<Shape> newShape;
            switch (selectedShapeType) {
                case 0: // Circle
                    newShape = std::make_unique<Circle>(position, newShapeSize[0], color);
                    break;
                case 1: // Rectangle
                    newShape = std::make_unique<Rectangle>(position, newShapeSize[0], newShapeSize[1], color);
                    break;
                case 2: // Triangle
                    newShape = std::make_unique<Triangle>(position, newShapeSize[0], color);
                    break;
            }
            
            if (newShape) {
                newShape->setMass(newShapeMass);
                newShape->setGravity(newShapeGravity);
                newShape->getPhysics().restitution = newShapeRestitution;
                physicsEngine->addShape(std::move(newShape));
            }
        }
    }
}

void UIManager::spawnGoldenRatio() {
    glm::vec3 color(newShapeColor[0], newShapeColor[1], newShapeColor[2]);
    glm::vec2 worldBounds = physicsEngine->getWorldBounds();
    glm::vec2 screenCenter(worldBounds.x * 0.5f, worldBounds.y * 0.5f);
    
    // Golden angle in radians (137.5 degrees)
    const float goldenAngle = 137.5f * static_cast<float>(M_PI) / 180.0f;
    
    for (int i = 0; i < goldenRatioCount; ++i) {
        // Calculate radius using Fibonacci-like growth
        float radius = goldenRatioRadius * sqrtf(static_cast<float>(i + 1)) * goldenRatioSpacing / 10.0f;
        
        // Calculate angle using golden ratio
        float angle = i * goldenAngle;
        
        // Calculate position on spiral
        glm::vec2 position = screenCenter + glm::vec2(
            radius * cos(angle),
            radius * sin(angle)
        );
        
        std::unique_ptr<Shape> newShape;
        switch (selectedShapeType) {
            case 0: // Circle
                newShape = std::make_unique<Circle>(position, newShapeSize[0], color);
                break;
            case 1: // Rectangle
                newShape = std::make_unique<Rectangle>(position, newShapeSize[0], newShapeSize[1], color);
                break;
            case 2: // Triangle
                newShape = std::make_unique<Triangle>(position, newShapeSize[0], color);
                break;
        }
        
        if (newShape) {
            newShape->setMass(newShapeMass);
            newShape->setGravity(newShapeGravity);
            newShape->getPhysics().restitution = newShapeRestitution;
            physicsEngine->addShape(std::move(newShape));
        }
    }
}

void UIManager::renderCopyright() {
    // Set window position at bottom left with generous margins to ensure visibility
    ImGui::SetNextWindowPos(ImVec2(20, ImGui::GetIO().DisplaySize.y - 60), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 40), ImGuiCond_Always);
    
    // Make window background transparent and non-interactive
    ImGui::SetNextWindowBgAlpha(0.3f);
    
    if (ImGui::Begin("Copyright", nullptr, 
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs)) {
        
        // Center the text vertically within the window
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 0.8f), 
            "Copyright (c) 2025 Alex's Physics Engine - All rights reserved");
    }
    ImGui::End();
} 
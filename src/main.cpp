#ifdef Rectangle
#undef Rectangle
#endif
#ifdef Circle
#undef Circle
#endif
#ifdef Triangle
#undef Triangle
#endif

#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include "shapes.h"
#include "config.h"
#include "physics_engine.h"
#include "ui_manager.h"
#include <glad/glad.h>
#include <imgui.h>

using namespace std;

// Global variables
PhysicsEngine* physicsEngine = nullptr;
UIManager* uiManager = nullptr;
GLFWwindow* window = nullptr;

// Mouse callback functions
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // Check if ImGui is capturing input
    if (ImGui::GetIO().WantCaptureMouse) {
        return; // Let ImGui handle this
    }
    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        // Check if click-to-spawn mode is active
        if (uiManager && uiManager->isClickToSpawnMode()) {
            // Convert screen coordinates to world coordinates
            glm::vec2 worldPos(xpos, ypos);
            uiManager->handleClickToSpawn(worldPos);
        } else if (physicsEngine) {
            // Normal shape selection/dragging
            physicsEngine->handleMousePress(glm::vec2(xpos, ypos));
            
            // Update UI selection
            Shape* selected = physicsEngine->getSelectedShape();
            if (uiManager) {
                uiManager->setSelectedShape(selected);
            }
        }
    } else if (action == GLFW_RELEASE) {
        if (physicsEngine && (!uiManager || !uiManager->isClickToSpawnMode())) {
            physicsEngine->handleMouseRelease();
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    // Check if ImGui is capturing input
    if (ImGui::GetIO().WantCaptureMouse) {
        return; // Let ImGui handle this
    }
    
    if (physicsEngine) {
        physicsEngine->handleMouseMove(glm::vec2(xpos, ypos));
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        cout << "Escape key pressed - closing window" << endl;
        glfwSetWindowShouldClose(window, true);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    cout << "Window resized to: " << width << "x" << height << endl;
    glViewport(0, 0, width, height);
    
    // Update physics engine world bounds
    if (physicsEngine) {
        physicsEngine->setWorldBounds(glm::vec2(width, height));
    }
    
    // Update renderer projection if available
    if (physicsEngine && physicsEngine->getRenderer()) {
        physicsEngine->getRenderer()->setupProjection(width, height);
    }
}

void setupOpenGL(int width, int height) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    // Note: Projection setup moved to ModernRenderer
    // This function now only handles global OpenGL state
    
    cout << "OpenGL setup complete for " << width << "x" << height << endl;
}

void renderBackground() {
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Note: Grid rendering moved to ModernRenderer for consistency
    // This function now only handles background clearing
}

int main() {
    cout << "Starting Physics Engine..." << endl;
    
    // Initialize GLFW
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }
    cout << "GLFW initialized successfully" << endl;

    // Create window
    window = glfwCreateWindow(1200, 800, "2D Physics Engine", NULL, NULL);
    if (!window) {
        cerr << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    cout << "Window created successfully" << endl;

    glfwMakeContextCurrent(window);
    
    // Enable vsync for more stable frame rates
    glfwSwapInterval(1);
    
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Failed to initialize GLAD" << endl;
        glfwTerminate();
        return -1;
    }
    cout << "GLAD initialized successfully" << endl;

    // Get actual window size and setup OpenGL
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    setupOpenGL(width, height);
    
    // Initialize ImGui
    if (!UIManager::init(window)) {
        cerr << "Failed to initialize ImGui" << endl;
        glfwTerminate();
        return -1;
    }
    cout << "ImGui initialized successfully" << endl;

    // Initialize physics engine
    physicsEngine = new PhysicsEngine(width, height);
    
    // Initialize modern GPU renderer
    if (!physicsEngine->initializeRenderer(width, height)) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        delete physicsEngine;
        glfwTerminate();
        return -1;
    }
    
    // Initialize UI manager
    uiManager = new UIManager(physicsEngine);
    cout << "Physics engine and UI manager created" << endl;
    
    // Set world bounds
    physicsEngine->setWorldBounds(glm::vec2(width, height));
    
    // Add some test shapes for mouse interaction testing
    auto testCircle = std::make_unique<Circle>(glm::vec2(width * 0.5f, height * 0.5f), 50.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    testCircle->setMass(1.0f);
    testCircle->setGravity(981.0f); // Use positive gravity (downward)
    testCircle->getPhysics().restitution = 0.8f;
    testCircle->getPhysics().isStatic = false; // Ensure it's not static
    physicsEngine->addShape(std::move(testCircle));
    
    auto testRect = std::make_unique<Rectangle>(glm::vec2(width * 0.25f, height * 0.25f), 80.0f, 60.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    testRect->setMass(1.0f);
    testRect->setGravity(981.0f); // Use positive gravity (downward)
    testRect->getPhysics().restitution = 0.8f;
    testRect->getPhysics().isStatic = false; // Ensure it's not static
    physicsEngine->addShape(std::move(testRect));
    
    auto testTriangle = std::make_unique<Triangle>(glm::vec2(width * 0.75f, height * 0.75f), 60.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    testTriangle->setMass(1.0f);
    testTriangle->setGravity(981.0f); // Use positive gravity (downward)
    testTriangle->getPhysics().restitution = 0.8f;
    testTriangle->getPhysics().isStatic = false; // Ensure it's not static
    physicsEngine->addShape(std::move(testTriangle));
    
    cout << "Physics engine initialized with test shapes. Starting main loop..." << endl;

    // Main loop with fixed timestep physics
    const float fixedTimeStep = 1.0f / 120.0f; // 120 Hz physics for smooth motion
    float accumulator = 0.0f;
    float lastTime = static_cast<float>(glfwGetTime());
    int frameCount = 0;
    const float targetFrameTime = 1.0f / 120.0f; // Target 120 FPS for rendering
    
    while (!glfwWindowShouldClose(window)) {
        float currentTime = static_cast<float>(glfwGetTime());
        float frameTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // Debug: Check if window should close
        if (glfwWindowShouldClose(window)) {
            cout << "Window should close detected!" << endl;
            break;
        }
        
        // Add frame time to accumulator
        accumulator += frameTime;
        
        // Cap accumulator to prevent spiral of death
        if (accumulator > 0.25f) accumulator = 0.25f;
        
        // Run physics updates with fixed timestep
        while (accumulator >= fixedTimeStep) {
            physicsEngine->update(fixedTimeStep);
            accumulator -= fixedTimeStep;
        }
        
        // Frame rate limiting for rendering - wait if we're running too fast
        if (frameTime < targetFrameTime) {
            float sleepTime = targetFrameTime - frameTime;
            if (sleepTime > 0.001f) { // Only sleep if it's worth it (>1ms)
                glfwWaitEventsTimeout(sleepTime);
            }
        }

        // Render
        renderBackground();
        physicsEngine->render();
        
        // Render velocity vectors if enabled
        if (uiManager->getShowVelocityVectors()) {
            std::cout << "DEBUG: Velocity vectors toggle is ON" << std::endl;
            physicsEngine->renderVelocityVectors();
        } else {
            std::cout << "DEBUG: Velocity vectors toggle is OFF" << std::endl;
        }
        
        // Render spatial grid if enabled
        if (physicsEngine->getShowSpatialGrid()) {
            physicsEngine->renderSpatialGrid();
        }

        // Render UI
        uiManager->newFrame();
        uiManager->render();
        UIManager::renderFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // Update debug info for UI
        const auto& shapes = physicsEngine->getShapes();
        uiManager->setDeltaTime(frameTime);
        uiManager->setObjectCount(static_cast<int>(shapes.size()));
    }

    cout << "Shutting down..." << endl;
    
    // Cleanup
    delete uiManager;
    delete physicsEngine;
    UIManager::shutdown();
    glfwTerminate();
    
    cout << "Physics Engine terminated successfully" << endl;
    return 0;
}
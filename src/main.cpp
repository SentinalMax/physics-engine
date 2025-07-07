#include "config.h"
#include "physics_engine.h"
#include "ui_manager.h"
#include <iostream>

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
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            cout << "Mouse press at: (" << xpos << ", " << ypos << ")" << endl;
            
            // Check if click-to-spawn mode is active
            if (uiManager->isClickToSpawnMode()) {
                // Convert screen coordinates to world coordinates
                glm::vec2 worldPos(xpos, ypos);
                uiManager->handleClickToSpawn(worldPos);
                cout << "Spawned object at: (" << worldPos.x << ", " << worldPos.y << ")" << endl;
            } else {
                // Normal shape selection/dragging
                physicsEngine->handleMousePress(glm::vec2(xpos, ypos));
                
                // Update UI selection
                Shape* selected = physicsEngine->getSelectedShape();
                uiManager->setSelectedShape(selected);
                if (selected) {
                    cout << "Selected shape at position: (" << selected->getPosition().x << ", " << selected->getPosition().y << ")" << endl;
                }
            }
        } else if (action == GLFW_RELEASE) {
            cout << "Mouse release" << endl;
            if (!uiManager->isClickToSpawnMode()) {
                physicsEngine->handleMouseRelease();
            }
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    // Check if ImGui is capturing input
    if (ImGui::GetIO().WantCaptureMouse) {
        return; // Let ImGui handle this
    }
    
    physicsEngine->handleMouseMove(glm::vec2(xpos, ypos));
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
    
    // Update projection matrix for new window size
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1); // Top-left origin
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    physicsEngine->setWorldBounds(glm::vec2(width, height));
}

void setupOpenGL(int width, int height) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    // Set up 2D orthographic projection matching window size
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1); // Top-left origin
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    cout << "OpenGL setup complete for " << width << "x" << height << endl;
}

void renderBackground() {
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Draw grid
    glColor3f(0.2f, 0.2f, 0.25f);
    glBegin(GL_LINES);
    
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    // Vertical lines
    for (int x = 0; x <= width; x += 50) {
        glVertex2f(static_cast<float>(x), 0.0f);
        glVertex2f(static_cast<float>(x), static_cast<float>(height));
    }
    
    // Horizontal lines
    for (int y = 0; y <= height; y += 50) {
        glVertex2f(0.0f, static_cast<float>(y));
        glVertex2f(static_cast<float>(width), static_cast<float>(y));
    }
    
    glEnd();
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

    // Create physics engine and UI manager
    physicsEngine = new PhysicsEngine();
    uiManager = new UIManager(physicsEngine);
    cout << "Physics engine and UI manager created" << endl;
    
    // Set world bounds
    physicsEngine->setWorldBounds(glm::vec2(width, height));
    
    // Add some initial shapes for demonstration
    physicsEngine->addShape(std::make_unique<Circle>(glm::vec2(200.0f, 100.0f), 30.0f, glm::vec3(1, 0, 0)));
    physicsEngine->addShape(std::make_unique<Rectangle>(glm::vec2(400.0f, 150.0f), 60.0f, 40.0f, glm::vec3(0, 1, 0)));
    physicsEngine->addShape(std::make_unique<Triangle>(glm::vec2(600.0f, 200.0f), 50.0f, glm::vec3(0, 0, 1)));
    
    // Add a static ground
    auto ground = std::make_unique<Rectangle>(
        glm::vec2(static_cast<float>(width) / 2.0f, static_cast<float>(height) - 50.0f),
        static_cast<float>(width), 100.0f, glm::vec3(0.5f, 0.5f, 0.5f));
    ground->getPhysics().isStatic = true;
    physicsEngine->addShape(std::move(ground));
    
    cout << "Initial shapes added. Starting main loop..." << endl;

    // Main loop
    float lastTime = 0.0f;
    int frameCount = 0;
    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // Cap delta time to prevent large jumps
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // Update physics
        physicsEngine->update(deltaTime);

        // Render
        renderBackground();
        physicsEngine->render();
        
        // Render velocity vectors if enabled
        if (uiManager->getShowVelocityVectors()) {
            physicsEngine->renderVelocityVectors();
        }

        // Render UI
        uiManager->newFrame();
        uiManager->render();
        UIManager::renderFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // Debug output every 60 frames
        frameCount++;
        if (frameCount % 60 == 0) {
            const auto& shapes = physicsEngine->getShapes();
            Shape* selected = physicsEngine->getSelectedShape();
            cout << "Frame " << frameCount << " - Shapes: " << shapes.size() 
                 << " - FPS: " << (1.0f / deltaTime) 
                 << " - Selected: " << (selected ? "Yes" : "No") << endl;
        }
        
        // Update debug info for UI
        float fps = 1.0f / static_cast<float>(deltaTime);
        const auto& shapes = physicsEngine->getShapes();
        uiManager->setFPS(fps);
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
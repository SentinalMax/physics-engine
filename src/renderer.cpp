#include "renderer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

ModernRenderer::ModernRenderer() 
    : shaderProgram(0), circleVAO(0), circleVBO(0), circleEBO(0), circleInstanceVBO(0),
      rectVAO(0), rectVBO(0), rectInstanceVBO(0),
      triangleVAO(0), triangleVBO(0), triangleInstanceVBO(0),
      gridVAO(0), gridVBO(0) {
}

ModernRenderer::~ModernRenderer() {
    if (shaderProgram) glDeleteProgram(shaderProgram);
    if (circleVAO) glDeleteVertexArrays(1, &circleVAO);
    if (circleVBO) glDeleteBuffers(1, &circleVBO);
    if (circleEBO) glDeleteBuffers(1, &circleEBO);
    if (circleInstanceVBO) glDeleteBuffers(1, &circleInstanceVBO);
    if (rectVAO) glDeleteVertexArrays(1, &rectVAO);
    if (rectVBO) glDeleteBuffers(1, &rectVBO);
    if (rectInstanceVBO) glDeleteBuffers(1, &rectInstanceVBO);
    if (triangleVAO) glDeleteVertexArrays(1, &triangleVAO);
    if (triangleVBO) glDeleteBuffers(1, &triangleVBO);
    if (triangleInstanceVBO) glDeleteBuffers(1, &triangleInstanceVBO);
    if (gridVAO) glDeleteVertexArrays(1, &gridVAO);
    if (gridVBO) glDeleteBuffers(1, &gridVBO);
}

std::string ModernRenderer::loadShaderSource(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filename << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string result = buffer.str();
    
    if (result.empty()) {
        std::cerr << "Shader file is empty: " << filename << std::endl;
    } else {
        std::cout << "Successfully loaded shader: " << filename << " (" << result.length() << " bytes)" << std::endl;
    }
    
    return result;
}

GLuint ModernRenderer::compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLuint ModernRenderer::createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
    
    if (!vertexShader || !fragmentShader) {
        return 0;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        glDeleteProgram(program);
        return 0;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

void ModernRenderer::generateCircleGeometry() {
    const int segments = 32;
    circleVertices.clear();
    circleIndices.clear();
    
    // Center vertex
    circleVertices.push_back(0.0f); // x
    circleVertices.push_back(0.0f); // y
    circleVertices.push_back(1.0f); // r
    circleVertices.push_back(1.0f); // g
    circleVertices.push_back(1.0f); // b
    
    // Perimeter vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159f * static_cast<float>(i) / static_cast<float>(segments);
        float x = cos(angle);
        float y = sin(angle);
        
        circleVertices.push_back(x); // x
        circleVertices.push_back(y); // y
        circleVertices.push_back(1.0f); // r
        circleVertices.push_back(1.0f); // g
        circleVertices.push_back(1.0f); // b
    }
    
    // Generate indices for triangle fan
    for (int i = 1; i <= segments; ++i) {
        circleIndices.push_back(0);
        circleIndices.push_back(i);
        circleIndices.push_back(i + 1);
    }
}

void ModernRenderer::generateRectangleGeometry() {
    rectVertices = {
        // positions        // colors
        -0.5f, -0.5f,      1.0f, 1.0f, 1.0f,  // bottom-left
         0.5f, -0.5f,      1.0f, 1.0f, 1.0f,  // bottom-right
         0.5f,  0.5f,      1.0f, 1.0f, 1.0f,  // top-right
        -0.5f,  0.5f,      1.0f, 1.0f, 1.0f   // top-left
    };
}

void ModernRenderer::generateTriangleGeometry() {
    triangleVertices = {
        // positions        // colors
         0.0f,  0.577f,    1.0f, 1.0f, 1.0f,  // top
        -0.5f, -0.289f,    1.0f, 1.0f, 1.0f,  // bottom-left
         0.5f, -0.289f,    1.0f, 1.0f, 1.0f   // bottom-right
    };
}

bool ModernRenderer::initialize() {
    // Load and compile shaders
    std::string vertexSource = loadShaderSource("src/shaders/vertex.glsl");
    std::string fragmentSource = loadShaderSource("src/shaders/fragment.glsl");
    
    // Try alternative paths if the first attempt fails
    if (vertexSource.empty()) {
        vertexSource = loadShaderSource("Debug/src/shaders/vertex.glsl");
    }
    if (fragmentSource.empty()) {
        fragmentSource = loadShaderSource("Debug/src/shaders/fragment.glsl");
    }
    
    // Try Release path as well
    if (vertexSource.empty()) {
        vertexSource = loadShaderSource("Release/src/shaders/vertex.glsl");
    }
    if (fragmentSource.empty()) {
        fragmentSource = loadShaderSource("Release/src/shaders/fragment.glsl");
    }
    
    if (vertexSource.empty() || fragmentSource.empty()) {
        std::cerr << "Failed to load shader sources" << std::endl;
        return false;
    }
    
    shaderProgram = createShaderProgram(vertexSource.c_str(), fragmentSource.c_str());
    if (!shaderProgram) {
        return false;
    }
    
    // Generate VAOs and VBOs for instanced rendering
    glGenVertexArrays(1, &circleVAO);
    glGenVertexArrays(1, &rectVAO);
    glGenVertexArrays(1, &triangleVAO);
    
    glGenBuffers(1, &circleVBO);
    glGenBuffers(1, &rectVBO);
    glGenBuffers(1, &triangleVBO);
    
    glGenBuffers(1, &circleEBO);
    glGenBuffers(1, &circleInstanceVBO);
    glGenBuffers(1, &rectInstanceVBO);
    glGenBuffers(1, &triangleInstanceVBO);
    
    // Setup circle geometry
    generateCircleGeometry();
    
    // Setup rectangle geometry
    generateRectangleGeometry();
    
    // Setup triangle geometry
    generateTriangleGeometry();
    
    // Setup circle VAO/VBO
    glBindVertexArray(circleVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, circleEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, circleIndices.size() * sizeof(unsigned int), circleIndices.data(), GL_STATIC_DRAW);
    
    // Vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    
    // Instance attributes
    glBindBuffer(GL_ARRAY_BUFFER, circleInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * 1000, nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)0);
    glVertexAttribDivisor(2, 1);
    
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(2 * sizeof(float)));
    glVertexAttribDivisor(3, 1);
    
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(5 * sizeof(float)));
    glVertexAttribDivisor(4, 1);
    
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(6 * sizeof(float)));
    glVertexAttribDivisor(5, 1);
    
    // Setup rectangle VAO/VBO
    glBindVertexArray(rectVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, rectVertices.size() * sizeof(float), rectVertices.data(), GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindBuffer(GL_ARRAY_BUFFER, rectInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * 1000, nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)0);
    glVertexAttribDivisor(2, 1);
    
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(2 * sizeof(float)));
    glVertexAttribDivisor(3, 1);
    
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(5 * sizeof(float)));
    glVertexAttribDivisor(4, 1);
    
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(6 * sizeof(float)));
    glVertexAttribDivisor(5, 1);
    
    // Setup triangle VAO/VBO
    glBindVertexArray(triangleVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
    glBufferData(GL_ARRAY_BUFFER, triangleVertices.size() * sizeof(float), triangleVertices.data(), GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindBuffer(GL_ARRAY_BUFFER, triangleInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * 1000, nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)0);
    glVertexAttribDivisor(2, 1);
    
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(2 * sizeof(float)));
    glVertexAttribDivisor(3, 1);
    
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(5 * sizeof(float)));
    glVertexAttribDivisor(4, 1);
    
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(6 * sizeof(float)));
    glVertexAttribDivisor(5, 1);
    
    glBindVertexArray(0);
    
    std::cout << "Modern GPU renderer initialized successfully!" << std::endl;
    return true;
}

void ModernRenderer::setupProjection(int width, int height) {
    glUseProgram(shaderProgram);
    
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(width), 
                                     static_cast<float>(height), 0.0f, -1.0f, 1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void ModernRenderer::beginFrame() {
    glUseProgram(shaderProgram);
    clearInstances();
}

void ModernRenderer::endFrame() {
    // Nothing needed for now
}

void ModernRenderer::addCircle(const glm::vec2& position, float radius, const glm::vec3& color, float rotation) {
    InstanceData instance;
    instance.position = position;
    instance.color = color;
    instance.rotation = rotation;
    instance.scale = radius;
    circleInstances.push_back(instance);
}

void ModernRenderer::addRectangle(const glm::vec2& position, float width, float height, const glm::vec3& color, float rotation) {
    InstanceData instance;
    instance.position = position;
    instance.color = color;
    instance.rotation = rotation;
    instance.scale = std::max(width, height); // Use larger dimension for scale
    rectInstances.push_back(instance);
}

void ModernRenderer::addTriangle(const glm::vec2& position, float sideLength, const glm::vec3& color, float rotation) {
    InstanceData instance;
    instance.position = position;
    instance.color = color;
    instance.rotation = rotation;
    instance.scale = sideLength;
    triangleInstances.push_back(instance);
}

void ModernRenderer::updateInstanceBuffers() {
    if (!circleInstances.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, circleInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, circleInstances.size() * sizeof(InstanceData), circleInstances.data());
    }
    if (!rectInstances.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, rectInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, rectInstances.size() * sizeof(InstanceData), rectInstances.data());
    }
    if (!triangleInstances.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, triangleInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, triangleInstances.size() * sizeof(InstanceData), triangleInstances.data());
    }
}

void ModernRenderer::renderCircles() {
    if (circleInstances.empty()) return;
    
    updateInstanceBuffers();
    
    glBindVertexArray(circleVAO);
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(circleIndices.size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(circleInstances.size()));
}

void ModernRenderer::renderRectangles() {
    if (rectInstances.empty()) return;
    
    updateInstanceBuffers();
    
    glBindVertexArray(rectVAO);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(rectInstances.size()));
}

void ModernRenderer::renderTriangles() {
    if (triangleInstances.empty()) return;
    
    updateInstanceBuffers();
    
    glBindVertexArray(triangleVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 3, static_cast<GLsizei>(triangleInstances.size()));
}

void ModernRenderer::renderAllShapes() {
    // Render all shape types in batches for optimal GPU performance
    renderCircles();
    renderRectangles();
    renderTriangles();
}

void ModernRenderer::clearInstances() {
    circleInstances.clear();
    rectInstances.clear();
    triangleInstances.clear();
} 
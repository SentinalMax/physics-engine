#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "shapes.h"

// GPU-accelerated renderer using modern OpenGL
class ModernRenderer {
private:
    // Shader program
    GLuint shaderProgram;
    
    // Circle rendering
    GLuint circleVAO, circleVBO, circleEBO;
    GLuint circleInstanceVBO;
    std::vector<float> circleVertices;
    std::vector<unsigned int> circleIndices;
    
    // Rectangle rendering
    GLuint rectVAO, rectVBO;
    GLuint rectInstanceVBO;
    std::vector<float> rectVertices;
    
    // Triangle rendering
    GLuint triangleVAO, triangleVBO;
    GLuint triangleInstanceVBO;
    std::vector<float> triangleVertices;
    
    // Grid rendering
    GLuint gridVAO, gridVBO;
    
    // Instance data structures
    struct InstanceData {
        glm::vec2 position;
        glm::vec3 color;
        float rotation;
        float scale;
    };
    
    std::vector<InstanceData> circleInstances;
    std::vector<InstanceData> rectInstances;
    std::vector<InstanceData> triangleInstances;
    
    // Shader compilation
    GLuint compileShader(const char* source, GLenum type);
    GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource);
    std::string loadShaderSource(const char* filename);
    
    // Geometry generation
    void generateCircleGeometry();
    void generateRectangleGeometry();
    void generateTriangleGeometry();
    
    // Instance data management
    void updateInstanceBuffers();
    
public:
    ModernRenderer();
    ~ModernRenderer();
    
    // Initialization
    bool initialize();
    void setupProjection(int width, int height);
    
    // Rendering
    void beginFrame();
    void endFrame();
    
    // Shape rendering (batch mode)
    void addCircle(const glm::vec2& position, float radius, const glm::vec3& color, float rotation = 0.0f);
    void addRectangle(const glm::vec2& position, float width, float height, const glm::vec3& color, float rotation = 0.0f);
    void addTriangle(const glm::vec2& position, float sideLength, const glm::vec3& color, float rotation = 0.0f);
    
    // Batch rendering
    void renderCircles();
    void renderRectangles();
    void renderTriangles();
    void renderAllShapes();
    
    // Grid rendering
    void renderGrid(const std::vector<std::pair<int, int>>& occupiedCells, float cellSize);
    
    // Utility
    void clearInstances();
}; 
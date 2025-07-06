#include "config.h"
using namespace std;

int main() {
    GLFWwindow* window;

    if (!glfwInit()) {
        cout << "GLFW couldn't start" << endl;
        return -1;
    }

    window = glfwCreateWindow(640, 480, "My Window", NULL, NULL);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        return -1;
    }

    glClearColor(0.25f, 0.5f, 0.75, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window); //swap buffers (double buffer system)
    }
    glfwTerminate();
    return 0;
}
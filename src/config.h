#pragma once //this header can be included a bunch of times and it's not reimporting everything (header card)
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include <glad/glad.h> // loads in extra OpenGL functionality
#include <GLFW/glfw3.h> // spins up a window (cross-platform windowing library)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define M_PI 3.14159265358979323846

// Ensure OpenGL functions are available
#ifndef GLAPI
#define GLAPI extern
#endif

// ... rest of the original file content ... 
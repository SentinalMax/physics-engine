cmake_minimum_required(VERSION 3.5.0)
project(physics_engine VERSION 1.0.0)

find_package(OpenGL REQUIRED)

add_executable(physics_engine src/config.h src/main.cpp src/glad.c)

target_include_directories(physics_engine PRIVATE dependencies)

# Link GLFW from local dependencies 
target_link_libraries(physics_engine 
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lib-vc2022/glfw3.lib
    OpenGL::GL
)
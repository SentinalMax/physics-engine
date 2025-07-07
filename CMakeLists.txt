cmake_minimum_required(VERSION 3.10.0)
project(physics_engine VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

# Add ImGui source files
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui-1.89.9)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Add source files
set(SOURCES
    src/main.cpp
    src/physics_engine.cpp
    src/shapes.cpp
    src/ui_manager.cpp
    src/renderer.cpp
    src/glad.c
    ${IMGUI_SOURCES}
)

add_executable(physics_engine ${SOURCES})

target_include_directories(physics_engine PRIVATE 
    dependencies
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glm-0.9.9.8
    dependencies/GLFW
    dependencies/glad
    dependencies/KHR
)

# Link GLFW from local dependencies 
target_link_libraries(physics_engine 
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lib-vc2022/glfw3.lib
    OpenGL::GL
    opengl32
)

# Add compiler definitions for ImGui and OpenGL
target_compile_definitions(physics_engine PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLAD
    GL_GLEXT_PROTOTYPES
    _USE_MATH_DEFINES
)

# Copy shader files to build directory
file(COPY src/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Copy shader files to the output directory (Debug/Release/etc.)
add_custom_command(TARGET physics_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/src/shaders $<TARGET_FILE_DIR:physics_engine>/src/shaders
)
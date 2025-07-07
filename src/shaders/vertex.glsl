#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aInstancePos;
layout (location = 3) in vec3 aInstanceColor;
layout (location = 4) in float aInstanceRotation;
layout (location = 5) in float aInstanceScale;

out vec3 Color;

uniform mat4 projection;
uniform mat4 view;

void main() {
    // Apply instance transformation
    float cos_rot = cos(aInstanceRotation);
    float sin_rot = sin(aInstanceRotation);
    mat2 rotation = mat2(cos_rot, -sin_rot, sin_rot, cos_rot);
    
    vec2 worldPos = rotation * (aPos * aInstanceScale) + aInstancePos;
    
    gl_Position = projection * view * vec4(worldPos, 0.0, 1.0);
    Color = aInstanceColor; // Use instance color instead of vertex color
} 
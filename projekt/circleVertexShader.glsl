#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

out vec3 fragColor;

uniform vec2 circleCenter;
uniform bool colorSwap;

void main() {
    
    gl_Position = vec4(position.x + circleCenter.x, position.y + circleCenter.y, position.z, 1.0);
    
    // Swap colors when needed
    if (colorSwap) {
        if (color.g > color.r) {
            fragColor = vec3(1.0f, 0.0f, 0.0f);
        } else { // If originally red (center)
            fragColor = vec3(0.0f, 1.0f, 0.0f);
        }
    } else {
        fragColor = color;
    }
}

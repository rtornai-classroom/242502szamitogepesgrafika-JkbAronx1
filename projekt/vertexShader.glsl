#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

out vec3 fragColor;

uniform vec2 circleCenter;
uniform bool colorSwap;
uniform float lineY;
uniform int objectType; // 0 = kör, 1 = vonal

void main() {
    if (objectType == 0) {
        // Kör
        gl_Position = vec4(position.x + circleCenter.x, position.y + circleCenter.y, position.z, 1.0);

        if (colorSwap) {
            if (color.g > color.r) {
                fragColor = vec3(1.0f, 0.0f, 0.0f);  // Külsõ zöld terület helyett piros
            } else {
                fragColor = vec3(0.0f, 1.0f, 0.0f);  // Eredetileg piros középpont helyett zöld
            }
        } else {
            fragColor = color;
        }
    } else if (objectType == 1) {
        // Vonal
        gl_Position = vec4(position.x, position.y + lineY, position.z, 1.0);
        fragColor = color;
    }
}

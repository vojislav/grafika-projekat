#version 330 core
layout (location = 0) out vec4 FragColor;

in vec2 texCoords;
in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightColor;

void main() {
    FragColor = vec4(lightColor, 1.0);
}
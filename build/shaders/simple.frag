#version 410

layout( location = 0 ) out vec4 FragColor;

uniform vec3 LightColor;

void main() {
    FragColor = vec4(LightColor, 1.0);
}

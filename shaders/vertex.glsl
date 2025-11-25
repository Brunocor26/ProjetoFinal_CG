#version 330 core

layout (location = 0) in vec2 aPos;

uniform mat4 projection;
uniform vec2 offset;

void main()
{
    gl_Position = projection * vec4(aPos + offset, 0.0, 1.0);
}

#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec2 offset;
uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(aPos.x + offset.x, aPos.y + offset.y, 0.0, 1.0);
}

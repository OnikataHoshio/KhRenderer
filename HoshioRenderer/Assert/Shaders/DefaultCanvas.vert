#version 430 core
layout (location = 0) in vec3 inPos;

out vec3 CanvasPos; 

void main()
{
    CanvasPos = inPos;
    gl_Position = vec4(CanvasPos, 1.0);
}
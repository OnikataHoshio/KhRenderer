#version 430 core
layout (location = 0) in vec3 inPos;

out vec3 CanvasPos; 
out vec2 CanvasUV;

void main()
{
    CanvasPos = inPos;
    CanvasUV = 0.5 * (inPos.xy + 1.0);
    gl_Position = vec4(CanvasPos, 1.0);
}
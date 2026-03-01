#version 430 core
out vec4 FragColor;

in vec3 CanvasPos; 

void main()
{
   FragColor = vec4(CanvasPos, 1.0);
}
#version 460 core

out vec4 FragColor;
in vec2 CanvasUV;

uniform sampler2D uTexture;
uniform float uGamma;    
//uniform float uExposure; 

vec3 ToneMapACES(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 color = texture(uTexture, CanvasUV).rgb;

    // 线性 HDR 颜色先乘曝光
    // color *= uExposure;

    color = ToneMapACES(color);

    color = pow(color, vec3(1.0 / uGamma));

    FragColor = vec4(color, 1.0);
}
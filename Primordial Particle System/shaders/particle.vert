#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in float density;
uniform vec2 resolution;
uniform float particleRadius;

out float vDensity;

void main()
{
    vDensity = density;

    vec2 screenPos = position / resolution * 2.0 - 1.0;
    screenPos.y = -screenPos.y;

    vec2 offset = vec2(particleRadius / resolution.x, particleRadius / resolution.y);
    gl_Position = vec4(screenPos + offset * gl_VertexID, 0.0, 1.0);
}

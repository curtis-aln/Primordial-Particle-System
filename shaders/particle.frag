#version 330 core

in float vDensity;
out vec4 FragColor;

uniform sampler2D density_texture; // Add this line

uniform float range1;
uniform float range2;
uniform float range3;

uniform vec4 first_color;
uniform vec4 second_color;
uniform vec4 third_color;
uniform vec4 fourth_color;

vec4 getColor(float density)
{
    if (density <= 0.0)
    {
        return first_color;
    }
    else if (density <= range1)
    {
        float factor = density / range1;
        return mix(first_color, second_color, factor);
    }
    else if (density <= range2)
    {
        float factor = (density - range1) / (range2 - range1);
        return mix(second_color, third_color, factor);
    }
    else if (density < range3)
    {
        float factor = (density - range2) / (range3 - range2);
        return mix(third_color, fourth_color, factor);
    }
    else
    {
        return fourth_color;
    }
}

void main()
{
    float density = texture(density_texture, vec2(0.0, gl_FragCoord.y / resolution.y)).r; // Read the density from the texture
    FragColor = getColor(density);
}

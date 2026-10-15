uniform sampler2D texture;
uniform vec2 texelSize;
uniform float sigma;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec4 col = vec4(0.0);
    float total = 0.0;
    int krad = 3; // covers 3*sigma comfortably

    for (int x = -krad; x <= krad; x++)
    {
        for (int y = -krad; y <= krad; y++)
        {
            float w = exp(-float(x*x + y*y) / (2.0 * sigma * sigma));
            col += texture2D(texture, uv + vec2(float(x), float(y)) * texelSize) * w;
            total += w;
        }
    }

    gl_FragColor = col / total;
}
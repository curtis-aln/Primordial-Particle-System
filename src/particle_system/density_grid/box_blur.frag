uniform sampler2D texture;
uniform vec2 texelSize;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec4 col = vec4(0.0);

    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
            col += texture2D(texture, uv + vec2(float(x), float(y)) * texelSize);

    gl_FragColor = col / 9.0;
}
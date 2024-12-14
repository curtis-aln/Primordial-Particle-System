uniform float circleRadius;

void main() {
    gl_Position = vec4(gl_Vertex.xy * circleRadius, 0.0, 1.0);
}

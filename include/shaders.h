// A simple vertex shader
static const char* vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() 
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// A texture shader for a scalar field and obstacles
static const char* fragment_shader_src = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D scalarTex;   // a scalar field
uniform sampler2D obstacleTex; // obstacle mask

// Jet colormap
vec3 jet(float t) 
{
    t = clamp(t, 0.0, 1.0);

    // The key colors
    vec3 c0 = vec3(0.0, 0.0, 0.5); // dark blue
    vec3 c1 = vec3(0.0, 0.0, 1.0); // blue
    vec3 c2 = vec3(0.0, 1.0, 1.0); // cyan
    vec3 c3 = vec3(1.0, 1.0, 0.0); // yellow
    vec3 c4 = vec3(1.0, 0.0, 0.0); // red
    vec3 c5 = vec3(0.5, 0.0, 0.0); // dark red

    // Interpolate based on the value's range
    if (t < 0.125) {
        return mix(c0, c1, t / 0.125);
    } else if (t < 0.375) {
        return mix(c1, c2, (t - 0.125) / 0.25);
    } else if (t < 0.625) {
        return mix(c2, c3, (t - 0.375) / 0.25);
    } else if (t < 0.875) {
        return mix(c3, c4, (t - 0.625) / 0.25);
    } else {
        return mix(c4, c5, (t - 0.875) / 0.125);
    }
}

void main()
{
    float scalar = texture(scalarTex, TexCoord).r;   
    float mask   = texture(obstacleTex, TexCoord).r; // 0 == fluid, 1 == obstacle

    float smoothedMask = smoothstep(0.2, 0.3, mask);

    if (smoothedMask > 0.5) {
        // Obstacle: render dark gray
        FragColor = vec4(0.2, 0.2, 0.2, 1.0);
    } else {
        // Fluid: jet colormap
        vec3 col = jet(scalar);
        FragColor = vec4(col, 1.0);
    }
}
)";

// A tracer vertex shader
static const char* tracer_vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec2 uGridSize;  
uniform float uPointSize;   

void main() 
{
    // Grid coords to NDC
    float x = (aPos.x / uGridSize.x) * 2.0 - 1.0;
    float y = (aPos.y / uGridSize.y) * 2.0 - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);

    gl_PointSize = uPointSize;
}
)";

static const char* tracer_fragment_shader_src = R"(
#version 330 core

out vec4 FragColor;
uniform vec4 uTracerColor;

void main() 
{
    // Distance from center
    float dist = length(gl_PointCoord - vec2(0.5));

    // Throw away anything outside the circle
    if (dist > 0.5)
        discard;

    FragColor = uTracerColor;

    // // Tail fade (still left → right for now)
    // float tail = 1.0 - gl_PointCoord.x;

    // // Color gradient
    // vec3 headColor = vec3(1.0, 0.2, 1.0);
    // vec3 tailColor = vec3(0.4, 0.0, 0.4);
    // vec3 color = mix(headColor, tailColor, tail);

    // // Smooth alpha near the edge
    // float alpha = smoothstep(0.5, 0.45, 0.5 - dist);

    // FragColor = vec4(color, alpha);
}


)";
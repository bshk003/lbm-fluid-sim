// The usual vertex shader
static const char* vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";


// A colorful fragment shader for visualizing scalar fields (like speed)
static const char* fragment_shader_src = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D fieldTexture; // normalized curl in [0,1]

// blue-white-red colormap
vec3 diverging(float t) {
    if (t < 0.5) {
        // 0.0 → blue, 0.5 → white
        return mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 1.0, 1.0), t * 2.0);
    } else {
        // 0.5 → white, 1.0 → red
        return mix(vec3(1.0, 1.0, 1.0), vec3(1.0, 0.0, 0.0), (t - 0.5) * 2.0);
    }
}

void main() {
    float value = texture(fieldTexture, TexCoord).r; // already normalized
    vec3 color = diverging(value);
    FragColor = vec4(color, 1.0);
}
)";

// // A grayscale fragment shader
// static const char* fragment_shader_grayscale_src = R"(
// #version 330 core
// in vec2 TexCoord;
// out vec4 FragColor;
// uniform sampler2D tex;
// void main() {
//     float val = texture(tex, TexCoord).r;
//     FragColor = vec4(val, val, val, 1.0); // grayscale
// }
// )";
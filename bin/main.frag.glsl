#version 430

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 out_Color;

layout(location = 2) in vec2 v_TexCoords;

void main() {
    out_Color = texture(tex, v_TexCoords);
}
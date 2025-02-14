#version 450
//#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 1) uniform sampler2D textures[100];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int f_textureID;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(textures[f_textureID], fragTexCoord);
}

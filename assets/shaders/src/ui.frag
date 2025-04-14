#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 1) uniform texture2D textures[75];
layout(set = 0, binding = 2) uniform sampler samp;
layout(set = 0, binding = 3) uniform texture2D uiTexture;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int f_textureID;
layout(location = 3) in vec3 FragPos;
layout(location = 4) flat in float time;
layout(location = 5) flat in vec3 lightPos;
layout(location = 6) flat in vec3 viewPos;
layout(location = 7) flat in int f_normalID;
layout(location = 8) in mat3 TBN;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(sampler2D(uiTexture, samp), fragTexCoord);
}

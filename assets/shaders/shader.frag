#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 1) uniform texture2D textures[];
layout(set = 0, binding = 2) uniform sampler samp;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int f_textureID;
layout(location = 0) out vec4 outColor;

void main() {
    //outColor = texture(textures[nonuniformEXT(f_textureID)], fragTexCoord);
    //outColor = texture(textures[(f_textureID)], fragTexCoord);
    outColor = texture(sampler2D(textures[f_textureID], samp), fragTexCoord);
}

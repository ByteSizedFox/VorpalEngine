#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 1) uniform texture2D textures[75];
layout(set = 0, binding = 2) uniform sampler samp;
layout(set = 0, binding = 3) uniform texture2D uiTexture;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int f_textureID;
layout(location = 3) flat in int f_isUI;
layout(location = 4) flat in int f_isDebug;
layout(location = 0) out vec4 outColor;

void main() {
    //outColor = texture(textures[nonuniformEXT(f_textureID)], fragTexCoord);
    //outColor = texture(textures[(f_textureID)], fragTexCoord);

    if (f_isUI == 1) {
        outColor = texture(sampler2D(uiTexture, samp), fragTexCoord);
    } else if (f_isDebug == 1) {
        outColor = vec4(fragColor, 1.0);
    } else {
        outColor = texture(sampler2D(textures[f_textureID], samp), fragTexCoord) * vec4(fragColor, 1.0);
    }
}

#extension GL_EXT_nonuniform_qualifier : enable

const float dayLength = 300.0; // 20*60 20 minute day cycle
const float AMBIENT_SCALE = 1.2; // Global ambient brightness control

vec3 calculateSunDirection(float time) {
    float angle = (time / dayLength) * 2.0 * 3.14159265;
    return normalize(vec3(cos(angle), sin(angle), 0.0));
}

vec4 textureIndex(int enableNonUniform, texture2D textures[75], sampler samp, vec2 uv, in int index) {
    if (enableNonUniform == 1) {
        return texture(sampler2D(textures[nonuniformEXT(index)], samp), uv);
    } else {
        return texture(sampler2D(textures[index], samp), uv);
    }
}

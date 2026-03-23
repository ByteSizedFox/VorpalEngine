#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
    float time;
    vec3 camPos;
} ubo;

layout(push_constant) uniform constant {
    mat4 model;
    int enableNormal;
    float metallic;
    float roughness;
} PushConstants;

layout(set = 1, binding = 0) readonly buffer BoneMatrices {
    mat4 bones[256];
} boneBuffer;

layout(location = 0) in vec3 inPosition;
// locations 1-7 are other vertex attributes (not needed for shadow pass)
layout(location = 8) in ivec4 inJointIndices;
layout(location = 9) in vec4 inJointWeights;

void main() {
    ivec4 ji = clamp(inJointIndices, ivec4(0), ivec4(255));
    mat4 skinMatrix =
        inJointWeights.x * boneBuffer.bones[ji.x] +
        inJointWeights.y * boneBuffer.bones[ji.y] +
        inJointWeights.z * boneBuffer.bones[ji.z] +
        inJointWeights.w * boneBuffer.bones[ji.w];

    vec4 skinnedPos = skinMatrix * vec4(inPosition, 1.0);
    gl_Position = ubo.lightSpaceMatrix * PushConstants.model * vec4(skinnedPos.xyz * 0.01, 1.0);
}

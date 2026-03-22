#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
    float time;
    vec3 camPos;
} ubo;

layout( push_constant ) uniform constant {
    mat4 model;
    int enableNormal;
    float metallic;
    float roughness;
} PushConstants;

layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = ubo.lightSpaceMatrix * PushConstants.model * vec4(inPosition * 0.01, 1.0);
}

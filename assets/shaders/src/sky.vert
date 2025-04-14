#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    float time;
} ubo;
layout( push_constant ) uniform constant {
	mat4 model;
        bool isUI;
        bool isDebug;
} PushConstants;

struct ObjectData {
        mat4 model;
        bool isUI;
        bool isDebug;
};
layout(binding = 4) readonly buffer ObjectBuffer{
	ObjectData objects[];
} objectBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in int textureID;
layout(location = 4) in int normalID;
layout(location = 5) in vec3 inTangent;
layout(location = 6) in vec3 inBitangent;

// pass world position to fragment for EYEDIR calc
layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) flat out float time;
layout(location = 2) flat out vec3 lightPos;

void main() {
    gl_Position = ubo.proj * vec4(inPosition.xy, -1.0, 1.0);
    mat3 viewRotation = mat3(ubo.view);
    fragWorldPos = transpose(viewRotation) * inPosition;
    time = ubo.time;
    lightPos = ubo.lightPos;
}

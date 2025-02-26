#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;
layout( push_constant ) uniform constant {
	mat4 model;
        bool isUI;
} PushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in int textureID;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out int f_textureID;
layout(location = 3) out int f_isUI;

void main() {
    gl_Position = ubo.proj * ubo.view * PushConstants.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    f_textureID = textureID;
    f_isUI = (PushConstants.isUI == false)?0:1 ;
}

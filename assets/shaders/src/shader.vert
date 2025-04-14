#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    float time;
    vec3 camPos;
} ubo;

layout( push_constant ) uniform constant {
    mat4 model;
    int enableNormal;
} PushConstants;

struct ObjectData {
    mat4 model;
};
layout(binding = 4) readonly buffer ObjectBuffer{
	ObjectData objects[];
} objectBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in int textureID;
layout(location	= 4) in	int normalID;
layout(location = 5) in vec4 aTangent;
layout(location = 6) in vec3 aBitangent;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) flat out int f_textureID;
layout(location = 2) flat out int f_normalID;
layout(location = 3) flat out float time;
layout(location = 4) flat out vec3 lightPos;
layout(location = 5) flat out vec3 viewPos;

layout(location = 6) out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
} vs_out;

void main() {
    gl_Position = (ubo.proj * ubo.view * PushConstants.model * vec4(inPosition * 0.01, 1.0));
    
    // Calculate fragment normal
    fragNormal = mat3(transpose(inverse(PushConstants.model))) * aNormal;
    
    // Pass through texture coordinates and IDs
    vs_out.TexCoords = inTexCoord;
    f_textureID = textureID;
    
    // Calculate fragment position in world space
    vs_out.FragPos = vec3(PushConstants.model * vec4(inPosition * 0.01, 1.0));
    
    // pass uniform data
    time = ubo.time;
    lightPos = ubo.lightPos;

    //viewPos = (-transpose(mat3(ubo.view)) * ubo.view[3].xyz) * -1.0;
    viewPos = ubo.camPos;

    // pass vertex data
    f_normalID = normalID;
    if (PushConstants.enableNormal == 0) {
        f_normalID = -1;
    }

    mat3 normalModelMatrix = mat3(transpose(inverse(PushConstants.model)));
    vec3 T = normalize(normalModelMatrix * aTangent.xyz);
    vec3 N = normalize(normalModelMatrix * aNormal);
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(cross(N, T));

    vs_out.TBN = mat3(T, B, N);
}

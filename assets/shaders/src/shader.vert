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
layout(location = 5) in int inMRID;
layout(location = 6) in vec4 aTangent;
layout(location = 7) in vec3 aBitangent;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) flat out int f_textureID;
layout(location = 2) flat out int f_normalID;
layout(location = 3) flat out float time;
layout(location = 4) flat out vec3 lightPos;
layout(location = 5) flat out vec3 viewPos;
layout(location = 6) flat out float f_metallic;
layout(location = 7) flat out float f_roughness;

layout(location = 8) out vec3 outFragPos;
layout(location = 9) out vec2 outTexCoords;
layout(location = 10) out mat3 outTBN;
layout(location = 13) flat out int f_mrID;
layout(location = 14) out vec4 outFragPosLightSpace;

void main() {
    vec4 worldPos = PushConstants.model * vec4(inPosition * 0.01, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    
    outFragPos = worldPos.xyz;
    outTexCoords = inTexCoord;
    f_textureID = textureID;
    f_mrID = inMRID;
    outFragPosLightSpace = ubo.lightSpaceMatrix * worldPos;
    
    mat3 normalMatrix = mat3(transpose(inverse(PushConstants.model)));
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T_in = normalize(normalMatrix * aTangent.xyz);
    
    // Re-orthogonalize T with respect to N with safeguard
    vec3 T = T_in - dot(T_in, N) * N;
    if (length(T) < 0.001) {
        T = cross(N, vec3(0, 1, 0));
        if (length(T) < 0.001) T = cross(N, vec3(1, 0, 0));
    }
    T = normalize(T);
    
    // Standard MikkTSpace bitangent derivation
    float tangentW = (aTangent.w == 0.0) ? 1.0 : aTangent.w;
    vec3 B = normalize(cross(N, T) * tangentW);

    fragNormal = N;
    outTBN = mat3(T, B, N);
    
    time = ubo.time;
    lightPos = ubo.lightPos;
    f_metallic = PushConstants.metallic;
    f_roughness = PushConstants.roughness;
    viewPos = ubo.camPos;

    f_normalID = (PushConstants.enableNormal == 1) ? normalID : -1;
}

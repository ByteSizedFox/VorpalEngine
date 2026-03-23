#version 450

#include "common.glsl"

layout(set = 0, binding = 1) uniform texture2D textures[75];
layout(set = 0, binding = 2) uniform sampler samp;
layout(set = 0, binding = 3) uniform texture2D uiTexture;
layout(set = 0, binding = 5) uniform sampler2DShadow shadowMap;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) flat in int f_textureID;
layout(location = 2) flat in int f_normalID;
layout(location = 3) flat in float time;
layout(location = 4) flat in vec3 lightPos;
layout(location = 5) flat in vec3 viewPos;
layout(location = 6) flat in float f_metallic;
layout(location = 7) flat in float f_roughness;

layout(location = 8) in vec3 inFragPos;
layout(location = 9) in vec2 inTexCoords;
layout(location = 10) in mat3 inTBN;
layout(location = 13) flat in int f_mrID;
layout(location = 14) in vec4 inFragPosLightSpace;

layout(location = 0) out vec4 FragColor;

const float PI = 3.14159265359;

vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom + 0.000001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k + 0.000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) * 
           GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ShadowCalculation(vec4 fragPosLightSpace) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    if (projCoords.z > 1.0) return 0.0;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            shadow += texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z));
        }    
    }
    shadow /= 9.0;
    
    return 1.0 - shadow;
}

void main() {
    if (f_textureID < 0 || f_textureID >= 75) { FragColor = vec4(0.8, 0.8, 0.8, 1.0); return; }
    vec4 albedoAlpha = textureIndex(0, textures, samp, inTexCoords, f_textureID);
    if (albedoAlpha.a < 0.1) discard;
    vec3 albedo = albedoAlpha.rgb;

    // Normal mapping
    vec3 N;
    if (f_normalID >= 0) {
        vec3 normalMap = textureIndex(0, textures, samp, inTexCoords, f_normalID).rgb;
        normalMap = normalize(normalMap * 2.0 - 1.0);
        N = normalize(inTBN * normalMap);
    } else {
        N = normalize(fragNormal);
    }

    vec3 V = normalize(viewPos - inFragPos);
    vec3 L = normalize(lightPos);
    vec3 H = normalize(V + L);

    float sunHeight = L.y;
    float sunDimming = smoothstep(-0.1, 0.1, sunHeight);
    float sunsetFactor = smoothstep(0.4, 0.0, sunHeight);

    // Balanced Light Colors
    vec3 dayColor = vec3(1.0, 0.98, 0.92);
    vec3 sunsetColor = vec3(1.0, 0.4, 0.1);
    vec3 lightColor = mix(dayColor, sunsetColor, sunsetFactor) * sunDimming;

    // Conservative Ambient
    float dayFactor = smoothstep(-0.15, 0.1, sunHeight);
    vec3 skyBase = mix(vec3(0.02, 0.03, 0.05), vec3(0.08, 0.15, 0.3), dayFactor);
    vec3 skyColor = mix(skyBase, sunsetColor * 0.4, sunsetFactor * dayFactor);
    float skyWeight = dot(N, vec3(0, 1, 0)) * 0.5 + 0.5;
    vec3 ambient = mix(vec3(0.01), skyColor, skyWeight) * albedo * AMBIENT_SCALE;

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = vec3(0.0);

    float shadow = ShadowCalculation(inFragPosLightSpace);

    if (f_mrID >= 0) {
        // PBR Path (Metallic-Roughness Map present)
        vec4 mrSample = textureIndex(0, textures, samp, inTexCoords, f_mrID);
        float roughness = clamp(mrSample.g, 0.05, 1.0);
        float metallic = clamp(mrSample.b, 0.0, 1.0);
        
        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        float NdotV = max(dot(N, V), 0.001);

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * NdotV * NdotL + 0.001;
        vec3 specular = numerator / denominator;

        Lo = (kD * albedo / PI + specular) * lightColor * NdotL * (1.0 - shadow);
    } else {
        // Pure Diffuse Path (No specular map)
        float kD = (1.0 - f_metallic); // Metals have no diffuse
        Lo = (kD * albedo / PI) * lightColor * NdotL * (1.0 - shadow);
    }

    vec3 color = ambient + Lo;
    FragColor = vec4(aces(color), albedoAlpha.a);
}

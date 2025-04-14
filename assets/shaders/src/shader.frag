#version 450

#include "common.glsl"

layout(set = 0, binding = 1) uniform texture2D textures[75];
layout(set = 0, binding = 2) uniform sampler samp;
layout(set = 0, binding = 3) uniform texture2D uiTexture;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) flat in int f_textureID;
layout(location = 2) flat in int f_normalID;
layout(location = 3) flat in float time;
layout(location = 4) flat in vec3 lightPos;
layout(location = 5) flat in vec3 viewPos;

layout(location = 6) in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

layout(location = 0) out vec4 FragColor;

// shader settings
const float specularStrength = 0.5;
const vec3 normalStrength = vec3(2.0,2.0,1.0);
const float lightRadius = 10000.0;

// sampler functions
vec3 getNormal() {
    vec3 normal = fragNormal;
    //normal = normal * 2.0 - 1.0;

    if (f_normalID >= 0) {
        normal = textureIndex(0, textures, samp, fs_in.TexCoords, f_normalID).rgb;
        normal *= vec3(2.0,2.0,1.0);
        normal = normal * 2.0 - 1.0; 

        // translate normal to model space
        normal = fs_in.TBN * normal;
    }
    return normalize(normal);
}
vec4 getColor() {
    return textureIndex(0, textures, samp, fs_in.TexCoords, f_textureID);
}

void main() {
    // input textures
    vec4 color = getColor();
    vec3 normal = getNormal();

    // directions
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);

    // day/night transition
    float sunHeight = normalize(lightPos - fs_in.FragPos).y;
    float dayFactor = smoothstep(-0.2, 0.2, sunHeight);

    // Base lighting
    vec3 dayLightColor = vec3(1.0, 0.95, 0.8); // Warm daylight
    vec3 nightLightColor = vec3(0.1, 0.2, 0.4); // Cool moonlight

    // Ambient light that changes with time of day
    vec3 dayAmbient = vec3(0.2, 0.2, 0.2);
    vec3 nightAmbient = vec3(0.05, 0.05, 0.1);
    vec3 ambient = mix(nightAmbient, dayAmbient, dayFactor);

    // Light color transitions
    vec3 lightColor = mix(nightLightColor, dayLightColor, dayFactor);

    // Diffuse lighting - Keep your existing diffuse calculation
    float dist = length(lightPos - fs_in.FragPos);
    float diff = clamp(dot(normal, lightDir), 0.0, 1.0);
    diff = clamp(diff * (1.0 - (dist / lightRadius)), 0.0, 1.0);
    vec3 diffuse = diff * lightColor;

    // Cook-Torrance parameters - adjust these to match your desired material properties
    float roughness = 0.3;          // Material roughness (0=smooth, 1=rough)
    float metallic = 0.0;           // Metallic factor (0=dielectric, 1=metallic)
    float F0_dielectric = 0.04;     // Base reflectivity for non-metals
    
    // Normalized vectors
    vec3 N = normalize(normal);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(viewDir);
    vec3 H = normalize(L + V);
    
    // Calculate basic dot products needed for multiple calculations
    float NdotL = max(dot(N, L), 0.001);  // Avoid complete darkness with small bias
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    // Distribution term - GGX/Trowbridge-Reitz
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = (NdotH * NdotH * (alpha2 - 1.0) + 1.0);
    float D = alpha2 / (3.14159265359 * denom * denom);
    
    // Geometric term - Smith GGX
    float k = ((roughness + 1.0) * (roughness + 1.0)) / 8.0;
    float G1L = NdotL / (NdotL * (1.0 - k) + k);
    float G1V = NdotV / (NdotV * (1.0 - k) + k);
    float G = G1L * G1V;
    
    // Fresnel term - Schlick's approximation
    // For metals, base reflectivity is derived from the albedo color
    vec3 F0 = mix(vec3(F0_dielectric), color.rgb, metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
    
    // Cook-Torrance specular BRDF
    vec3 specularBRDF = (D * G * F) / (4.0 * NdotL * NdotV + 0.001);
    
    // Apply light attenuation based on distance
    float attenuation = clamp(1.0 - (dist / lightRadius), 0.0, 1.0);
    
    // Final specular contribution
    vec3 specular = specularBRDF * lightColor * NdotL * attenuation * specularStrength;
    
    // For metals, diffuse component is reduced (metals don't scatter light inside)
    vec3 diffuseFactor = mix(vec3(1.0), vec3(0.0), metallic);
    diffuse *= diffuseFactor;
    
    // Combine all lighting components
    vec3 result = (ambient + diffuse + specular) * color.rgb;
    
    FragColor = vec4(result, color.a);
    
    // Debug visualization if needed
    // vec3 rgb_normal = normal * 0.5 + 0.5;
    // FragColor = vec4(rgb_normal, 1.0);
}

#version 450

#include "common.glsl"

#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 1) uniform texture2D textures[75];
layout(set = 0, binding = 2) uniform sampler samp;
layout(set = 0, binding = 3) uniform texture2D uiTexture;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) flat in float time;
layout(location = 2) flat in vec3 lightPos;

layout(location = 0) out vec4 outColor;

vec3 calculateEyeDir() {
    return normalize(fragWorldPos);
}

// CLOUD TEST
// Simple hash function
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// Simple noise function
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

// Function to generate clouds
vec3 generateClouds(vec3 color, vec2 uv, float time, float density) {
    vec3 clouds_edge_color = vec3(0.8, 0.8, 0.98);
    vec3 clouds_top_color = vec3(1.0, 1.0, 1.00);
    vec3 clouds_middle_color = vec3(0.92, 0.92, 0.98);
    vec3 clouds_bottom_color = vec3(0.83, 0.83, 0.94);
    float clouds_cutoff = 0.4;
    float clouds_fuzziness = 0.5;

    float n1 = noise(uv * 2.0 + time * 0.1);
    float n2 = 0.5 * noise(uv * 4.0 + time * 0.2);
    float n3 = 0.25 * noise(uv * 8.0 + time * 0.3);

    n1 = smoothstep(clouds_cutoff, clouds_cutoff + clouds_fuzziness, n1);
    n2 = smoothstep(clouds_cutoff, clouds_cutoff + clouds_fuzziness, n2 + n1 * 0.2) * 1.1;
    n3 = smoothstep(clouds_cutoff, clouds_cutoff + clouds_fuzziness, n3 + n2 * 0.4) * 1.2;

    vec3 clouds_color = mix(color, clouds_top_color, n3);
    clouds_color = mix(clouds_color, clouds_middle_color, n2);
    clouds_color = mix(clouds_color, clouds_bottom_color, n1);
    // The edge color gives a nice smooth edge, you can try turning this off if you need sharper edges
    clouds_color = mix(clouds_color, clouds_edge_color, n3);

    return vec3(clouds_color);
}

vec3 getSkyColor(vec3 eyeDir, vec3 sunDir) {
    float sunDot = max(dot(eyeDir, sunDir), 0.0);
    float sunIntensity = pow(sunDot, 256.0);
    vec3 sunColor = vec3(1.0, 0.9, 0.7) * sunIntensity;
    return vec3(0.3, 0.5, 0.8) + sunColor;
}

// END CLOUD TEST
void main() {
    vec3 EYEDIR = calculateEyeDir();
    float depth = abs(EYEDIR.y);
    vec3 POSITION = vec3(0.0,0.0,0.0);
    vec2 sky_uv = EYEDIR.xz / sqrt(EYEDIR.y);
    vec3 sunDir = normalize( lightPos );

    // get color for sky and ground
    vec3 groundColor = vec3(0.5, 0.2, 0.8);
    vec3 skyColor = getSkyColor(EYEDIR, sunDir);
    // combine sky and ground
    vec3 color = mix(skyColor, groundColor, -EYEDIR.y);
    // apply clouds
    //color = generateClouds(color, sky_uv * 0.5, time, 0.5);

    // Blend grid and clouds
    outColor.rgb = mix(skyColor, color, clamp(abs(EYEDIR.y) / 0.25, 0.0, 1.0));
    outColor.a = 1.0; //depth * 0.5;
}
	

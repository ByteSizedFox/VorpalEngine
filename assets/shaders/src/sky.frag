#version 450

#include "common.glsl"

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) flat in float time;
layout(location = 2) flat in vec3 lightPos;

layout(location = 0) out vec4 outColor;

// Simplified ACES fitting
vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Noise helpers for clouds
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    mat2 rot = mat2(1.6, 1.2, -1.2, 1.6);
    for (int i = 0; i < 5; ++i) {
        v += a * noise(p);
        p = rot * p * 2.0 + time * 0.01;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec3 eyeDir = normalize(fragWorldPos);
    vec3 sunDir = normalize(lightPos);
    
    float sunHeight = sunDir.y;
    float dayFactor = smoothstep(-0.1, 0.1, sunHeight);
    float sunsetFactor = smoothstep(0.4, -0.1, sunHeight);
    
    // Base sky colors (Linear)
    vec3 skyTop = mix(vec3(0.0, 0.01, 0.03), vec3(0.02, 0.08, 0.25), dayFactor);
    vec3 skyHorizon = mix(vec3(0.01, 0.01, 0.02), vec3(0.3, 0.4, 0.6), dayFactor);
    
    // Sunset tint
    vec3 sunsetColor = vec3(0.6, 0.2, 0.05);
    skyHorizon = mix(skyHorizon, sunsetColor, sunsetFactor * dayFactor);
    
    float viewHeight = max(eyeDir.y, 0.0);
    vec3 color = mix(skyHorizon, skyTop, pow(viewHeight, 0.8));
    
    // Procedural Clouds
    if (eyeDir.y > 0.0) {
        //vec2 cloudUV = eyeDir.xz / (eyeDir.y + 0.3);
        //float cloudDensity = fbm(cloudUV * 0.5 + time * 0.02);
        //cloudDensity = smoothstep(0.4, 0.7, cloudDensity) * viewHeight; // Fade clouds at horizon
        
        //vec3 cloudColor = mix(vec3(0.8, 0.85, 0.9), vec3(1.0), dayFactor);
        //cloudColor = mix(cloudColor, sunsetColor, sunsetFactor * 0.5);
        
        //color = mix(color, cloudColor, cloudDensity * 0.6);
    }

    // Sun
    float sunDot = max(dot(eyeDir, sunDir), 0.0);
    if (sunDot > 0.0) {
        float sunDisk = smoothstep(0.999, 1.0, sunDot);
        vec3 sunColor = vec3(1.0, 0.95, 0.8) * 3.0 * dayFactor;
        float sunGlow = pow(sunDot, 128.0) * 0.4 * dayFactor;
        color += sunColor * sunDisk + mix(sunColor, sunsetColor, sunsetFactor) * sunGlow;
    }
    
    // Ground
    if (eyeDir.y < 0.0) color = skyTop; //vec3(0.01);
    
    outColor = vec4(aces(color), 1.0);
}

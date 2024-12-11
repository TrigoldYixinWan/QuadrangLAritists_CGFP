#version 330 core 

in vec2 v_TexCoord;
in vec2 v_Position;

uniform vec3 u_Color;
uniform int u_PlanetType;  // 0: Sun, 1: Rocky, 2: Gas Giant; other values = non-planet
uniform vec2 u_LightPos;   
uniform float u_Time;

out vec4 fragColor;

float rand(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 ip = floor(p);
    vec2 u = fract(p);
    u = u*u*(3.0-2.0*u);

    float res = mix(
        mix(rand(ip), rand(ip+vec2(1.0,0.0)), u.x),
        mix(rand(ip+vec2(0.0,1.0)), rand(ip+vec2(1.0,1.0)), u.x),
        u.y);
    return res*res;
}

void main() {
    vec3 baseColor = u_Color;
    float alpha = 1.0;
    float dist = length(v_Position);

    // Only apply circular mask if it's a planet
    if((u_PlanetType == 0 || u_PlanetType == 1 || u_PlanetType == 2) && dist > 1.0) {
        discard;
    }

    // Different effects based on planet type
    if(u_PlanetType == 0) {  // Sun
        float corona = smoothstep(0.8, 1.0, dist);
        float flare = noise(v_Position * 10.0 + u_Time * 0.1);
        baseColor = mix(u_Color, u_Color * 1.5, corona * flare);
        float flicker = 1.0 + 0.1 * noise(vec2(u_Time * 2.0, 0.0));
        baseColor *= flicker;
    } else if(u_PlanetType == 1) {  // Rocky
        float rockPattern = noise(v_Position * 15.0);
        float craterPattern = smoothstep(0.4, 0.6, noise(v_Position * 8.0));
        baseColor = mix(baseColor * 0.7, baseColor * 1.2, rockPattern);
        baseColor = mix(baseColor, baseColor * 0.6, craterPattern * 0.5);

        vec2 lightDir = normalize(u_LightPos - v_Position);
        float diffuse = max(dot(normalize(-v_Position), lightDir), 0.0);
        baseColor *= (0.4 + 0.6 * diffuse);
    } else if(u_PlanetType == 2) {  // Gas giant
        float bands = noise(vec2(v_Position.y * 8.0 + u_Time * 0.1, v_Position.x * 2.0));
        float storms = noise(v_Position * 12.0 + u_Time * 0.05);
        vec3 bandColor = mix(baseColor * 0.8, baseColor * 1.2, bands);
        baseColor = mix(bandColor, baseColor * 1.3, storms * 0.3);

        float atmosphere = smoothstep(0.8, 1.0, dist);
        baseColor = mix(baseColor, baseColor * 1.3, atmosphere * 0.5);
    }

    // Add edge glow for planets only
    if(u_PlanetType == 0 || u_PlanetType == 1 || u_PlanetType == 2) {
        float edge = smoothstep(0.8, 1.0, dist);
        baseColor = mix(baseColor, baseColor * 1.2, edge * 0.5);
    }

    fragColor = vec4(baseColor, alpha);
}
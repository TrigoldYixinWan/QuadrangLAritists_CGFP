#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 cameraPos;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform Material material;

const int MAX_LIGHTS = 8;
uniform int numLights;

struct Light {
    int type;          // 0 = Directional, 1 = Point, 2 = Spotlight
    vec3 position;     // For point lights and spotlights
    vec3 direction;    // For directional lights and spotlights
    vec3 color;        // Light color
    float angle;       // For spotlights (cutoff angle in degrees)
    float penumbra;    // For spotlights (penumbra angle in degrees)
    vec3 attenuation;  // (k_c, k_l, k_q) for attenuation
};

uniform Light lights[MAX_LIGHTS];

float calculateAttenuation(Light light, float distance) {
    float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * distance + light.attenuation.z * distance * distance);
    return attenuation;
}

float calculateSpotlightEffect(Light light, vec3 L, vec3 lightDir) {
    float cosTheta = dot(-L, lightDir);
    float cosInner = cos(radians(light.angle - light.penumbra));
    float cosOuter = cos(radians(light.angle));
    float intensity = clamp((cosTheta - cosOuter) / (cosInner - cosOuter), 0.0, 1.0);
    return intensity;
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPos - FragPos);
    vec3 totalColor = vec3(0.0);

    vec3 ambient = material.ambient * 0.1; // Adjust as needed

    for (int i = 0; i < numLights; ++i) {
        Light light = lights[i];
        vec3 L;
        float attenuation = 1.0;
        float spotlightEffect = 1.0;

        if (light.type == 0) {
            // Directional Light
            L = normalize(-light.direction);
        } else if (light.type == 1) {
            // Point Light
            vec3 lightPos = light.position;
            L = normalize(lightPos - FragPos);
            float distance = length(lightPos - FragPos);
            attenuation = calculateAttenuation(light, distance);
        } else if (light.type == 2) {
            // Spotlight
            vec3 lightPos = light.position;
            vec3 lightDir = normalize(light.direction);
            L = normalize(lightPos - FragPos);
            float distance = length(lightPos - FragPos);
            attenuation = calculateAttenuation(light, distance);
            spotlightEffect = calculateSpotlightEffect(light, L, lightDir);
        }

        // Diffuse term
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = material.diffuse * diff * light.color;

        // Specular term (using Phong model)
        vec3 R = reflect(-L, N);
        float spec = 0.0;
        if (diff > 0.0) {
            float specAngle = max(dot(R, V), 0.0);
            if (specAngle > 0.0) {
                spec = pow(specAngle, material.shininess);
            }
        }
        vec3 specular = material.specular * spec * light.color;

        // Apply attenuation and spotlight effect
        diffuse *= attenuation * spotlightEffect;

        // Option 1: Modify attenuation impact on specular
        float specularAttenuation = sqrt(attenuation * spotlightEffect);
        specular *= specularAttenuation;

        // Option 2: Do not attenuate specular (uncomment if preferred)
        // specular *= 1.0;

        // Combine results
        vec3 lightColor = diffuse + specular;
        totalColor += lightColor;
    }

    totalColor += ambient;
    FragColor = vec4(totalColor, 1.0);
}

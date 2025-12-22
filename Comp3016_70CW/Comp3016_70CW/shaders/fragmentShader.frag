#version 460

// Output colour value
out vec4 FragColor;

// Inputs from vertexShader
in vec3 positionFrag;
in vec3 normalFrag;
in vec2 textureFrag;

// Texture maps
uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

// Light direction
uniform vec3 lightDir = normalize(vec3(0.5f, -1.0f, 0.3f));

// Light intensity based on day/night cycle
uniform float lightIntensity;

// Ambient light strength (ensures objects without direct lighting arent completely black)
uniform float ambientStrength = 0.3f;


void main() {
    // Normal sampled from normal map
    vec3 normalMapSample = texture(normalMap, textureFrag).rgb;
    normalMapSample = normalize(normalMapSample * 2.0f - 1.0f);

    // Combine vertex normal and normal map
    vec3 finalNormal = normalize(normalFrag + normalMapSample);

    // Diffuse shading and colour
    float diffuseFactor = max(dot(finalNormal, -lightDir), 0.0f);
    vec3 diffuse = diffuseFactor * texture(diffuseMap, textureFrag).rgb * lightIntensity;

    // Ambient lighting
    vec3 ambient = ambientStrength * texture(diffuseMap, textureFrag).rgb;

    // Final colour
    vec3 result = ambient + diffuse;
    FragColor = vec4(result, 1.0f);
}

#version 460

// Output colour value
out vec4 FragColor;

// Inputs from vertexShader
in vec3 positionFrag;
in vec3 normalFrag;
in vec2 textureFrag;

// Diffuse (texture) maps
uniform sampler2D sandDiffuse;
uniform sampler2D grassDiffuse;
uniform sampler2D rockDiffuse;
uniform sampler2D snowDiffuse;

// Normal maps
uniform sampler2D sandNormal;
uniform sampler2D grassNormal;
uniform sampler2D rockNormal;
uniform sampler2D snowNormal;

// Lighting
uniform vec3 lightDir = normalize(vec3(0.5f, -1.0f, 0.3f));
uniform float lightIntensity;
uniform float ambientStrength = 0.3f;

// Height thresholds
const float sandHeight  = -10.0f;
const float grassHeight = 2.0f;
const float rockHeight  = 13.0f;
const float snowHeight  = 23.0f;

// Blending band width between textures
const float blendWidth = 5.0f;


float Blend(float centre, float height) {
    return smoothstep(centre - blendWidth, centre + blendWidth, height);
}


void main() {
    float height = positionFrag.y;

    // Transition values
    float sandToGrass = Blend(sandHeight, height);
    float grassToRock = Blend(grassHeight, height);
    float rockToSnow  = Blend(rockHeight, height);

    // Blend weights
    float sandWeight = 1.0f - sandToGrass;
    float grassWeight = sandToGrass * (1.0f - grassToRock);
    float rockWeight = grassToRock * (1.0f - rockToSnow);
    float snowWeight = rockToSnow;

    // Normalise weights
    float total = sandWeight + grassWeight + rockWeight + snowWeight;
    sandWeight /= total;
    grassWeight /= total;
    rockWeight /= total;
    snowWeight /= total;

    // Sample diffuse textures
    vec3 sandColour = texture(sandDiffuse, textureFrag).rgb;
    vec3 grassColour = texture(grassDiffuse, textureFrag).rgb;
    vec3 rockColour = texture(rockDiffuse, textureFrag).rgb;
    vec3 snowColour = texture(snowDiffuse, textureFrag).rgb;

    vec3 finalColour =
        sandColour * sandWeight +
        grassColour * grassWeight +
        rockColour * rockWeight +
        snowColour * snowWeight;

    // Sample normal maps
    vec3 sandNormalMap = texture(sandNormal, textureFrag).rgb * 2.0f - 1.0f;
    vec3 grassNormalMap = texture(grassNormal, textureFrag).rgb * 2.0f - 1.0f;
    vec3 rockNormalMap = texture(rockNormal, textureFrag).rgb * 2.0f - 1.0f;
    vec3 snowNormalMap = texture(snowNormal, textureFrag).rgb * 2.0f - 1.0f;

    vec3 blendedNormal =
        sandNormalMap * sandWeight +
        grassNormalMap * grassWeight +
        rockNormalMap * rockWeight +
        snowNormalMap * snowWeight;

    vec3 finalNormal = normalize(normalFrag + blendedNormal);

    // Lighting
    float diffuseFactor = max(dot(finalNormal, -lightDir), 0.0f);
    vec3 diffuse = diffuseFactor * finalColour * lightIntensity;
    vec3 ambient = ambientStrength * finalColour;

    // Final colour
    vec3 result = ambient + diffuse;
    FragColor = vec4(result, 1.0f);
}

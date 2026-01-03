#version 460

// Output colour value
out vec4 FragColor;

// Inputs from vertexShader
in vec2 textureFrag;

uniform sampler2D texture_diffuse1;

// Light intensity based on day/night cycle
uniform float lightIntensity;


void main() {
    // Setting of colour coordinates to colour map
    vec4 textureColour = texture(texture_diffuse1, textureFrag);

    // Adjust based on light intensity
    textureColour.rgb *= lightIntensity;

    // Remove zero alpha pixels to allow branches to be seen behind each other
    if (textureColour.a == 0) { discard; }

    FragColor = textureColour;
}

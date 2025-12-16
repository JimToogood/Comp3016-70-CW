#version 460

// Colour value to send to next stage
out vec4 FragColor;

// Texture coordinates from last stage
in vec2 textureCoordinatesFrag;

// Texture (colours)
uniform sampler2D textureIn;

// Alpha transparency value
uniform float waterAlpha;


void main() {
    // Setting of texture & its coordinates as colour map
    vec3 colour = texture(textureIn, textureCoordinatesFrag).rgb;

    // Combine colour value and alpha value into full RGBA
    FragColor = vec4(colour, waterAlpha);
}

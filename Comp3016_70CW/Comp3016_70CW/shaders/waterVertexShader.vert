#version 460

// Vertex attributes
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 textureCoordinatesVertex;

// Texture coordinates to send
out vec2 textureCoordinatesFrag;

// Inputs from main
uniform mat4 mvpIn;
uniform float timer;

// Wave variables
uniform float waveAmplitude = 0.2f;     // Height of waves
uniform float waveFrequency = 1.8f;     // Amount of waves
uniform float waveSpeed = 0.8f;         // Speed of waves


void main() {
    vec3 displacedPos = position;

    displacedPos.y += sin(displacedPos.x * waveFrequency + timer * waveSpeed) * waveAmplitude;
    displacedPos.y += cos(displacedPos.z * waveFrequency + timer * waveSpeed) * waveAmplitude;

    // Transformation applied to vertices
    gl_Position = mvpIn * vec4(displacedPos, 1.0);

    // Sending texture coordinates to next stage
    textureCoordinatesFrag = textureCoordinatesVertex;
}

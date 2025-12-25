#version 460

// Vertex attributes
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texture;

// Texture coordinates to send
out vec2 textureFrag;

// Inputs from main
uniform mat4 mvpIn;
uniform float deltaTime;


void main() {
    textureFrag = texture;

    // Transformation applied to vertices
    gl_Position = mvpIn * vec4(position.x, position.y, position.z, 1.0f);
}

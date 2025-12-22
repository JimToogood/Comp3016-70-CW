#version 460

// Vertex attributes
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture;

// Outputs to fragmentShader
out vec3 positionFrag;
out vec3 normalFrag;
out vec2 textureFrag;

// Uniforms
uniform mat4 mvpIn;
uniform mat4 model;


void main() {
    positionFrag = vec3(model * vec4(position, 1.0f));
    normalFrag = normalize(mat3(transpose(inverse(model))) * normal);
    textureFrag = texture;

    // Transformation applied to vertices
    gl_Position = mvpIn * vec4(position, 1.0f);
}

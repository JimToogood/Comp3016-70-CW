#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm/ext/matrix_transform.hpp>
#include <string>

using namespace std;
using namespace glm;


class Game;

// All GPU data needed to render an object
struct RenderObject {
    GLuint VAO;                 // Vertex array object
    GLuint VBO;                 // Vertex buffer object
    GLuint EBO;                 // Element buffer object
    GLuint texture;             // Texture ID
    unsigned int indexCount;    // Number of indices to draw
    mat4 modelMatrix;           // Model transformation

    RenderObject() : VAO(0), VBO(0), EBO(0), texture(0), indexCount(0), modelMatrix(mat4(1.0f)) {}

    void SetPosition(const vec3& pos) {
        modelMatrix = translate(modelMatrix, pos);
    }

    void Rotate(float degrees, const vec3& axis) {
        modelMatrix = rotate(modelMatrix, radians(degrees), axis);
    }
};

// Window resize logic
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

// Function to create a textured quad
RenderObject CreateTextureQuad(float width, float height, const string& texturePath);

#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm/ext/matrix_transform.hpp>
#include <string>

using namespace std;
using namespace glm;


class Game;

// All GPU data needed to render terrain
struct RenderTerrainObject {
    GLuint VAO;                 // Vertex array object
    GLuint VBO;                 // Vertex buffer object
    GLuint EBO;                 // Element buffer object

    GLuint sandTexture;
    GLuint sandNormal;

    GLuint grassTexture;
    GLuint grassNormal;

    GLuint rockTexture;
    GLuint rockNormal;

    GLuint snowTexture;
    GLuint snowNormal;

    unsigned int indexCount;    // Number of indices to draw
    mat4 modelMatrix;           // Model transformation

    RenderTerrainObject() : 
        VAO(0), VBO(0), EBO(0), 
        sandTexture(0), sandNormal(0),
        grassTexture(0), grassNormal(0),
        rockTexture(0), rockNormal(0),
        snowTexture(0), snowNormal(0),
        indexCount(0), modelMatrix(mat4(1.0f))
    {}

    void SetPosition(const vec3& pos) {
        modelMatrix = translate(modelMatrix, pos);
    }

    void Rotate(float degrees, const vec3& axis) {
        modelMatrix = rotate(modelMatrix, radians(degrees), axis);
    }
};

// All GPU data needed to render water
struct RenderWaterObject {
    GLuint VAO;                 // Vertex array object
    GLuint VBO;                 // Vertex buffer object
    GLuint EBO;                 // Element buffer object
    GLuint texture;             // Texture ID
    unsigned int indexCount;    // Number of indices to draw
    mat4 modelMatrix;           // Model transformation
    float alpha;                // Transparency alpha value

    RenderWaterObject() : VAO(0), VBO(0), EBO(0), texture(0), indexCount(0), modelMatrix(mat4(1.0f)), alpha(1.0f) {}

    void SetPosition(const vec3& pos) {
        modelMatrix = translate(modelMatrix, pos);
    }

    void Rotate(float degrees, const vec3& axis) {
        modelMatrix = rotate(modelMatrix, radians(degrees), axis);
    }
};

// Window resize logic
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

// Function to create textured flat terrain
RenderTerrainObject CreateTerrain(
    int gridWidth, int gridDepth, float tileSize,
    const string& sandTexturePath, const string& sandNormalPath,
    const string& grassTexturePath, const string& grassNormalPath,
    const string& rockTexturePath, const string& rockNormalPath,
    const string& snowTexturePath, const string& snowNormalPath
);

// Function to create textured flat water
RenderWaterObject CreateWater(int width, int depth, float tileSize, float alpha, const string& texturePath);

// Function generate y values for terrain mapping
float GenerateHeight(float x, float z);

// Function generate normal values
vec3 GenerateNormal(float x, float z);

// Load texture image from given file location
GLuint LoadTexture(const string& texturePath);

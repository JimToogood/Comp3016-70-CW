#include <iostream>
#include <string>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glm/glm/ext/vector_float3.hpp"
#include <glm/glm/ext/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtc/noise.hpp>

#include "main.h"
#include "LoadShaders.h"

using namespace std;
using namespace glm;


class Camera {
public:
    Camera(int windowWidth, int windowHeight) :
        position(vec3(0.0f, 1.0f, 2.0f)),
        front(vec3(0.0f, 0.0f, -1.0f)),
        up(vec3(0.0f, 1.0f, 0.0f)),
        yaw(-90.0f),
        pitch(0.0f),
        lastXPos(windowWidth / 2.0f),
        lastYPos(windowHeight / 2.0f),
        mouseFirstEntry(true)
    {}

    void HandleKeyboard(GLFWwindow* window, float deltaTime) {
        const float speed = 4.0f * deltaTime;

        // Horizontal movement controls
        vec3 horizontalFront = normalize(vec3(front.x, 0.0f, front.z));
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            position += speed * horizontalFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            position -= speed * horizontalFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            position -= normalize(cross(horizontalFront, up)) * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            position += normalize(cross(horizontalFront, up)) * speed;

        // Vertical movement controls
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            position += speed * up;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            position -= speed * up;
    }

    void HandleMouse(double xpos, double ypos) {
        // Initialise last positions
        if (mouseFirstEntry) {
            lastXPos = (float)xpos;
            lastYPos = (float)ypos;
            mouseFirstEntry = false;
        }

        float xOffset = (float)xpos - lastXPos;
        float yOffset = lastYPos - (float)ypos;
        lastXPos = (float)xpos;
        lastYPos = (float)ypos;

        // Define mouse sensitivity
        const float sensitivity = 0.04f;
        xOffset *= sensitivity;
        yOffset *= sensitivity;

        yaw += xOffset;
        pitch += yOffset;

        // Clamp pitch to avoid turning up & down beyond 90 degrees
        pitch = clamp(pitch, -89.0f, 89.0f);

        // Calculate camera direction
        vec3 direction = vec3(
            cos(radians(yaw)) * cos(radians(pitch)),
            sin(radians(pitch)),
            sin(radians(yaw)) * cos(radians(pitch))
        );
        front = normalize(direction);
    }

    // Getters and Setters
    mat4 GetView() { return lookAt(position, position + front, up); }

private:
    vec3 position;
    vec3 front;
    vec3 up;

    float yaw;
    float pitch;

    float lastXPos;
    float lastYPos;
    bool mouseFirstEntry;
};

class Game {
public:
    Game() :
        window(nullptr),
        program(0),
        waterProgram(0),
        windowWidth(1280),
        windowHeight(720),
        deltaTime(0.0f),
        lastFrame(0.0f),
        waterLevel(-1.8f),
        projection(mat4(1.0f)),
        camera(windowWidth, windowHeight)
    {}

    void Initialise() {
        // Initialise GLFW
        glfwInit();

        // Create window
        window = glfwCreateWindow(windowWidth, windowHeight, "window", nullptr, nullptr);
        if (!window) {
            cerr << "Failed to initialise GLFW Window" << endl;
            glfwTerminate();
            return;
        }
        glfwMakeContextCurrent(window);
        glewInit();

        glEnable(GL_DEPTH_TEST);
        glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);    // Use the FramebufferSizeCallback function when window is resized
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);        // Automatically binds cursor to window & hides pointer

        // Enable blending for transparent objects
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // -=-=- Load shaders -=-=-
        ShaderInfo shaders[] = {
            { GL_VERTEX_SHADER, "shaders/vertexShader.vert" },
            { GL_FRAGMENT_SHADER, "shaders/fragmentShader.frag" },
            { GL_NONE, nullptr }
        };
        program = LoadShaders(shaders);

        ShaderInfo waterShaders[] = {
            { GL_VERTEX_SHADER, "shaders/waterVertexShader.vert" },
            { GL_FRAGMENT_SHADER, "shaders/waterFragmentShader.frag" },
            { GL_NONE, nullptr }
        };
        waterProgram = LoadShaders(waterShaders);

        // Set sampler uniform
        int texLoc = glGetUniformLocation(program, "textureSampler");
        glUniform1i(texLoc, 0);     // GL_TEXTURE0

        SetProjectionMatrix();

        // -=-=- Create scene objects -=-=-
        // Water
        Water = CreateWater(100, 100, 2.0f, 0.5f, "media/water.jpg");   // grid width, grid height, tile size, alpha, texture
        Water.SetPosition(vec3(0.0f, waterLevel + 0.01f, 0.0f));        // Add 0.01f to water level avoid z-fighting

        // Terrain
        Terrain = CreateTerrain(100, 100, 2.0f, "media/grass.png", "media/grass_normal.png");   // grid width, grid height, tile size, texture, normal
    }

    void HandleInput() {
        // Close window on escape pressed
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        camera.HandleKeyboard(window, deltaTime);

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        camera.HandleMouse(x, y);
    }

    void Update() {
        // Time management
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
    }

    void Render() {
        glClearColor(0.56f, 0.78f, 0.92f, 1.0f);                // RGBA Colour (normalised between 0.0f-1.0f instead of 0-255)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera view matrix sets position of the viewer, movement direction in relation to it & world up direction
        mat4 view = camera.GetView();

        // -=-=- Render Terrain -=-=-
        glUseProgram(program);

        // Build transform
        mat4 terrainMvp = projection * view * Terrain.modelMatrix;
        glUniformMatrix4fv(glGetUniformLocation(program, "mvpIn"), 1, GL_FALSE, value_ptr(terrainMvp));
        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, value_ptr(Terrain.modelMatrix));

        // Draw terrain
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Terrain.texture);
        glUniform1i(glGetUniformLocation(program, "diffuseMap"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Terrain.normalTexture);
        glUniform1i(glGetUniformLocation(program, "normalMap"), 1);

        glBindVertexArray(Terrain.VAO);
        glDrawElements(GL_TRIANGLES, Terrain.indexCount, GL_UNSIGNED_INT, nullptr);

        // -=-=- Render Water -=-=-
        glUseProgram(waterProgram);
        glDepthMask(GL_FALSE);
        
        // Build transform
        mat4 waterMvp = projection * view * Water.modelMatrix;
        glUniformMatrix4fv(glGetUniformLocation(waterProgram, "mvpIn"), 1, GL_FALSE, value_ptr(waterMvp));

        // Assign transparency alpha value
        glUniform1f(glGetUniformLocation(waterProgram, "waterAlpha"), Water.alpha);

        // Draw water
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Water.texture);
        glUniform1i(glGetUniformLocation(waterProgram, "diffuseMap"), 0);

        glBindVertexArray(Water.VAO);
        glDrawElements(GL_TRIANGLES, Water.indexCount, GL_UNSIGNED_INT, nullptr);
        glDepthMask(GL_TRUE);

        // Refreshing
        glfwSwapBuffers(window);    // Swaps the colour buffer
        glfwPollEvents();           // Queries all GLFW events
    }

    void Run() {
        while (!glfwWindowShouldClose(window)) {
            HandleInput();
            Update();
            Render();
        }
    }

    void CleanUp() {
        glfwTerminate();
    }

    // Getters and Setters
    void SetWindowSize(int width, int height) {
        windowWidth = width;
        windowHeight = height;
        SetProjectionMatrix();
    }
    void SetProjectionMatrix() {
        projection = perspective(radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 150.0f);
        glViewport(0, 0, windowWidth, windowHeight);
    }

private:
    GLFWwindow* window;
    GLuint program;
    GLuint waterProgram;

    int windowWidth;
    int windowHeight;
    float deltaTime;
    float lastFrame;
    float waterLevel;

    RenderObject Terrain;
    RenderObject Water;

    mat4 projection;
    Camera camera;
};


void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        // Resizes window based on given width and height values
        game->SetWindowSize(width, height);
    }
}

RenderObject CreateTerrain(int gridWidth, int gridDepth, float tileSize, const string& texturePath, const string& normalPath) {
    RenderObject object;

    vector<float> vertices;
    vector<unsigned int> indices;

    // Generate vertices
    for (int z = 0; z <= gridDepth; z++) {
        for (int x = 0; x <= gridWidth; x++) {
            float worldX = x * tileSize;
            float worldZ = z * tileSize;

            vec3 normal = GenerateNormal(worldX, worldZ);

            // positions
            vertices.push_back(worldX);
            vertices.push_back(GenerateHeight(worldX, worldZ));
            vertices.push_back(worldZ);

            // normals
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);

            // textures
            vertices.push_back((float)x);
            vertices.push_back((float)z);
        }
    }

    // Generate indices
    for (int z = 0; z < gridDepth; z++) {
        for (int x = 0; x < gridWidth; x++) {
            int topLeft = z * (gridWidth + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (gridWidth + 1) + x;
            int bottomRight = bottomLeft + 1;

            // first triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    object.indexCount = (unsigned int)indices.size();

    // Centre terrain at world origin
    object.modelMatrix = translate(object.modelMatrix, vec3(-gridWidth * tileSize / 2.0f, 0.0f, -gridDepth * tileSize / 2.0f));

    // Generate VAO/VBO/EBO
    glGenVertexArrays(1, &object.VAO);
    glGenBuffers(1, &object.VBO);
    glGenBuffers(1, &object.EBO);
    glBindVertexArray(object.VAO);

    // Vertex data
    glBindBuffer(GL_ARRAY_BUFFER, object.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal data
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture data
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // -=-=- Texture loading -=-=-
    glGenTextures(1, &object.texture);
    glBindTexture(GL_TEXTURE_2D, object.texture);

    // Enable texture wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int imageWidth, imageHeight, colourChannels;
    unsigned char* data = stbi_load(texturePath.c_str(), &imageWidth, &imageHeight, &colourChannels, 0);

    // If retrieval successful
    if (data) {
        // Upload texture to GPU
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        cerr << "Failed to load texture: " << texturePath << endl;
    }
    stbi_image_free(data);

    // -=-=- Normal loading -=-=-
    glGenTextures(1, &object.normalTexture);
    glBindTexture(GL_TEXTURE_2D, object.normalTexture);

    // Enable texture wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char* normalData = stbi_load(normalPath.c_str(), &imageWidth, &imageHeight, &colourChannels, 0);

    // If retrieval successful
    if (normalData) {
        // Upload normal to GPU
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, normalData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        cerr << "Failed to load normal: " << normalPath << endl;
    }
    stbi_image_free(normalData);

    glBindVertexArray(0);
    return object;
}

RenderObject CreateWater(int width, int depth, float tileSize, float alpha, const string& texturePath) {
    RenderObject object;
    object.alpha = alpha;

    float worldX = width * tileSize;
    float worldZ = depth * tileSize;

    float vertices[] = {
        // positions                      // textures
        worldX / 2,  0.0f,  worldZ / 2,   worldX, worldZ,   // top right
        worldX / 2,  0.0f, -worldZ / 2,   worldX, 0.0f,     // bottom right
       -worldX / 2,  0.0f, -worldZ / 2,   0.0f,   0.0f,     // bottom left
       -worldX / 2,  0.0f,  worldZ / 2,   0.0f,   worldZ    // top left
    };

    unsigned int indices[] = {
        0, 1, 3,    // first triangle
        1, 2, 3     // second triangle
    };
    object.indexCount = 6;

    // Generate VAO/VBO/EBO
    glGenVertexArrays(1, &object.VAO);
    glGenBuffers(1, &object.VBO);
    glGenBuffers(1, &object.EBO);
    glBindVertexArray(object.VAO);

    // Vertex data
    glBindBuffer(GL_ARRAY_BUFFER, object.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture data
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // -=-=- Texture loading -=-=-
    glGenTextures(1, &object.texture);
    glBindTexture(GL_TEXTURE_2D, object.texture);

    // Enable texture wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int imageWidth, imageHeight, colourChannels;
    unsigned char* data = stbi_load(texturePath.c_str(), &imageWidth, &imageHeight, &colourChannels, 0);

    // If retrieval successful
    if (data) {
        // Upload texture to GPU
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        cerr << "Failed to load texture: " << texturePath << endl;
    }
    stbi_image_free(data);

    glBindVertexArray(0);
    return object;
}

float GenerateHeight(float x, float z) {
    float baseFrequency = 0.05f;    // Higher value = More hills
    float baseAmplitude = 4.0f;     // Higher value = Bigger hills

    float height = 0.0f;        // Accumlated height
    float persistence = 0.35f;  // Amplitude scaling for each octave
    int octaves = 6;            // Higher value = more terrain detail

    float puddleThreshold = 0.3f;   // Minium size of water bodies

    for (int i = 0; i < octaves; i++) {
        // Frequency increases per octave
        float frequency = baseFrequency * pow(2.0f, i);

        // Amplitude decreases per octave
        float amplitude = baseAmplitude * pow(persistence, i);

        // Use glm to generate perlin noise and multiply by amplitude
        height += perlin(vec2(x * frequency, z * frequency)) * amplitude;
    }

    return height;
}

vec3 GenerateNormal(float x, float z) {
    float heightL = GenerateHeight(x - 1.0f, z);
    float heightR = GenerateHeight(x + 1.0f, z);
    float heightD = GenerateHeight(x, z - 1.0f);
    float heightU = GenerateHeight(x, z + 1.0f);

    vec3 normal = vec3(heightL - heightR, 2.0f, heightD - heightU);

    return normalize(normal);
}


int main(int argc, char* argv[]) {
    Game game;
    game.Initialise();

    game.Run();

    game.CleanUp();
    return 0;
}

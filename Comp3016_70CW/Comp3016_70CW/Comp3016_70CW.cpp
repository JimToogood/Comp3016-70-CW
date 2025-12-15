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
        const float speed = 2.0f * deltaTime;

        // Camera movement controls
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            position += speed * front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            position -= speed * front;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            position -= normalize(cross(front, up)) * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            position += normalize(cross(front, up)) * speed;
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
        windowWidth(1280),
        windowHeight(720),
        deltaTime(0.0f),
        lastFrame(0.0f),
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

        // Load shaders
        ShaderInfo shaders[] = {
            { GL_VERTEX_SHADER, "shaders/vertexShader.vert" },
            { GL_FRAGMENT_SHADER, "shaders/fragmentShader.frag" },
            { GL_NONE, nullptr }
        };
        program = LoadShaders(shaders);
        glUseProgram(program);

        mvpLocation = glGetUniformLocation(program, "mvpIn");

        // Set sampler uniform
        int texLoc = glGetUniformLocation(program, "textureSampler");
        glUniform1i(texLoc, 0);     // GL_TEXTURE0

        SetProjectionMatrix();

        // -=-=- Create scene objects -=-=-
        // Wooden Quad
        RenderObject quad = CreateTextureQuad(1.0f, 1.0f, "media/wood.jpg");    // width, height, texture file
        quad.SetPosition(vec3(0.0f, 1.07f, -2.0f));                             // x, y, z coords
        quad.Rotate(45.0f, vec3(0.0f, 1.0f, 0.0f));                             // degrees around axis
        sceneObjects.push_back(quad);

        // Metal Quad
        RenderObject quad2 = CreateTextureQuad(0.75f, 0.75f, "media/metal.jpg");
        quad2.SetPosition(vec3(1.5f, 1.1f, -3.0f));
        sceneObjects.push_back(quad2);

        // Terrain
        RenderObject terrain = CreateTerrain(10, 10, 2.0f, "media/grass.jpg");  // grid width, grid height, tile size, texture file
        sceneObjects.push_back(terrain);
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
        glClearColor(0.50f, 0.68f, 0.86f, 1.0f);                // RGBA Colour (normalised between 0.0f-1.0f instead of 0-255)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera view matrix sets position of the viewer, movement direction in relation to it & world up direction
        mat4 view = camera.GetView();

        // Render each scene object
        for (auto& object : sceneObjects) {
            // Build transform
            mat4 mvp = projection * view * object.modelMatrix;
            glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, value_ptr(mvp));

            // Draw object
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, object.texture);

            glBindVertexArray(object.VAO);
            glDrawElements(GL_TRIANGLES, object.indexCount, GL_UNSIGNED_INT, nullptr);
        }

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
        projection = perspective(radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
        glViewport(0, 0, windowWidth, windowHeight);
    }

private:
    GLFWwindow* window;
    GLuint program;

    int windowWidth;
    int windowHeight;
    float deltaTime;
    float lastFrame;

    mat4 projection;
    GLint mvpLocation;
    Camera camera;
    vector<RenderObject> sceneObjects;
};


void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        // Resizes window based on given width and height values
        game->SetWindowSize(width, height);
    }
}

RenderObject CreateTextureQuad(float width, float height, const string& texturePath) {
    RenderObject object;

    float vertices[] = {
        // positions                    // textures
        width / 2,  height / 2, 0.0f,   1.0f, 1.0f,     // top right
        width / 2, -height / 2, 0.0f,   1.0f, 0.0f,     // bottom right
       -width / 2, -height / 2, 0.0f,   0.0f, 0.0f,     // bottom left
       -width / 2,  height / 2, 0.0f,   0.0f, 1.0f      // top left
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

    // Texture coordinates data
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // -=-=- Texture loading -=-=-
    glGenTextures(1, &object.texture);
    glBindTexture(GL_TEXTURE_2D, object.texture);

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

RenderObject CreateTerrain(int gridWidth, int gridDepth, float tileSize, const string& texturePath) {
    RenderObject object;

    vector<float> vertices;
    vector<unsigned int> indices;

    // Generate vertices
    for (int z = 0; z <= gridDepth; z++) {
        for (int x = 0; x <= gridWidth; x++) {
            float worldX = x * tileSize;
            float worldZ = z * tileSize;

            // positions
            vertices.push_back(worldX);
            vertices.push_back(GenerateHeight(worldX, worldZ));
            vertices.push_back(worldZ);

            // textures
            vertices.push_back(x);
            vertices.push_back(z);
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinates data
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
    float hillFrequency = 0.15f;
    float hillAmplitude = 1.4f;

    return sin(x * hillFrequency) * cos(z * hillFrequency) * hillAmplitude;
}


int main(int argc, char* argv[]) {
    Game game;
    game.Initialise();

    game.Run();

    game.CleanUp();
    return 0;
}

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


// -=-=- Window -=-=-
int windowWidth = 1280;
int windowHeight = 720;

// -=-=- Camera -=-=-
vec3 cameraPosition = vec3(0.0f, 0.0f, 3.0f);
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);

float cameraYaw = -90.0f;   // Sideways rotation
float cameraPitch = 0.0f;   // Vertical rotation

float cameraLastXPos = windowWidth / 2.0f;  // Position of camera from previous frame
float cameraLastYPos = windowHeight / 2.0f;
bool mouseFirstEntry = true;

// -=-=- Time -=-=-
float deltaTime = 0.0f;
float lastFrame = 0.0f;     // Previous value of time change


void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    // Resizes window based on given width and height values
    glViewport(0, 0, width, height);
}

void ProcessUserInput(GLFWwindow* WindowIn) {
    // Close window on escape pressed
    if (glfwGetKey(WindowIn, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(WindowIn, true);
    }

    const float speed = 2.0f * deltaTime;

    // Camera movement controls
    if (glfwGetKey(WindowIn, GLFW_KEY_W) == GLFW_PRESS)
        cameraPosition += speed * cameraFront;
    if (glfwGetKey(WindowIn, GLFW_KEY_S) == GLFW_PRESS)
        cameraPosition -= speed * cameraFront;
    if (glfwGetKey(WindowIn, GLFW_KEY_A) == GLFW_PRESS)
        cameraPosition -= normalize(cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(WindowIn, GLFW_KEY_D) == GLFW_PRESS)
        cameraPosition += normalize(cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(WindowIn, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPosition += speed * cameraUp;
    if (glfwGetKey(WindowIn, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPosition -= speed * cameraUp;
}

void MouseCallback(GLFWwindow* window, double xpos, double ypos) {
    // Initialise last positions
    if (mouseFirstEntry) {
        cameraLastXPos = xpos;
        cameraLastYPos = ypos;
        mouseFirstEntry = false;
    }

    float xOffset = xpos - cameraLastXPos;
    float yOffset = cameraLastYPos - ypos;
    cameraLastXPos = xpos;
    cameraLastYPos = ypos;

    // Define mouse sensitivity
    const float sensitivity = 0.04f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    cameraYaw += xOffset;
    cameraPitch += yOffset;

    // Clamp pitch to avoid turning up & down beyond 90 degrees
    cameraPitch = clamp(cameraPitch, -89.0f, 89.0f);

    // Calculate camera direction
    vec3 direction = vec3(
        cos(radians(cameraYaw)) * cos(radians(cameraPitch)),
        sin(radians(cameraPitch)),
        sin(radians(cameraYaw)) * cos(radians(cameraPitch))
    );
    cameraFront = normalize(direction);
}

RenderObject CreateTextureQuad(float width, float height, const string& texturePath) {
    RenderObject object;

    float vertices[] = {
        // positions                // textures
        width/2,  height/2, 0.0f,   1.0f, 1.0f,  // top right
        width/2, -height/2, 0.0f,   1.0f, 0.0f,  // bottom right
       -width/2, -height/2, 0.0f,   0.0f, 0.0f,  // bottom left
       -width/2,  height/2, 0.0f,   0.0f, 1.0f   // top left
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
    } else {
        cerr << "Failed to load texture: " << texturePath << endl;
    }

    stbi_image_free(data);

    glBindVertexArray(0);
    return object;
}


int main() {
    // Initialise GLFW
    glfwInit();

    // Create window
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "window", NULL, NULL);
    if (!window) {
        cerr << "Failed to initialise GLFW Window" << endl;
        glfwTerminate();
        return -1;
    }

    // Bind OpenGL to window
    glfwMakeContextCurrent(window);
    glewInit();
    glViewport(0, 0, windowWidth, windowHeight);

    // Assign callbacks
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);    // Use the FramebufferSizeCallback function when window is resized
    glfwSetCursorPosCallback(window, MouseCallback);                    // Use the MouseCallback function for the mouse movement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);        // Automatically binds cursor to window & hides pointer

    // Load shaders
    ShaderInfo shaders[] = {
        { GL_VERTEX_SHADER, "shaders/vertexShader.vert" },
        { GL_FRAGMENT_SHADER, "shaders/fragmentShader.frag" },
        { GL_NONE, NULL }
    };
    program = LoadShaders(shaders);
    glUseProgram(program);

    // Set sampler uniform once
    int texLoc = glGetUniformLocation(program, "textureSampler");
    glUniform1i(texLoc, 0); // GL_TEXTURE0

    vector<RenderObject> sceneObjects;

    // Wooden Quad
    RenderObject quad = CreateTextureQuad(1.0f, 1.0f, "media/wood.jpg");        // width, height, texture file
    quad.modelMatrix = translate(quad.modelMatrix, vec3(0.0f, 0.0f, -2.0f));    // x, y, z coords
    sceneObjects.push_back(quad);

    // Metal Quad
    RenderObject quad2 = CreateTextureQuad(0.5f, 0.5f, "media/metal.jpg");
    quad2.modelMatrix = translate(quad2.modelMatrix, vec3(1.5f, 0.0f, -3.0f));
    sceneObjects.push_back(quad2);

    // Projection matrix
    mat4 projection = perspective(radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

    // Enable depth testing once before main loop
    glEnable(GL_DEPTH_TEST);

    // -=-=- Main loop -=-=-
    while (!glfwWindowShouldClose(window)) {
        // Time management
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // User Input
        ProcessUserInput(window);

        // Rendering
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);                   // RGBA Colour (normalised between 0.0f-1.0f instead of 0-255)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera view matrix sets position of the viewer, movement direction in relation to it & world up direction
        mat4 view = lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);

        // Render each scene object
        for (auto& object : sceneObjects) {
            // Build transform
            mat4 mvp = projection * view * object.modelMatrix;

            int mvpLoc = glGetUniformLocation(program, "mvpIn");
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, value_ptr(mvp));

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

    // Terminate GLFW
    glfwTerminate();
    cout << "Program exited." << endl;
    return 0;
}

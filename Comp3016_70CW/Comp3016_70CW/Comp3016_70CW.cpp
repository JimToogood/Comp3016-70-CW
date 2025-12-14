#include <iostream>
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


// VAO vertex attribute positions in correspondence to vertex attribute type
enum VAO_IDs { Triangles, Indices, Colours, Textures, NumVAOs = 2 };
GLuint VAOs[NumVAOs];       // VAOs

// Buffer types
enum Buffer_IDs { ArrayBuffer, NumBuffers = 4 };
GLuint Buffers[NumBuffers];     // Buffer objects

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
    vec3 direction;
    direction.x = cos(radians(cameraYaw)) * cos(radians(cameraPitch));
    direction.y = sin(radians(cameraPitch));
    direction.z = sin(radians(cameraYaw)) * cos(radians(cameraPitch));
    cameraFront = normalize(direction);
}


int main() {
    // Initialise GLFW
    glfwInit();

    // Create window
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "window", NULL, NULL);
    if (!window) {
        cerr << "GLFW Window failed to initialise, exiting..." << endl;
        glfwTerminate();
        return -1;
    }

    // Bind OpenGL to window
    glfwMakeContextCurrent(window);
    glewInit();
    glViewport(0, 0, 1280, 720);

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

    float vertices[] = {
        // positions            // textures
        0.5f, 0.5f, 0.5f,       1.0f, 1.0f,     // top right
        0.5f, -0.5f, 0.5f,      1.0f, 0.0f,     // bottom right
        -0.5f, -0.5f, 0.5f,     0.0f, 0.0f,     // bottom left
        -0.5f, 0.5f, 0.5f,      0.0f, 1.0f      // top left
    };

    unsigned int indices[] = {
        0, 1, 3,    // first triangle
        1, 2, 3     // second triangle
    };

    // Sets index of VAO
    glGenVertexArrays(NumVAOs, VAOs);
    // Binds VAO to a buffer
    glBindVertexArray(VAOs[0]);
    // Sets indexes of all required buffer objects
    glGenBuffers(NumBuffers, Buffers);

    // Binds vertex object to array buffer
    glBindBuffer(GL_ARRAY_BUFFER, Buffers[Triangles]);
    // Allocates buffer memory for the vertices of the 'Triangles' buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Binding and allocation for indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[Indices]);    // Buffers[Indices]
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Allocation & indexing of vertex attribute memory for vertex shader
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Colours
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbinding
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Texture index
    unsigned int texture;
    // Textures to generate
    glGenTextures(1, &texture);

    // Binding texture to type 2D texture
    glBindTexture(GL_TEXTURE_2D, texture);

    // Sets to use linear interpolation between adjacent mipmaps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // Sets to use linear interpolation upscaling (past largest mipmap texture)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Parameters that will be sent & set based on retrieved texture
    int width, height, colourChannels;
    // Retrieves texture data
    unsigned char* data = stbi_load("media/wood.jpg", &width, &height, &colourChannels, 0);

    // If retrieval successful
    if (data) {
        // Generation of texture from retrieved texture data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        // Automatically generates all required mipmaps on bound texture
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    // If retrieval unsuccessful
    else {
        cout << "Failed to load texture.\n";
        return -1;
    }

    // Clears retrieved texture from memory
    stbi_image_free(data);

    // Model matrix
    mat4 model = mat4(1.0f);
    // Scaling to zoom in
    model = scale(model, vec3(2.0f, 2.0f, 2.0f));
    // Rotation to look down
    model = rotate(model, radians(-45.0f), vec3(1.0f, 0.0f, 0.0f));
    // Movement to position further back
    model = translate(model, vec3(0.0f, 1.f, -1.5f));

    // Projection matrix
    mat4 projection = perspective(radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

    // Game loop
    while (!glfwWindowShouldClose(window)) {
        // Time management
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // User Input
        ProcessUserInput(window);

        // Rendering
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);   // RGBA Colour (normalised between 0.0f-1.0f instead of 0-255)
        glClear(GL_COLOR_BUFFER_BIT);           // Clear colour buffer

        // Transformations
        mat4 view = lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);     // Sets the position of the viewer, the movement direction in relation to it & the world up direction
        mat4 mvp = projection * view * model;
        int mvpLoc = glGetUniformLocation(program, "mvpIn");

        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, value_ptr(mvp));

        // Drawing
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAOs[0]);         // Bind buffer object to render;
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Refreshing
        glfwSwapBuffers(window);    // Swaps the colour buffer
        glfwPollEvents();           // Queries all GLFW events
    }

    // Terminate GLFW
    glfwTerminate();
    cout << "Program exited." << endl;
    return 0;
}

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "main.h"
#include "LoadShaders.h"
using namespace std;


// VAO vertex attribute positions in correspondence to vertex attribute type
enum VAO_IDs { Triangles, Indices, Colours, Textures, NumVAOs = 2 };
GLuint VAOs[NumVAOs];       // VAOs

// Buffer types
enum Buffer_IDs { ArrayBuffer, NumBuffers = 4 };
GLuint Buffers[NumBuffers];     // Buffer objects

// Define shaders program
GLuint program;


void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    // Resizes window based on given width and height values
    glViewport(0, 0, width, height);
}

void ProcessUserInput(GLFWwindow* WindowIn) {
    // If Escape pressed
    if (glfwGetKey(WindowIn, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        // Tell window to close
        glfwSetWindowShouldClose(WindowIn, true);
    }
    // If E pressed
    else if (glfwGetKey(WindowIn, GLFW_KEY_E) == GLFW_PRESS) {
        cout << "E pressed" << endl;
    }
}


int main() {
    // Define variables
    const int WIN_WIDTH = 800;
    const int WIN_HEIGHT = 600;

    // Initialise GLFW
    glfwInit();

    // Initialise GLFWwindow object
    GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "test window", NULL, NULL);

    // Check if window has been successfully instantiated
    if (window == NULL) {
        cerr << "GLFW Window did not instantiate, exiting..." << endl;
        glfwTerminate();
        return -1;
    }

    // Bind OpenGL to window
    glfwMakeContextCurrent(window);
    glewInit();
    glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

    // Load shaders
    ShaderInfo shaders[] = {
        { GL_VERTEX_SHADER, "shaders/vertexShader.vert" },
        { GL_FRAGMENT_SHADER, "shaders/fragmentShader.frag" },
        { GL_NONE, NULL }
    };

    program = LoadShaders(shaders);
    glUseProgram(program);

    // Use the FramebufferSizeCallback function when window is resized
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    float vertices[] = {
        // positions             // colours
        0.5f, 0.5f, 0.0f,       1.0f, 0.0f, 0.0f,   // top right
        0.5f, -0.5f, 0.0f,      0.0f, 1.0f, 0.0f,   // bottom right
        -0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f,   // bottom left
        -0.5f, 0.5f, 0.0f,      1.0f, 1.0f, 1.0f    // top left
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Colours
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbinding
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Program loop
    while (!glfwWindowShouldClose(window)) {
        // User Input
        ProcessUserInput(window);

        // Rendering
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);   // RGBA Colour (normalised between 0.0f-1.0f instead of 0-255)
        glClear(GL_COLOR_BUFFER_BIT);           // Clear the colour buffer

        glBindVertexArray(VAOs[0]);             // Bind buffer object to render
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Refreshing
        glfwSwapBuffers(window);    // Swap the colour buffer
        glfwPollEvents();           // Query all GLFW events
    }

    // Terminate GLFW
    glfwTerminate();
    cout << "Program finished without error." << endl;
    return 0;
}

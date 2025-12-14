#pragma once
#include <GLFW/glfw3.h>

// Window resize function
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

// Mouse movement function
void MouseCallback(GLFWwindow* window, double xpos, double ypos);

// Manages inputs from user
void ProcessUserInput(GLFWwindow* WindowIn);

// Define shaders program
GLuint program;

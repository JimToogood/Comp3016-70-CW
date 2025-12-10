#pragma once
#include <GLFW/glfw3.h>

// Window resize function
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

// Manages inputs from user
void ProcessUserInput(GLFWwindow* WindowIn);

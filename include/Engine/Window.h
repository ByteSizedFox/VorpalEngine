#pragma once
#include <GLFW/glfw3.h>
#include <stdio.h>

class Window {
public:
    GLFWwindow *m_window;
    void initWindow(uint32_t WIDTH, uint32_t HEIGHT);
    bool shouldClose();
    void destroy();
    GLFWwindow *get();
    void getFramebufferSize(int *width, int *height);
    
};
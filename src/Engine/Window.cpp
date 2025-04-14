#include "Engine/Window.hpp"
#include "config.h"
#include "glm/gtc/matrix_transform.hpp"

#include <stdio.h>

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

#include <cstring>

void Window::init(int WIDTH, int HEIGHT) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, scroll_callback);

    // DO NOT CHANGE, this fixes a conflict with the two renderpasses, only change if you're fixing the issue
    glfwSetWindowSizeLimits(window, UI_WIDTH, UI_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
    
    // mouse and kb capture
    glfwSetWindowFocusCallback(window, [](GLFWwindow* window, int focused) {
        if (focused) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        }
    });

    // update initial projection matrix
    Engine::projectionMatrix = glm::perspective(glm::radians(45.0f), (float) WIDTH / (float) HEIGHT, 0.001f, 1000.0f);

    // mouse and kb capture
    glfwSetKeyCallback(window, key_callback);
    memset(key_pressed, false, 1024);
}
bool Window::ShouldClose() {
    return glfwWindowShouldClose(window);
}
void Window::destroy() {
    glfwDestroyWindow(window);
}
void Window::GetFramebufferSize(int *width, int *height) {
    glfwGetFramebufferSize(window, width, height);
}

void Window::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));

    if (app->first_mouse) {
        app->last_mouse_pos = {xpos, ypos};
        app->first_mouse = false;
    }

    int focused = glfwGetWindowAttrib(window, GLFW_FOCUSED);
    if (focused != app->last_focus) {
        app->first_mouse = true;
        app->last_focus = focused;
        return;
    }

    app->mouse_pos = {xpos, ypos};
}

glm::vec2 Window::getMouseVector() {
    if (first_mouse) {
        return {0.0f, 0.0f};
    }
    
    float xoffset = last_mouse_pos.x - mouse_pos.x;
    float yoffset = last_mouse_pos.y - mouse_pos.y; // Reversed since y-coordinates range from bottom to top

    if (key_pressed[GLFW_KEY_F]) { // smoothing test
        last_mouse_pos = (mouse_pos + last_mouse_pos) / glm::vec2(2.0f);
    } else {
        last_mouse_pos = mouse_pos;
    }

    last_mouseVec = {xoffset, yoffset};

    return {xoffset, -yoffset};
}
void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS ||action == GLFW_RELEASE) {
        app->key_pressed[key] = (action == GLFW_PRESS);
    }
}

#include "imgui.h"
#include <stdio.h>

void Window::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < ImGuiMouseButton_COUNT)
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
}

void Window::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent((float) xoffset, (float) yoffset);
}

#include "Engine/Window.hpp"

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void Window::init(int WIDTH, int HEIGHT) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    
    // mouse and kb capture
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, key_callback);
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
    last_mouse_pos = mouse_pos;

    return {xoffset, -yoffset};
}
void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    //auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    //if (action == GLFW_PRESS ||action == GLFW_RELEASE) {
    //    app->pressed_keys[key] = (action == GLFW_PRESS);
    //}
}
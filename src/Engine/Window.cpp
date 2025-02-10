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
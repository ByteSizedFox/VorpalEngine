#include "Engine/Window.h"
#include "Engine/Engine.h"

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    Engine::framebufferResized = true;
}

void Window::initWindow(uint32_t WIDTH, uint32_t HEIGHT) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

bool Window::shouldClose() {
    return glfwWindowShouldClose(m_window);
}

void Window::destroy() {
    glfwDestroyWindow(m_window);
}

GLFWwindow *Window::get() {
    return m_window;
}
void Window::getFramebufferSize(int *width, int *height) {
    glfwGetFramebufferSize(m_window, width, height);
}
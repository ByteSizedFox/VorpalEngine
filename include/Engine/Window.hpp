#include <volk.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window {
public:
    bool framebufferResized = false;
    GLFWwindow* window;
    void init(int WIDTH, int HEIGHT);
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    bool ShouldClose();
    void destroy();
    void GetFramebufferSize(int *width, int *height);
    GLFWwindow* get() {
        return window;
    }
};
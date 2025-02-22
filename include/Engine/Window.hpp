#include <volk.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

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
    glm::vec2 getMouseVector();
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    glm::vec2 mouse_pos;
    glm::vec2 last_mouse_pos;
    int last_focus = 0;
    bool first_mouse;
    bool key_pressed[1024];
};
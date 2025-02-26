#include <volk.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <functional>

#include "Engine/Camera.hpp"

class Window {
private:
    bool framebufferResized = true;
    GLFWwindow* window;
    glm::vec2 mouse_pos;
    glm::vec2 last_mouse_pos;
    glm::vec2 last_mouseVec;
    
    int last_focus = 0;
    bool first_mouse;
    bool key_pressed[400];

    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    glm::mat4 projectionMatrix;

    std::function<void(double, double)> userMouseCallback;

public:
    Camera camera;

    void init(int WIDTH, int HEIGHT);
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    bool ShouldClose();
    void destroy();
    void GetFramebufferSize(int *width, int *height);
    GLFWwindow* get() {
        return window;
    }

    // input related functions
    glm::vec2 getMouseVector();
    glm::vec2 getMousePos() {
        return mouse_pos;
    }

    bool isKeyPressed(int key) {
        return key_pressed[key];
    }
    bool wasResized() {
        return framebufferResized;
    }
    void clearResized() {
        framebufferResized = false;
    }
    glm::mat4 getProjectionMatrix() {
        return projectionMatrix;
    }
    void updateProjectionMatrix(int width, int height) {
        projectionMatrix = glm::perspective(glm::radians(45.0f), (float) width / (float) height, 0.001f, 10.0f);
    }
    void setUserMouseCallback(std::function<void(double, double)> callback) {
        userMouseCallback = callback;
    }
    Camera *getCamera() {
        return &camera;
    }
};
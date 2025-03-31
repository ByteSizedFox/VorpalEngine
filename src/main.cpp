#include "Engine/Renderer.hpp"
#include "Game/MainScene.hpp"
#include "Game/TestScene.hpp"

extern const char _binary_a_bin_start[];
extern const char _binary_a_bin_end[];

Renderer renderer;
MainScene scene;
TestScene scene1;
bool tmp = true;

void mainLoop() {
    renderer.setScene(&scene);

    while (!renderer.window.ShouldClose()) {
        double now = glfwGetTime();
        renderer.frames++;

        glfwPollEvents();

        renderer.drawFrame();

        if (renderer.window.isKeyPressed(GLFW_KEY_G) && tmp == true) {            
            renderer.setScene(&scene1);
            tmp = false;
            continue;
        }
        if (renderer.window.isKeyPressed(GLFW_KEY_H) && tmp == false) {            
            renderer.setScene(nullptr);
            tmp = true;
            continue;
        }

        // move camera
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            renderer.updateUniformBuffer(i);
        }
        renderer.handle_input();

        if (now - renderer.last_time >= 1.0) {
            renderer.fps = renderer.frames;
            renderer.frames = 0;
            renderer.last_time = now;
            printf("FPS: %i\n", renderer.fps);
            renderer.uiNeedsUpdate = true;
        }
    }
    vkDeviceWaitIdle(VK::device);

    // stop scenes
    scene.destroy();
    scene1.destroy();
}

int main() {
    // test, NOTE: this code loads a model from a zip archive
    const size_t size = _binary_a_bin_end - _binary_a_bin_start;
    Utils::initIOSystem(_binary_a_bin_start, size);

    try {
        renderer.run();
        mainLoop();
        renderer.cleanup();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
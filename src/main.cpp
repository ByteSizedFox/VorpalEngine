#include "Engine/Renderer.hpp"
#include "Game/MenuScene.hpp"
#include "Game/MainScene.hpp"
#include "Game/TestScene.hpp"
#include "Engine/LuaScene.hpp"
#include "Engine/AudioManager.hpp"

#include <fstream>
#include <vector>
#include <stdexcept>

static std::vector<char> g_assetData;

Renderer renderer;
bool tmp = true;

void setup() {
    AudioManager::get().init();
    renderer.run();
    renderer.setScene(new LuaScene("assets/scripts/game.lua"));
    Logger::success("MAIN", "Loading Finished!");
}
void loop(double deltaTime) {
    if (renderer.window.isKeyPressed(GLFW_KEY_1)) {
        vkDeviceWaitIdle(VK::device);
        renderer.recreateRender(true);
        return;
    }
    if (renderer.window.isKeyPressed(GLFW_KEY_2)) {
        vkDeviceWaitIdle(VK::device);
        renderer.recreateRender(false);
        return;
    }

    renderer.drawFrame();

    if (renderer.window.isKeyPressed(GLFW_KEY_G) && tmp == true) {            
        renderer.setScene(new MainScene());
        tmp = false;
        return;
    }
    if (renderer.window.isKeyPressed(GLFW_KEY_H) && tmp == false) {            
        renderer.setScene(new TestScene());
        tmp = true;
        return;
    }

    if (renderer.window.isKeyPressed(GLFW_KEY_N)) {            
        Engine::enableNormal = 1;
    }
    if (renderer.window.isKeyPressed(GLFW_KEY_M)){            
        Engine::enableNormal = 0;
    }

    // move camera
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        renderer.updateUniformBuffer(i);
    }

    // user input
    renderer.handle_input();
}
void cleanup() {
    renderer.cleanup();
    AudioManager::get().shutdown();
}

void mainLoop() {
    double fpsLastTime = engineGetTime();
    double deltaTimeLastTime = engineGetTime();
    double deltaTime = 0.0;
    int frames = 0;
    int fps = 0;

    while (!renderer.window.ShouldClose()) {
        // FIXME: dangerous pointer casting, make better scene queue system
        Scene *nextScene = static_cast<Scene *>(Engine::nextScene);
        if (nextScene != nullptr) { // a scene is queued for loading
            nextScene->init();
            renderer.setScene(nextScene);
            Engine::nextScene = nullptr; // scene is loaded, clear queue
        }

        // window Events
        glfwPollEvents();

        // calculate deltaTime
        double now = engineGetTime();
        deltaTime = now - deltaTimeLastTime;
        deltaTimeLastTime = now;
        renderer.deltaTime = deltaTime;

        // count FPS
        frames++;
        if (now - fpsLastTime >= 1.0) {
            fps = frames;
            frames = 0;
            fpsLastTime = now;
            printf("FPS: %i\n", fps);
            renderer.uiNeedsUpdate = true;
        }
        
        renderer.drawFrame();
        loop(deltaTime);
    }
    vkDeviceWaitIdle(VK::device);
}

int main() {
    Logger::info("MAIN", "Loading...");
    // test, NOTE: this code loads a model from a zip archive
    {
        std::ifstream f("assets.zip", std::ios::binary | std::ios::ate);
        if (!f) throw std::runtime_error("assets.zip not found — run from the build directory");
        g_assetData.resize(f.tellg());
        f.seekg(0);
        f.read(g_assetData.data(), g_assetData.size());
    }
    Utils::initIOSystem(g_assetData.data(), g_assetData.size());

    try {
        setup();
        mainLoop();
        cleanup();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
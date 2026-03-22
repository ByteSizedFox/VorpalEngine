#pragma once
#include "Engine/Scene.hpp"
#include "Engine/LuaManager.hpp"
#include <string>

class LuaScene : public Scene {
public:
    LuaScene(const std::string& scriptPath);
    void setup() override;
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Window *window) override;
    void drawUI(Window *window) override;

    // Lua-friendly draw method
    void lua_draw(Window* window) {
        Scene::draw(currentCommandBuffer, currentPipelineLayout, window);
    }

    // These are used to allow Lua to call scene:draw() without needing to pass handles
    VkCommandBuffer currentCommandBuffer;
    VkPipelineLayout currentPipelineLayout;

private:
    std::string scriptPath;
    sol::function setupFunc;
    sol::function drawFunc;
    sol::function drawUIFunc;
};

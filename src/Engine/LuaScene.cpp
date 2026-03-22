#include "Engine/LuaScene.hpp"

void Scene::load_lua_scene(const char* scriptPath) {
    loadScene(new LuaScene(scriptPath));
}

LuaScene::LuaScene(const std::string& scriptPath) : scriptPath(scriptPath) {
}

void LuaScene::setup() {
    LuaManager::init();
    
    sol::protected_function_result result;

    // Try loading from zip first
    std::vector<char> scriptData = Utils::readFileZip(scriptPath);
    if (!scriptData.empty()) {
        result = LuaManager::lua.safe_script(std::string(scriptData.data(), scriptData.size()), sol::script_pass_on_error);
    } else {
        // Fallback to filesystem
        result = LuaManager::lua.safe_script_file(scriptPath, sol::script_pass_on_error);
    }

    if (!result.valid()) {
        sol::error err = result;
        Logger::error("LuaScene", (std::string("Failed to load script: ") + err.what()).c_str());
        return;
    }

    // Get functions
    setupFunc = LuaManager::lua["setup"];
    drawFunc = LuaManager::lua["draw"];
    drawUIFunc = LuaManager::lua["drawUI"];

    if (setupFunc.valid()) {
        setupFunc(this);
    }
}

void LuaScene::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Window *window) {
    currentCommandBuffer = commandBuffer;
    currentPipelineLayout = pipelineLayout;
    if (drawFunc.valid()) {
        drawFunc(this, window);
    } else {
        // Fallback to default draw
        Scene::draw(commandBuffer, pipelineLayout, window);
    }
}

void LuaScene::drawUI(Window *window) {
    if (drawUIFunc.valid()) {
        drawUIFunc(this, window);
    } else {
        Scene::drawUI(window);
    }
}

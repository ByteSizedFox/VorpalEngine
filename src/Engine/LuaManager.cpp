#include "Engine/LuaManager.hpp"
#include "Engine/Scene.hpp"
#include "Engine/LuaScene.hpp"
#include "config.h"
#include <GLFW/glfw3.h>

sol::state LuaManager::lua;

void LuaManager::init() {
    static bool initialized = false;
    if (initialized) return;

    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::os);

    bindGLM(lua);
    bindImGui(lua);
    bindEngine(lua);

    initialized = true;
}

void LuaManager::bindGLM(sol::state& lua) {
    lua.new_usertype<glm::vec3>("vec3",
        sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z,
        "length", [](const glm::vec3& v) { return glm::length(v); },
        sol::meta_function::addition, [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
        sol::meta_function::multiplication, [](const glm::vec3& a, float b) { return a * b; }
    );

    lua.set_function("radians", [](float degrees) { return glm::radians(degrees); });
}

void LuaManager::bindImGui(sol::state& lua) {
    auto imgui = lua.create_table("imgui");
    imgui.set_function("begin", [](const char* name) { return ImGui::Begin(name); });
    imgui.set_function("window_end", []() { ImGui::End(); });
    imgui.set_function("text", [](const char* text) { ImGui::Text("%s", text); });
    imgui.set_function("button", [](const char* label) { return ImGui::Button(label); });
    imgui.set_function("set_next_window_size", [](float x, float y) { ImGui::SetNextWindowSize(ImVec2(x, y)); });
    imgui.set_function("set_next_window_pos", [](float x, float y) { ImGui::SetNextWindowPos(ImVec2(x, y)); });
}

void LuaManager::bindEngine(sol::state& lua) {
    lua.new_enum("ColliderType",
        "BOX", ColliderType::BOX,
        "CONVEXHULL", ColliderType::CONVEXHULL,
        "TRIMESH", ColliderType::TRIMESH
    );

    lua.new_usertype<Mesh3D>("Mesh3D",
        sol::constructors<Mesh3D()>(),
        "init", &Mesh3D::init,
        "setPosition", &Mesh3D::setPosition,
        "setRotation", &Mesh3D::setRotation,
        "setOrientation", &Mesh3D::setOrientation,
        "createRigidBody", &Mesh3D::createRigidBody
    );

    lua.new_usertype<Camera>("Camera",
        "getPosition", &Camera::getPosition,
        "setPosition", &Camera::setPosition,
        "getForward", &Camera::getForward,
        "getOrientation", &Camera::getOrientation
    );

    lua.new_usertype<Window>("Window",
        "isKeyPressed", &Window::isKeyPressed,
        "setShouldClose", &Window::setShouldClose
    );

    // Keys
    lua.set("KEY_E", GLFW_KEY_E);
    lua.set("KEY_ESCAPE", GLFW_KEY_ESCAPE);

    // UI Constants
    lua.set("UI_WIDTH", UI_WIDTH);
    lua.set("UI_HEIGHT", UI_HEIGHT);

    lua.new_usertype<Scene>("Scene",
        "add_object", &Scene::add_object,
        "create_object", &Scene::create_object,
        "load_lua_scene", &Scene::load_lua_scene,
        "handleUIInteraction", &Scene::handleUIInteraction,
        "camera", &Scene::camera,
        "uiMesh", &Scene::uiMesh
    );

    lua.new_usertype<LuaScene>("LuaScene",
        sol::base_classes, sol::bases<Scene>(),
        "draw", &LuaScene::lua_draw
    );
}

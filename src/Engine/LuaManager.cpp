#include "Engine/LuaManager.hpp"
#include "Engine/Scene.hpp"
#include "Engine/LuaScene.hpp"
#include "config.h"
#include <GLFW/glfw3.h>

sol::state LuaManager::lua;

void LuaManager::init() {
    static bool initialized = false;
    if (initialized) return;

    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
                       sol::lib::string, sol::lib::table, sol::lib::os);

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

        // length
        "length", [](const glm::vec3& v) { return glm::length(v); },

        // normalize → returns a new vec3
        "normalize", [](const glm::vec3& v) {
            float l = glm::length(v);
            if (l < 0.0001f) return glm::vec3(0.0f);
            return v / l;
        },

        // dot product
        "dot", [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); },

        // cross product
        "cross", [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); },

        // distance to another vec3
        "distance", [](const glm::vec3& a, const glm::vec3& b) { return glm::distance(a, b); },

        // arithmetic
        sol::meta_function::addition,       [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
        sol::meta_function::subtraction,    [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
        sol::meta_function::multiplication, sol::overload(
            [](const glm::vec3& a, float b) { return a * b; },
            [](float a, const glm::vec3& b) { return a * b; }
        ),
        sol::meta_function::division,       [](const glm::vec3& a, float b) { return a / b; },
        sol::meta_function::unary_minus,    [](const glm::vec3& a) { return -a; },

        // to_string for debug prints
        sol::meta_function::to_string, [](const glm::vec3& v) {
            char buf[64];
            snprintf(buf, sizeof(buf), "vec3(%.3f, %.3f, %.3f)", v.x, v.y, v.z);
            return std::string(buf);
        }
    );

    lua.set_function("radians", [](float degrees) { return glm::radians(degrees); });
    lua.set_function("degrees", [](float rad)     { return glm::degrees(rad); });
    lua.set_function("vec3_lerp", [](const glm::vec3& a, const glm::vec3& b, float t) {
        return glm::mix(a, b, t);
    });
}

void LuaManager::bindImGui(sol::state& lua) {
    auto imgui = lua.create_table("imgui");

    imgui.set_function("begin",               [](const char* name) { return ImGui::Begin(name); });
    imgui.set_function("window_end",          []() { ImGui::End(); });
    imgui.set_function("text",                [](const char* text) { ImGui::Text("%s", text); });
    imgui.set_function("text_colored", [](float r, float g, float b, float a, const char* text) {
        ImGui::TextColored(ImVec4(r, g, b, a), "%s", text);
    });
    imgui.set_function("button",              [](const char* label) { return ImGui::Button(label); });
    imgui.set_function("set_next_window_size",[](float x, float y) { ImGui::SetNextWindowSize(ImVec2(x, y)); });
    imgui.set_function("set_next_window_pos", [](float x, float y) { ImGui::SetNextWindowPos(ImVec2(x, y)); });
    imgui.set_function("separator",           []() { ImGui::Separator(); });
    imgui.set_function("same_line",           []() { ImGui::SameLine(); });
    imgui.set_function("spacing",             []() { ImGui::Spacing(); });
    imgui.set_function("dummy", [](float w, float h) { ImGui::Dummy(ImVec2(w, h)); });
    imgui.set_function("progress_bar", sol::overload(
        [](float fraction) { ImGui::ProgressBar(fraction); },
        [](float fraction, const char* overlay) { ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), overlay); }
    ));
}

void LuaManager::bindEngine(sol::state& lua) {
    lua.new_enum("ColliderType",
        "BOX",       ColliderType::BOX,
        "CONVEXHULL",ColliderType::CONVEXHULL,
        "TRIMESH",   ColliderType::TRIMESH
    );

    // -------------------------------------------------------------------------
    // Mesh3D
    // -------------------------------------------------------------------------
    lua.new_usertype<Mesh3D>("Mesh3D",
        sol::constructors<Mesh3D()>(),
        "init",               &Mesh3D::init,
        "setPosition",        &Mesh3D::setPosition,
        "getPosition",        &Mesh3D::getPosition,
        "getScale",           &Mesh3D::getScale,
        "setScale", sol::overload(
            static_cast<void (Mesh3D::*)(glm::vec3)>(&Mesh3D::setScale),
            [](Mesh3D& m, float x, float y, float z) { m.setScale(glm::vec3(x, y, z)); }
        ),
        "setRotation",        &Mesh3D::setRotation,
        "getRotation",        &Mesh3D::getRotation,
        "setOrientation",     &Mesh3D::setOrientation,
        "setVisible",         &Mesh3D::setVisible,
        "isVisible",          &Mesh3D::isVisible,
        "createRigidBody",    &Mesh3D::createRigidBody,
        "setLinearVelocity",  &Mesh3D::setLinearVelocity,
        "getLinearVelocity",  &Mesh3D::getLinearVelocity,
        "getPhysicsPosition", &Mesh3D::getPhysicsPosition,
        "setFriction",        &Mesh3D::setFriction,
        "setRestitution",     &Mesh3D::setRestitution,
        "setDamping",         &Mesh3D::setDamping
    );

    // -------------------------------------------------------------------------
    // SkinnedMesh3D — extends Mesh3D; only animation/capsule methods listed here
    // -------------------------------------------------------------------------
    lua.new_usertype<SkinnedMesh3D>("SkinnedMesh3D",
        sol::constructors<SkinnedMesh3D()>(),
        sol::base_classes, sol::bases<Mesh3D>(),
        "init",             &SkinnedMesh3D::init,
        "playAnimation",    &SkinnedMesh3D::playAnimation,
        "setLooping",       &SkinnedMesh3D::setLooping,
        "setAnimSpeed",     &SkinnedMesh3D::setAnimSpeed,
        "getAnimSpeed",     &SkinnedMesh3D::getAnimSpeed,
        "getAnimDuration",  &SkinnedMesh3D::getAnimDuration,
        "getJointCount",    &SkinnedMesh3D::getJointCount,
        "getTestBoneIndex", &SkinnedMesh3D::getTestBoneIndex,
        "getAnimCount",     &SkinnedMesh3D::getAnimCount,
        "getAnimTime",      &SkinnedMesh3D::getAnimTime,
        "createCapsuleRigidBody", sol::overload(
            [](SkinnedMesh3D& m, float mass) { m.createCapsuleRigidBody(mass); },
            [](SkinnedMesh3D& m, float mass, float radius) { m.createCapsuleRigidBody(mass, radius); },
            [](SkinnedMesh3D& m, float mass, float radius, float height) { m.createCapsuleRigidBody(mass, radius, height); }
        )
    );

    // -------------------------------------------------------------------------
    // Camera
    // -------------------------------------------------------------------------
    lua.new_usertype<Camera>("Camera",
        "getPosition",    &Camera::getPosition,
        "setPosition",    &Camera::setPosition,
        "getForward",     &Camera::getForward,
        "getRight",       &Camera::getRight,
        "getUp",          &Camera::getUp,
        "getVelocity",    &Camera::getVelocity,
        "getOrientation", &Camera::getOrientation,
        "setOrientation", &Camera::setOrientation
    );

    // -------------------------------------------------------------------------
    // Window
    // -------------------------------------------------------------------------
    lua.new_usertype<Window>("Window",
        "isKeyPressed",   &Window::isKeyPressed,
        "setShouldClose", &Window::setShouldClose,
        "getMousePos",    &Window::getMousePos,
        "getWidth",       &Window::getWidth,
        "getHeight",      &Window::getHeight
    );

    // -------------------------------------------------------------------------
    // Scene
    // -------------------------------------------------------------------------
    lua.new_usertype<Scene>("Scene",
        "add_object",            &Scene::add_object,
        "create_object",         &Scene::create_object,
        "create_skinned_object", &Scene::create_skinned_object,
        "remove_object",         &Scene::remove_object,
        "remove_skinned_object", &Scene::remove_skinned_object,
        "registerPhysics",       &Scene::registerPhysics,
        "load_lua_scene",        &Scene::load_lua_scene,
        "handleUIInteraction",   &Scene::handleUIInteraction,
        "camera",                &Scene::camera,
        "uiMesh",                &Scene::uiMesh
    );

    lua.new_usertype<LuaScene>("LuaScene",
        sol::base_classes, sol::bases<Scene>(),
        "draw", &LuaScene::lua_draw
    );

    // -------------------------------------------------------------------------
    // Keys
    // -------------------------------------------------------------------------
    lua.set("KEY_ESCAPE",  GLFW_KEY_ESCAPE);
    lua.set("KEY_SPACE",   GLFW_KEY_SPACE);
    lua.set("KEY_TAB",     GLFW_KEY_TAB);
    lua.set("KEY_ENTER",   GLFW_KEY_ENTER);

    lua.set("KEY_W",       GLFW_KEY_W);
    lua.set("KEY_A",       GLFW_KEY_A);
    lua.set("KEY_S",       GLFW_KEY_S);
    lua.set("KEY_D",       GLFW_KEY_D);
    lua.set("KEY_E",       GLFW_KEY_E);
    lua.set("KEY_Q",       GLFW_KEY_Q);
    lua.set("KEY_R",       GLFW_KEY_R);
    lua.set("KEY_F",       GLFW_KEY_F);
    lua.set("KEY_C",       GLFW_KEY_C);
    lua.set("KEY_G",       GLFW_KEY_G);
    lua.set("KEY_H",       GLFW_KEY_H);
    lua.set("KEY_Z",       GLFW_KEY_Z);
    lua.set("KEY_X",       GLFW_KEY_X);
    lua.set("KEY_V",       GLFW_KEY_V);
    lua.set("KEY_SHIFT",   GLFW_KEY_LEFT_SHIFT);
    lua.set("KEY_CTRL",    GLFW_KEY_LEFT_CONTROL);
    lua.set("KEY_ALT",     GLFW_KEY_LEFT_ALT);

    lua.set("KEY_1",       GLFW_KEY_1);
    lua.set("KEY_2",       GLFW_KEY_2);
    lua.set("KEY_3",       GLFW_KEY_3);
    lua.set("KEY_4",       GLFW_KEY_4);
    lua.set("KEY_5",       GLFW_KEY_5);
    lua.set("KEY_6",       GLFW_KEY_6);
    lua.set("KEY_7",       GLFW_KEY_7);
    lua.set("KEY_8",       GLFW_KEY_8);
    lua.set("KEY_9",       GLFW_KEY_9);
    lua.set("KEY_0",       GLFW_KEY_0);

    lua.set("KEY_UP",      GLFW_KEY_UP);
    lua.set("KEY_DOWN",    GLFW_KEY_DOWN);
    lua.set("KEY_LEFT",    GLFW_KEY_LEFT);
    lua.set("KEY_RIGHT",   GLFW_KEY_RIGHT);

    lua.set("KEY_F1",      GLFW_KEY_F1);
    lua.set("KEY_F2",      GLFW_KEY_F2);
    lua.set("KEY_F3",      GLFW_KEY_F3);
    lua.set("KEY_F4",      GLFW_KEY_F4);
    lua.set("KEY_F5",      GLFW_KEY_F5);

    // -------------------------------------------------------------------------
    // UI constants
    // -------------------------------------------------------------------------
    lua.set("UI_WIDTH",  UI_WIDTH);
    lua.set("UI_HEIGHT", UI_HEIGHT);

    // -------------------------------------------------------------------------
    // Global light
    // -------------------------------------------------------------------------
    lua.set_function("set_light_pos", [](float x, float y, float z) {
        Engine::lightPos = glm::vec3(x, y, z);
    });

    // -------------------------------------------------------------------------
    // Time
    // -------------------------------------------------------------------------
    lua.set_function("get_time", []() {
        return glfwGetTime();
    });

    // get_delta_time: tracks its own previous time so Lua doesn't have to
    lua.set_function("get_delta_time", []() -> float {
        static double prev = glfwGetTime();
        double now = glfwGetTime();
        float dt = (float)(now - prev);
        prev = now;
        return dt < 0.0f ? 0.0f : (dt > 0.1f ? 0.1f : dt);
    });

    // -------------------------------------------------------------------------
    // Math helpers
    // -------------------------------------------------------------------------
    lua.set_function("clamp", [](float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    });
    lua.set_function("lerp", [](float a, float b, float t) {
        return a + (b - a) * t;
    });
    lua.set_function("sign", [](float v) -> float {
        return v < 0.0f ? -1.0f : (v > 0.0f ? 1.0f : 0.0f);
    });
}

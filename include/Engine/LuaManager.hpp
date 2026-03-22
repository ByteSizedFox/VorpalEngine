#pragma once
#include <sol/sol.hpp>
#include <glm/glm.hpp>
#include <imgui.h>
#include "Engine/Mesh3D.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Window.hpp"

class LuaManager {
public:
    static sol::state lua;

    static void init();
    static void bindEngine(sol::state& lua);
    static void bindGLM(sol::state& lua);
    static void bindImGui(sol::state& lua);
};

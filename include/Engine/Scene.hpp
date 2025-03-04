#pragma once

#include <stdio.h>
#include <vector>
#include <atomic>
#include <thread>

#include "Mesh3D.hpp"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"

class Scene {
private:
    std::vector<Mesh3D*> meshes;
    std::thread loadThread;
public:
    std::vector<Texture> textures;

    bool isReady = false;

    void init() {
        for (Mesh3D *mesh : meshes) {
            mesh->loadModel(mesh->fileName.c_str());
        }
        textures.resize(VK::textureMap.size());
        for (auto& tex: VK::textureMap) {
            textures[tex.second.textureID] = tex.second;
            printf("TextureID: %i\n", tex.second.textureID);
        }
        isReady = true;
    }
    void loop() {
        printf("WARNING: Unimplemented Loop!\n");
    }
    void drawFrame() {
        printf("WARNING: Unimplemented DrawFrame!\n");
    }

    void destroy() {
        for (int i = 0; i < textures.size(); i++) {
            Texture *texture = &textures[i];
            texture->destroy();
        }
    }

    void add_object(Mesh3D *mesh) {
        meshes.push_back(mesh);
    }
    
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
        for (Mesh3D *mesh : meshes) {
            mesh->draw(commandBuffer, pipelineLayout);
        }
    }

    void drawUI() {
        ImGui::SetNextWindowSize(ImVec2(120, 100));

        ImGui::Begin("Notification", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
        ImGui::Text("Hello World");

        ImGui::SetCursorPosX(20);
        ImGui::SetCursorPosY(70);
        ImGui::Button("No");

        ImGui::SetCursorPosX(50);
        ImGui::SetCursorPosY(70);
        ImGui::Button("Yes");

        ImGui::End();
    }
};
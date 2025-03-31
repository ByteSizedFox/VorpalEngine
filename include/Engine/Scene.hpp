#pragma once

#include <stdio.h>
#include <vector>
#include <atomic>
#include <thread>

#include "Mesh3D.hpp"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "Window.hpp"
#include "FrustumCull.h"

#include "Engine/PhysicsManager.hpp"
#include "Engine/Camera.hpp"

class Scene {
private:
    std::thread loadThread;
public:
    //std::vector<Texture> textures;
    std::vector<Mesh3D*> meshes;
    Physics::PhysicsManager *physManager;
    Camera camera;

    bool isReady = false;
    virtual void setup() {

    }
    void init() {
        camera.createRigidBody();
        camera.setPosition({0.0, 0.0, 0.0});
        

        setup();

        physManager = new Physics::PhysicsManager(meshes, &camera);
        
        isReady = true;
    }
    void loop() {
        printf("WARNING: Unimplemented Loop!\n");
    }
    void drawFrame() {
        printf("WARNING: Unimplemented DrawFrame!\n");
    }

    void destroy() {
        for (Mesh3D *mesh : meshes) {
            mesh->destroy();
        }
    }

    void add_object(Mesh3D *mesh) {
        meshes.push_back(mesh);
    }
    
    virtual void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Window *window) {
        Frustum frustum(window->getProjectionMatrix() * camera.getViewMatrix());

        int c = 0;
        for (Mesh3D *mesh : meshes) {
            if (!mesh->hasPhysics) { // non physics-meshes dont have AABB, skip frustum culling
                mesh->draw(commandBuffer, pipelineLayout);
                continue;
            }

            btVector3 AA;
            btVector3 BB;
            mesh->rigidBody->getAabb(AA, BB);
            glm::vec3 min = glm::vec3(AA.getX(), AA.getY(), AA.getZ()) / glm::vec3(100.0);
            glm::vec3 max = glm::vec3(BB.getX(), BB.getY(), BB.getZ()) / glm::vec3(100.0);

            if (frustum.IsBoxVisible(min, max)) {
                mesh->draw(commandBuffer, pipelineLayout);
                c++;
            }
        }
        //printf("Displaying: %i/%i\n", c, meshes.size());
    }

    virtual void drawUI() {
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
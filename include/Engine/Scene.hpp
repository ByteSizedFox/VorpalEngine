#pragma once

#include <stdio.h>
#include <vector>
#include <atomic>

#include "Mesh3D.hpp"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "Window.hpp"
#include "FrustumCull.hpp"

#include "Engine/PhysicsManager.hpp"
#include "Engine/Camera.hpp"

// forward declaration
class Renderer;

class Scene {
public:
    //std::vector<Texture> textures;
    std::vector<Mesh3D*> meshes;
    Physics::PhysicsManager *physManager;
    Camera camera;

    // ui mesh
    Mesh3D uiMesh;
    std::array<glm::vec3, 6> quadVertices;
    std::array<glm::vec2, 6> quadUVs;

    bool isReady = false;
    virtual void setup() {

    }
    void init() {
        Logger::info("Scene", "Loading Scene...");
        camera.createRigidBody();
        camera.setPosition({0.0, 0.0, 0.0});

        setup();

        physManager = new Physics::PhysicsManager(meshes, &camera);
        
        isReady = true;

        Logger::success("Scene", "Done Loading...");
    }
    void loop() {
        printf("WARNING: Unimplemented Loop!\n");
    }
    void drawFrame() {
        printf("WARNING: Unimplemented DrawFrame!\n");
    }

    void destroy() {
        printf("Destroy Scene\n");
        uiMesh.destroy();
        for (Mesh3D *mesh : meshes) {
            mesh->destroy();
        }
    }

    void add_object(Mesh3D *mesh) {
        meshes.push_back(mesh);
    }
    
    virtual void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Window *window) {
       
        int c = 0;
        for (Mesh3D *mesh : meshes) {
            printf("C\n");
            if (!mesh->hasPhysics) { // non physics-meshes dont have AABB, skip frustum culling
                mesh->draw(commandBuffer, pipelineLayout, 1);
                continue;
            }
            printf("D\n");

            btVector3 AA;
            btVector3 BB;
            mesh->rigidBody->getAabb(AA, BB);
            glm::vec3 min = glm::vec3(AA.getX(), AA.getY(), AA.getZ()) / glm::vec3(100.0);
            glm::vec3 max = glm::vec3(BB.getX(), BB.getY(), BB.getZ()) / glm::vec3(100.0);

            printf("E\n");
            
            if (camera.getFrustum().IsBoxVisible(min, max)) {
                mesh->draw(commandBuffer, pipelineLayout, 1);
                c++;
            }

            printf("F\n");
        }
        //printf("Displaying: %i/%i\n", c, meshes.size());
    }

    virtual void drawUI(Window *window) {
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
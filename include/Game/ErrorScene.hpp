#include "Engine/Scene.hpp"

class ErrorScene : public Scene {
    Mesh3D mesh;
    
    void setup() override {
        // load models
        mesh.init("assets/models/male_07.glb");
        mesh.setRotation({0.0, glm::radians(0.0), glm::radians(180.0)});
        mesh.setPosition({0.01, -2.0, 0.01});
        mesh.createRigidBody(0.0, ColliderType::BOX);
        add_object(&mesh);
    }
    
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Window *window) override {
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
    void drawUI() override {
        ImGui::SetNextWindowSize(ImVec2(120, 100));

        ImGui::Begin("Error Scene", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
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
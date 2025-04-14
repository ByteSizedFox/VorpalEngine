#pragma once

#include "Engine/Scene.hpp"
#include "Engine/Animation.hpp"


#include "GLFW/glfw3.h"

class MainScene : public Scene {
    Mesh3D mesh;
    Mesh3D mesh1;
    Mesh3D meshFloor;

    Animation anim;

    float deltaTime = engineGetTime();
    float lastTime = deltaTime;

    void setup() override {
        // load models
        mesh.init("assets/models/male_07.glb");
        mesh.setRotation({glm::radians(-90.0), glm::radians(0.0), glm::radians(180.0)});
        mesh.setPosition({0.01, -7.0, 0.01});
        mesh.createRigidBody(10.0, ColliderType::CONVEXHULL);

        mesh1.init("assets/models/flatgrass_new.glb");
        mesh1.setPosition({0.0, -5.0, 0.0});
        mesh1.setRotation({0.0,0.0,0.0});
        mesh1.createRigidBody(0.0, ColliderType::TRIMESH);
        
        add_object(&mesh);
        add_object(&mesh1);

        // load UImesh
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Utils::createUIMesh( (float)UI_WIDTH / (float)UI_HEIGHT, vertices, indices);
        uiMesh.loadRaw(vertices, indices, "UIMesh MainScene");
        for (int i = 0; i < 6; i++) {
            quadVertices[i] = uiMesh.m_vertices[i].pos;
            quadUVs[i] = uiMesh.m_vertices[i].texCoord;
        }
        uiMesh.isUI = true;
        uiMesh.setPosition({0.0f, 0.0f, -1.0f});
        uiMesh.setRotation({0.0, 0.0, glm::radians(0.0f)});
    }

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Window *window) override {
        double time = engineGetTime();
        deltaTime = time - lastTime;
        lastTime = time;

        const glm::vec3 forward = camera.getForward();
        glm::vec3 camPos = physicsToWorld(camera.rigidBody->getInterpolationWorldTransform().getOrigin()) + glm::vec3(0.0, 0.25, 0.0);
        glm::vec3 rot = glm::degrees(glm::eulerAngles(camera.getOrientation()));

        // handle mouse interaction with UI
        glm::vec2 result;
        glm::vec3 worldResult;
        float distance = 0.0;
        bool hit = Experiment::raycastRectangle(camPos * glm::vec3(WORLD_SCALE), -forward, quadVertices, quadUVs, uiMesh.m_indices, uiMesh.getModelMatrix().model, camera.getViewMatrix(), Engine::projectionMatrix, distance, result, worldResult);
        ImGuiIO& io = ImGui::GetIO();

        if (distance <= 1.0 && hit) {
            io.AddMousePosEvent(result.x * (float)UI_WIDTH, result.y * (float)UI_HEIGHT);
            io.MouseDrawCursor = true;
        } else {
            io.AddMousePosEvent(999.0, 999.0);
            io.MouseDrawCursor = false;
        }
        
        if (camera.getPosition().y < -10.0) {
            btTransform tr;
            tr.setIdentity();
            camera.rigidBody->setWorldTransform(tr);
            camera.rigidBody->setLinearVelocity(btVector3(0,0,0));
        } 

        // draw all meshes in scene
        int c = 0;
        for (Mesh3D *mesh : meshes) {
            if (!mesh->hasPhysics) { // non physics-meshes dont have AABB, skip frustum culling
                mesh->draw(commandBuffer, pipelineLayout, 1);
                c++;
                continue;
            }
            if (camera.isVisible(mesh->rigidBody)) {
                mesh->draw(commandBuffer, pipelineLayout, 2);
                c++;
            }
        }

        double dayLength = 20.0;
        double t = (time / dayLength) * 2.0 * 3.14159265;
        double x = cos(t) * 1000.0;
        double y = sin(t) * 1000.0; 
 
        Engine::lightPos = glm::vec3(x, y, 0.0);
        Engine::camPos = camera.getPosition() / glm::vec3(100.0);
    }

    void drawUI(Window *window) override {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(VK::physicalDevice, &properties);
        
        ImGui::SetNextWindowSize(ImVec2(UI_WIDTH, UI_HEIGHT));
        ImGui::SetNextWindowPos(ImVec2(0,0));
        
        ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);
        
        glm::vec3 pos = camera.getPosition();

        ImGui::Text("XYZ: %.2f %.2f %.2f", pos.x, pos.y, pos.z);
        ImGui::Text("Press Escape To Exit");
        ImGui::Text("Device Info:");
        ImGui::Text("GPU VendorID: 0x%04X", properties.vendorID);
        ImGui::Text("GPU DeviceID: 0x%04X", properties.deviceID);
        ImGui::Text("GPU Name: %s", properties.deviceName);

        float width = ImGui::CalcTextSize("Exit").x + 10.0;
        ImGui::SetCursorPosX( (UI_WIDTH / 2.0f) - (width/2.0f));
        if (ImGui::Button("Exit", ImVec2(width, 0.0)) || window->isKeyPressed(GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window->get(), GLFW_TRUE);
        }

        ImGui::End();
    }
};

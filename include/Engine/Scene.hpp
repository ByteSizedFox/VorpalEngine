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
class LuaScene;

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

        // load UImesh
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Utils::createUIMesh( (float)UI_WIDTH / (float)UI_HEIGHT, vertices, indices);
        uiMesh.loadRaw(vertices, indices, "UIMesh");
        for (int i = 0; i < 6; i++) {
            quadVertices[i] = uiMesh.m_vertices[i].pos;
            quadUVs[i] = uiMesh.m_vertices[i].texCoord;
        }
        uiMesh.isUI = true;
        uiMesh.setPosition({0.0f, 0.0f, -1.0f});
        uiMesh.setRotation({0.0, 0.0, glm::radians(0.0f)});

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
            delete mesh; // Clean up the meshes created by create_object
        }
        meshes.clear();
    }

    void add_object(Mesh3D *mesh) {
        meshes.push_back(mesh);
    }

    Mesh3D* create_object(const char* modelPath) {
        Mesh3D* mesh = new Mesh3D();
        mesh->init(modelPath);
        meshes.push_back(mesh);
        return mesh;
    }

    void load_lua_scene(const char* scriptPath);

    void handleUIInteraction() {
        const glm::vec3 forward = camera.getForward();
        glm::vec3 camPos = physicsToWorld(camera.rigidBody->getInterpolationWorldTransform().getOrigin()) + glm::vec3(0.0, 0.25, 0.0);

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
    }
    
    virtual void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Window *window) {
       
        int c = 0;
        for (Mesh3D *mesh : meshes) {
            if (!mesh->hasPhysics) { // non physics-meshes dont have AABB, skip frustum culling
                mesh->draw(commandBuffer, pipelineLayout, 1);
                continue;
            }

            btVector3 AA;
            btVector3 BB;
            mesh->rigidBody->getAabb(AA, BB);
            glm::vec3 min = glm::vec3(AA.getX(), AA.getY(), AA.getZ()) / glm::vec3(100.0);
            glm::vec3 max = glm::vec3(BB.getX(), BB.getY(), BB.getZ()) / glm::vec3(100.0);

            
            if (camera.getFrustum().IsBoxVisible(min, max)) {
                mesh->draw(commandBuffer, pipelineLayout, 1);
                c++;
            }

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

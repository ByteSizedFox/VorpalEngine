#include "Engine/Scene.hpp"
#include "Engine/Animation.hpp"

#include "GLFW/glfw3.h"

class ErrorScene : public Scene {
    Mesh3D mesh;
    Animation anim;

    float deltaTime = glfwGetTime();
    float lastTime = deltaTime;

    void setup() override {
        //anim.currentIndex = 0;
        anim.time = 0.0;

        anim.keyframes.push_back({
            glm::vec3(0.0, 2.0, 0.0),
            glm::quat(glm::radians(glm::vec3(-20.0, 0.0, 0.0))),
            0.0
        });
        anim.keyframes.push_back({
            glm::vec3(-10.0, 2.0, -10.0),
            glm::quat(glm::radians(glm::vec3(0.0, 90.0, 0.0))),
            4.0
        });
        anim.keyframes.push_back({
            glm::vec3( -20.0, 2.0, -20.0),
            glm::quat(glm::radians(glm::vec3(20.0, 0.0, 0.0))),
            8.0
        });
        anim.keyframes.push_back({
            glm::vec3( -10.0, 2.0, -30.0),
            glm::quat(glm::radians(glm::vec3(20.0, -90.0, 0.0))),
            12.0
        });
        anim.keyframes.push_back({
            glm::vec3( -0.0, 2.0, -30.0),
            glm::quat(glm::radians(glm::vec3(20.0, 180.0, 0.0))),
            16.0
        });
        anim.keyframes.push_back({
            glm::vec3( 10.0, 2.0, -20.0),
            glm::quat(glm::radians(glm::vec3(20.0, -90.0, 0.0))),
            20.0
        });
        anim.keyframes.push_back({
            glm::vec3( 20.0, 2.0, -10.0),
            glm::quat(glm::radians(glm::vec3(20.0, 180.0, 0.0))),
            24.0
        });

        // // load models
        // mesh.init("assets/models/flatgrass.glb");
        // mesh.setRotation({0.0, glm::radians(0.0), glm::radians(0.0)});
        // mesh.setPosition({0.0, 0.0, 0.0});
        // mesh.createRigidBody(0.0, ColliderType::TRIMESH);
        // add_object(&mesh);

        // load UImesh
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Utils::createUIMesh( (float)UI_WIDTH / (float)UI_HEIGHT, vertices, indices);
        uiMesh.loadRaw(vertices, indices);
        for (int i = 0; i < 6; i++) {
            quadVertices[i] = uiMesh.m_vertices[i].pos;
            quadUVs[i] = uiMesh.m_vertices[i].texCoord;
        }
        uiMesh.isUI = true;
        uiMesh.setPosition({0.0f, 0.0f, -1.0f});
        uiMesh.setRotation({0.0, 0.0, glm::radians(0.0f)});
        //uiMesh.createRigidBody(0.0, ColliderType::BOX);
        //add_object(&uiMesh);
    }


    glm::quat clampQuaternion(const glm::quat& q, float maxAngleRadians) {
        float dot = glm::clamp(q.w, -1.0f, 1.0f);
        float angle = 2.0f * acos(fabs(dot));
        if (angle <= maxAngleRadians) {
            return q;
        }
        if (glm::length(glm::vec3(q.x, q.y, q.z)) < 0.001f) {
            return glm::angleAxis(maxAngleRadians, glm::vec3(0.0f, 0.0f, 1.0f)); // Default axis if no clear direction
        }
        return glm::angleAxis(maxAngleRadians, glm::normalize(glm::vec3(q.x, q.y, q.z)));
    }

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Window *window) override {
        float time = glfwGetTime();
        deltaTime = time - lastTime;
        lastTime = time;

        // dont go out of animation bounds
        if (anim.time >= 24.0) {
            anim.time = 0.0;
        }
        tickAnimation(deltaTime, &anim);

        // hold camera at 0,0

        btTransform tf;
        tf.setIdentity();
        tf.setOrigin(btVector3(0,0,0));
        
        btQuaternion ang = camera.rigidBody->getWorldTransform().getRotation();
        tf.setRotation(ang);

        //camera.setPosition(anim.position);
        //camera.setOrientation(anim.rotation);

        camera.rigidBody->setWorldTransform(tf);
        camera.rigidBody->setLinearVelocity(btVector3(0,0,0));

        // handle mouse interaction with UI
        // raycast to         

        // frustum culling
        Frustum frustum(window->getProjectionMatrix() * camera.getViewMatrix());

        camera.setPosition({0.0,0.0,0.0});
        glm::quat rot = camera.getOrientation();
        glm::quat newRot = clampQuaternion(rot, glm::radians(25.0f));
        camera.setOrientation(newRot);

        const glm::vec3 forward = camera.getForward();

        glm::vec2 result;
        glm::vec3 worldResult;
        float distance = 0.0;
        bool hit = Experiment::raycastRectangle(glm::vec3(0.0,0.0,0.0), -forward, quadVertices, quadUVs, uiMesh.m_indices, uiMesh.getModelMatrix().model, camera.getViewMatrix(), window->getProjectionMatrix(), distance, result, worldResult);
        ImGuiIO& io = ImGui::GetIO();

        if (distance <= 1.0 && hit) {
            io.AddMousePosEvent(result.x * (float)UI_WIDTH, result.y * (float)UI_HEIGHT);
            io.MouseDrawCursor = true;
        } else {
            io.AddMousePosEvent(999.0, 999.0);
            io.MouseDrawCursor = false;
        }

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
        uiMesh.draw(commandBuffer, pipelineLayout);
        //printf("Displaying: %i/%i\n", c, meshes.size());
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

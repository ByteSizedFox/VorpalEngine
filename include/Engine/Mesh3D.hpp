#pragma once
#include <vector>
#include <atomic>

#include <Engine/Vertex.hpp>
#include "Engine/Texture.hpp"

#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

class Mesh3D {
private:
    glm::vec3 position = glm::vec3(0.0f,0.0f,0.0f);
    glm::quat rotationB;

    glm::mat4 modelMatrix = glm::mat4(1.0);
    bool isDirty = false;
public:
    std::string fileName;
    
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    bool isUI = false;

    Mesh3D() {

    }

    void init(const char *modelName);
    void destroy();
    void createVertexBuffer();
    void createIndexBuffer();
    void loadModel(const char* filename);

    ModelBufferObject getModelMatrix();
    void updateModelMatrix();
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    void updatePushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

    void setPosition(glm::vec3 position) {
        this->position = position;
        isDirty = true;
    }
    void setRotation(glm::vec3 rotation) {
        this->rotationB = glm::quat(rotation);
        isDirty = true;
    }
    glm::vec3 getRotation() {
        return glm::eulerAngles(rotationB);
    }
    glm::quat getOrientation() {
        return rotationB;
    }
    void setOrientation(glm::quat rot) {
        rotationB = rot;
    }
    glm::vec3 getPosition() {
        return position;
    }
    void setMatrix(glm::mat4 mat) {
        modelMatrix = mat;
        isDirty = false;
    }
};
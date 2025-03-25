#pragma once
#include <vector>
#include <atomic>

#include <Engine/Vertex.hpp>
#include "Engine/Texture.hpp"
#include "Engine/Node3D.hpp"

// physics
#include "btBulletDynamicsCommon.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"

// to differentiate collider types
enum ColliderType {
    BOX,
    CONVEXHULL,
    TRIMESH
};

class Mesh3D : public Node3D {
public:
    bool hasPhysics = false;

    btRigidBody* rigidBody;

    glm::vec3 AA;
    glm::vec3 BB;
    
    std::string fileName;
    
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    glm::vec3 modelCenter;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    bool isUI = false;
    bool isDebug = false;

    Mesh3D() {

    }

    void init(const char *modelName);
    void destroy();
    void createVertexBuffer();
    void createIndexBuffer();
    void loadModel(const char* filename);
    void loadRaw(std::vector<Vertex> &m_vertices, std::vector<uint32_t> &m_indices);

    ModelBufferObject getModelMatrix();
    void updateModelMatrix();
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    void updatePushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    void createRigidBody(float mass, ColliderType colliderType);
};
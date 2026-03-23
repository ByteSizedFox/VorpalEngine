#pragma once
#include <vector>
#include <atomic>
#include <unordered_map>
#include <string>

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

// Geometry shared across all instances of the same model file.
// GPU buffers are created once and ref-counted; CPU data is kept so that
// cloned instances can still build rigid bodies without re-parsing the file.
struct SharedMeshGeometry {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
    glm::vec3 AA{0}, BB{0}, modelCenter{0};
    VkBuffer       vertexBuffer       = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer       indexBuffer        = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory  = VK_NULL_HANDLE;
    int refCount = 0;
};

class Mesh3D : public Node3D {
public:
    // Geometry cache: models are loaded once and shared across instances
    static std::unordered_map<std::string, SharedMeshGeometry*> s_cache;
    SharedMeshGeometry* sharedGeom = nullptr;  // non-null when using cached geometry

    bool hasPhysics = false;

    btRigidBody* rigidBody;

    glm::vec3 AA;
    glm::vec3 BB;
    
    std::string fileName;
    
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    glm::vec3 modelCenter;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    bool isUI = false;
    bool isDebug = false;

    float metallic = 0.0f;
    float roughness = 0.5f;

    Mesh3D() {

    }

    void init(const char *modelName);
    void destroy();
    void createVertexBuffer();
    void createIndexBuffer();
    void loadModel(const char* filename);
    void loadRaw(std::vector<Vertex> &m_vertices, std::vector<uint32_t> &m_indices, const char *name);

    ModelBufferObject getModelMatrix();
    void updateModelMatrix();
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int count);
    void updatePushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    void createRigidBody(float mass, ColliderType colliderType);
    void setLinearVelocity(glm::vec3 velocity);
    glm::vec3 getLinearVelocity() const;
    glm::vec3 getPhysicsPosition() const;
    void setFriction(float f)               { if (hasPhysics) rigidBody->setFriction(f); }
    void setRestitution(float r)            { if (hasPhysics) rigidBody->setRestitution(r); }
    void setDamping(float linear, float angular) { if (hasPhysics) rigidBody->setDamping(linear, angular); }
};
#include "Engine/Mesh3D.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Vertex.hpp"
#include "config.h"

#include <string.h>
#include <unordered_map>
#include <chrono>

#include <unordered_map>
#include <vector>
#include <stdexcept>

std::unordered_map<std::string, SharedMeshGeometry*> Mesh3D::s_cache;

void Mesh3D::init(const char *modelName) {
    fileName = modelName;
    loadModel(modelName);
    hasPhysics = false;
}

void Mesh3D::destroy() {
    if (sharedGeom) {
        sharedGeom->refCount--;
        if (sharedGeom->refCount <= 0) {
            // Last reference — free shared GPU buffers and evict from cache
            vkDestroyBuffer(VK::device, sharedGeom->vertexBuffer, nullptr);
            vkFreeMemory(VK::device, sharedGeom->vertexBufferMemory, nullptr);
            vkDestroyBuffer(VK::device, sharedGeom->indexBuffer, nullptr);
            vkFreeMemory(VK::device, sharedGeom->indexBufferMemory, nullptr);
            s_cache.erase(fileName);
            delete sharedGeom;
            sharedGeom = nullptr;
        }
        // Shared buffers are still in use by other instances — don't free
    } else {
        vkDestroyBuffer(VK::device, indexBuffer, nullptr);
        vkFreeMemory(VK::device, indexBufferMemory, nullptr);
        vkDestroyBuffer(VK::device, vertexBuffer, nullptr);
        vkFreeMemory(VK::device, vertexBufferMemory, nullptr);
    }
}

void Mesh3D::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    Memory::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

#if ENABLE_DEBUG == true
    VkDebugUtilsObjectNameInfoEXT name_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    name_info.objectType                    = VK_OBJECT_TYPE_BUFFER;
    name_info.objectHandle                  = (uint64_t) stagingBuffer;
    name_info.pObjectName                   = "Staging Buffer";
    vkSetDebugUtilsObjectNameEXT(VK::device, &name_info);
#endif

    void* data;
    vkMapMemory(VK::device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(VK::device, stagingBufferMemory);
    Memory::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
#if ENABLE_DEBUG == true
    std::string name = std::string("Vertex Buffer: ") + std::string(fileName);
    name_info.objectType                    = VK_OBJECT_TYPE_BUFFER;
    name_info.objectHandle                  = (uint64_t) vertexBuffer;
    name_info.pObjectName                   = name.c_str();
    vkSetDebugUtilsObjectNameEXT(VK::device, &name_info);
#endif
    Memory::copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(VK::device, stagingBuffer, nullptr);
    vkFreeMemory(VK::device, stagingBufferMemory, nullptr);
}
void Mesh3D::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    Memory::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(VK::device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_indices.data(), (size_t) bufferSize);
    vkUnmapMemory(VK::device, stagingBufferMemory);

    Memory::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
#if ENABLE_DEBUG == true
    std::string name = std::string("Index Buffer: ") + std::string(fileName);
    VkDebugUtilsObjectNameInfoEXT name_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    name_info.objectType                    = VK_OBJECT_TYPE_BUFFER;
    name_info.objectHandle                  = (uint64_t) indexBuffer;
    name_info.pObjectName                   = name.c_str();
    vkSetDebugUtilsObjectNameEXT(VK::device, &name_info);
#endif
    Memory::copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(VK::device, stagingBuffer, nullptr);
    vkFreeMemory(VK::device, stagingBufferMemory, nullptr);
}

void Mesh3D::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, int count) {
    if (vertexBuffer == VK_NULL_HANDLE || indexBuffer == VK_NULL_HANDLE) {
        return;
    }
    // update mesh push constants
    updatePushConstants(commandBuffer, pipelineLayout);

    // bind buffers for model
    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = {0};    
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // draw
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), count, 0, 0, 0);
}

void Mesh3D::loadModel(const char* filename) {
    auto it = s_cache.find(filename);
    if (it != s_cache.end()) {
        // Cache hit: reuse parsed geometry and GPU buffers
        sharedGeom = it->second;
        sharedGeom->refCount++;
        m_vertices   = sharedGeom->vertices;   // CPU copy needed for createRigidBody
        m_indices    = sharedGeom->indices;
        AA           = sharedGeom->AA;
        BB           = sharedGeom->BB;
        modelCenter  = sharedGeom->modelCenter;
        vertexBuffer       = sharedGeom->vertexBuffer;
        vertexBufferMemory = sharedGeom->vertexBufferMemory;
        indexBuffer        = sharedGeom->indexBuffer;
        indexBufferMemory  = sharedGeom->indexBufferMemory;
    } else {
        // Cache miss: parse from disk and upload to GPU
        sharedGeom = new SharedMeshGeometry();
        sharedGeom->refCount = 1;
        Assets::loadModel(filename, sharedGeom->vertices, sharedGeom->indices,
                          sharedGeom->AA, sharedGeom->BB, sharedGeom->modelCenter);
        m_vertices  = sharedGeom->vertices;
        m_indices   = sharedGeom->indices;
        AA          = sharedGeom->AA;
        BB          = sharedGeom->BB;
        modelCenter = sharedGeom->modelCenter;
        createVertexBuffer();
        createIndexBuffer();
        sharedGeom->vertexBuffer       = vertexBuffer;
        sharedGeom->vertexBufferMemory = vertexBufferMemory;
        sharedGeom->indexBuffer        = indexBuffer;
        sharedGeom->indexBufferMemory  = indexBufferMemory;
        s_cache[filename] = sharedGeom;
    }
    updateModelMatrix();
}

extern std::vector<Texture> textures;

void Mesh3D::updateModelMatrix() {
    modelMatrix = glm::translate(glm::mat4(1.0f), position * glm::vec3(WORLD_SCALE))
                * glm::toMat4(orientation)
                * glm::scale(glm::mat4(1.0f), scale);
}

ModelBufferObject Mesh3D::getModelMatrix() {
    if (isDirty) {
        updateModelMatrix();
        isDirty = false;
    }
    ModelBufferObject ubo{};
    ubo.model = modelMatrix;

    // shader toggles
    ubo.enableNormal = Engine::enableNormal;
    ubo.metallic = metallic;
    ubo.roughness = roughness;
    // return the matrix to the GPU via push constants
    return ubo;
}

void Mesh3D::updatePushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    ModelBufferObject buffer = getModelMatrix();

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelBufferObject), &buffer);
}

void Mesh3D::createRigidBody(float mass, ColliderType colliderType) {
    glm::vec3 size = glm::vec3(abs(BB.x - AA.x), abs(BB.y - AA.y), abs(BB.z - AA.z));

    btCollisionShape *collisionShape = nullptr;

    hasPhysics = true;

    if (colliderType == ColliderType::BOX) {
        collisionShape = new btCompoundShape();
        btBoxShape *child = new btBoxShape(btVector3(size.x / 2.0, size.y / 2.0, size.z / 2.0));

        glm::vec3 vertCenter = -modelCenter;
        btTransform localTransform;
        localTransform.setIdentity();
        //localTransform.setOrigin(btVector3(vertCenter.x, vertCenter.y, vertCenter.z));
        ((btCompoundShape*)collisionShape)->addChildShape(localTransform, child);
    } else if (colliderType == ColliderType::CONVEXHULL) {
        collisionShape = new btConvexHullShape();
        float triangleCount = m_indices.size() / 3;

        // 33% of vertices: optimization, triangulated mesh, every 3 vertices are close, so only use every third vertex
        // double 3 to 6 because we skip every other triangle,
        // since they are often right next to eachother and can be removed from the convex hull

        for (int i = 0; i < m_indices.size(); i += 6) {
            glm::vec3 pnt = m_vertices[m_indices[ i ]].pos;
            ((btConvexHullShape *)collisionShape)->addPoint(btVector3(pnt.x, pnt.y, pnt.z), true);
        }
        ((btConvexHullShape *)collisionShape)->optimizeConvexHull();
    } else { // static triangle mesh
        btTriangleMesh *mesh = new btTriangleMesh();
        for (int i = 0; i < m_indices.size(); i += 3) {
            glm::vec3 pntA = m_vertices[m_indices[ i ]].pos;
            glm::vec3 pntB = m_vertices[m_indices[ i+1 ]].pos;
            glm::vec3 pntC = m_vertices[m_indices[ i+2 ]].pos;

            mesh->addTriangle(
                btVector3(pntA.x, pntA.y, pntA.z),
                btVector3(pntB.x, pntB.y, pntB.z),
                btVector3(pntC.x, pntC.y, pntC.z)
            );
            mesh->addTriangleIndices(0, 1, 2);
        }
        collisionShape = new btBvhTriangleMeshShape(mesh, true);

    }

    btTransform bodyTransform;
    bodyTransform.setIdentity();
    bodyTransform.setOrigin(btVector3(position.x, position.y, position.z));
    bodyTransform.setRotation(btQuaternion(orientation.x, orientation.y, orientation.z, orientation.w));

    collisionShape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));

    btVector3 localInertia(0, 0, 0);
    if (mass != 0.0) {
        collisionShape->calculateLocalInertia(mass, localInertia);
    }

    btDefaultMotionState* motionState = new btDefaultMotionState(bodyTransform);

    collisionShape->setMargin(0.001);

    rigidBody = new btRigidBody(
        btRigidBody::btRigidBodyConstructionInfo(
            mass, motionState, collisionShape, localInertia
        )
    );
}

void Mesh3D::loadRaw(std::vector<Vertex> &m_vertices, std::vector<uint32_t> &m_indices, const char *name) {
    this->m_vertices = m_vertices;
    this->m_indices = m_indices;
    updateModelMatrix();

    this->fileName = name;
    
    createVertexBuffer();
    createIndexBuffer();

    hasPhysics = false;
}

void Mesh3D::setLinearVelocity(glm::vec3 velocity) {
    if (hasPhysics)
        rigidBody->setLinearVelocity(worldToPhysics(velocity));
}

glm::vec3 Mesh3D::getLinearVelocity() const {
    if (!hasPhysics) return glm::vec3(0.0f);
    return physicsToWorld(rigidBody->getLinearVelocity());
}

glm::vec3 Mesh3D::getPhysicsPosition() const {
    if (!hasPhysics) return position;
    return physicsToWorld(rigidBody->getInterpolationWorldTransform().getOrigin());
}
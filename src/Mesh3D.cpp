#include "Engine/Mesh3D.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Vertex.hpp"
#include "config.h"

#include <string.h>
#include <unordered_map>
#include <chrono>

// model loading
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

void Mesh3D::init(const char *modelName, std::vector<int> textures) {
    loadModel(modelName);
    createVertexBuffer();
    createIndexBuffer();
}

void Mesh3D::destroy() {
    vkDestroyBuffer(VK::device, indexBuffer, nullptr);
    vkFreeMemory(VK::device, indexBufferMemory, nullptr);
    vkDestroyBuffer(VK::device, vertexBuffer, nullptr);
    vkFreeMemory(VK::device, vertexBufferMemory, nullptr);
}

void Mesh3D::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    Memory::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(VK::device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(VK::device, stagingBufferMemory);

    Memory::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

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

    Memory::copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(VK::device, stagingBuffer, nullptr);
    vkFreeMemory(VK::device, stagingBufferMemory, nullptr);
}

void Mesh3D::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t currentFrame) {
    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = {0};
    
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
}

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <unordered_map>
#include <vector>
#include <stdexcept>

void Mesh3D::loadModel(const char* filename) {
    // Import the model with postprocessing steps to ensure triangulation and texture coordinates
    const aiScene* scene = Utils::importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_PreTransformVertices);

    // Check if the import was successful
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("Assimp error: " + std::string(Utils::importer.GetErrorString()));
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices {};

    // Process each mesh in the scene
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        
        // Get material for this mesh
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        // Get diffuse texture path
        aiString texturePath;
        int textureID = 0; // Default to 0 if no texture
        
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
            std::string texPath = std::string("assets/textures/") + std::string(texturePath.C_Str());
            printf("Texture Path: %s\n", texPath.c_str());
            
            // Check if texture path already exists in our global list
            auto it = std::find(VK::g_texturePathList.begin(), VK::g_texturePathList.end(), texPath);
            
            if (it != VK::g_texturePathList.end()) {
                // Texture already exists, get its index
                textureID = static_cast<int>(std::distance(VK::g_texturePathList.begin(), it));
            } else {
                // Check if file exists before adding                
                bool exists = Utils::fileExistsZip(texPath);
                if (exists) {
                    // Add new texture path and get its index
                    textureID = static_cast<int>(VK::g_texturePathList.size());
                    VK::g_texturePathList.push_back(texPath);
                }
            }
            printf("Texture ID: %i\n", textureID);
        }

        // Process each face (triangle) in the mesh
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace& face = mesh->mFaces[j];

            // Process each index in the face
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                uint32_t index = face.mIndices[k];

                // Create a new vertex structure
                Vertex vertex{};

                // Position (AI_DEFAULT) assumes a 3D vector format.
                vertex.pos = {
                    mesh->mVertices[index].x,
                    mesh->mVertices[index].y,
                    mesh->mVertices[index].z
                };

                // Texture coordinates (may be missing, but we check if valid)
                if (mesh->mTextureCoords[0]) {
                    vertex.texCoord = {
                        mesh->mTextureCoords[0][index].x,
                        1.0f - mesh->mTextureCoords[0][index].y // Manually flip the Y-coordinate
                    };
                } else {
                    vertex.texCoord = {0.0f, 0.0f}; // Default to (0, 0) if no texture coords
                }

                // Default color (white), as per original code
                vertex.color = {1.0f, 1.0f, 1.0f};
                
                // Store the texture ID
                vertex.textureID = textureID;
                //printf("Texture ID: %i\n", textureID);

                // If this vertex is not already in the unique list, add it
                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                    m_vertices.push_back(vertex);
                }

                // Add index to the indices list
                m_indices.push_back(uniqueVertices[vertex]);
            }
        }
    }
}

extern std::vector<Texture> textures;

ModelBufferObject Mesh3D::getModelMatrix(int index) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    ModelBufferObject ubo{};
    ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.01));
    ubo.model = glm::translate(ubo.model, glm::vec3(index, 0,0)); // translate by index

    // return the matrix to the GPU via push constants
    return ubo;
}
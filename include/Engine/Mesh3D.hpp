#include <vector>
#include <Engine/Vertex.hpp>
#include "Engine/Texture.hpp"

class Mesh3D {
public:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkDescriptorSet> descriptorSets;

    // uniforms are WIP
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    Mesh3D() {
    }

    void init(const char *modelName, std::vector<int> textures);
    void destroy();
    void createVertexBuffer();
    void createIndexBuffer();
    void loadModel(const char* filename);
    

    // wip
    void createUniformBuffers();
    void createDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool);
    void updateUniformBuffer(VkExtent2D swapChainExtent, uint32_t currentImage, int index);
    ModelBufferObject updateModelBuffer(VkExtent2D swapChainExtent, uint32_t currentImage, int index);
    void createTextureImageView();

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t currentFrame);
};
#include <vector>
#include <Engine/Vertex.hpp>

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

    // constants
    const int MAX_FRAMES_IN_FLIGHT = 2;

    Mesh3D() {

    }

    void init(const char *modelName);
    void destroy();
    void createVertexBuffer();
    void createIndexBuffer();
    void loadModel(const char* filename);
    

    // wip
    void createUniformBuffers();
    void createDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, VkImageView textureImageView, VkSampler textureSampler);
    void updateUniformBuffer(VkExtent2D swapChainExtent, uint32_t currentImage, int index);

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t currentFrame);
};
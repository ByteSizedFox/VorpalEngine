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

    Mesh3D() {

    }

    void init();
    void destroy();
    void createVertexBuffer();
    void createIndexBuffer();
    void loadModel(const char* filename);

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, std::vector<VkDescriptorSet> descriptorSets, uint32_t currentFrame);
};
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

    Mesh3D() {

    }

    void init(const char *modelName, std::vector<int> textures);
    void destroy();
    void createVertexBuffer();
    void createIndexBuffer();
    void loadModel(const char* filename);

    ModelBufferObject getModelMatrix(int index);
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t currentFrame);
};
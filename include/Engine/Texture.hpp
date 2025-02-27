#pragma once

#include <volk.h>

#include <math.h>
#include <string.h>
#include <vector>

#include <assimp/scene.h>

class Texture {
public:
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    uint32_t mipLevels;
    int textureID;

    void createTextureImageView();

    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    void createTextureImage(const char *path);
    void createAssimpTextureImage(aiTexture *tex);
    void destroy();
    std::string hashTexture(const char *data, size_t size);
};
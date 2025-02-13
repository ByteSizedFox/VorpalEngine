#pragma once
#include <volk.h>
#include "Engine/Engine.hpp"

#include <math.h>
#include <string.h>

class Texture {
public:
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    uint32_t mipLevels;
    VkSampler textureSampler;

    void createTextureImageView();

    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    void createTextureImage(const char *path);

    void createTextureSampler();
    void destroy();
};
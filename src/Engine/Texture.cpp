#include "Engine/Texture.hpp"
#include "Engine/Engine.hpp"

#include "stb_image.h"

void Texture::createTextureImageView() {
    textureImageView = Image::createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

void Texture::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
// Check if image format supports linear blitting
VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(VK::physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = Command::beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
    Command::endSingleTimeCommands(commandBuffer);
}
/*
void Texture::createAssimpTextureImage(aiTexture *tex) {
    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load_from_memory((unsigned char*) tex->pcData, tex->mWidth, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    Memory::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(VK::device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(VK::device, stagingBufferMemory);

    stbi_image_free(pixels);

    Image::createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, tex->mFilename.C_Str());

    Image::transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
    Image::copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

    vkDestroyBuffer(VK::device, stagingBuffer, nullptr);
    vkFreeMemory(VK::device, stagingBufferMemory, nullptr);

    generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
}
*/

void Texture::createFromGLTFImage(const tinygltf::Image& image, VkFormat format) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = nullptr;
    VkDeviceSize imageSize = 0;
    
    // Handle different image formats in glTF
    if (!image.image.empty()) {
        // Image data is already loaded in the glTF
        texWidth = image.width;
        texHeight = image.height;
        texChannels = image.component;
        
        // If image is not RGBA, we need to convert it
        if (texChannels != 4) {
            // Use the existing image data but convert to RGBA
            pixels = stbi_load_from_memory(
                reinterpret_cast<const stbi_uc*>(image.image.data()),
                static_cast<int>(image.image.size()),
                &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            
            imageSize = texWidth * texHeight * 4;
        } else {
            // Already in RGBA format, just use the data directly
            pixels = const_cast<stbi_uc*>(image.image.data());
            imageSize = image.image.size();
        }
    } else if (!image.uri.empty() && image.uri.find("data:") == 0) {
        // Handle base64 embedded image data
        // Extract the base64 data part
        std::string base64Data = image.uri.substr(image.uri.find(",") + 1);
        
        // Decode the base64 string
        std::vector<unsigned char> decodedData;
        decodedData.resize(base64Data.size() * 3 / 4); // Approximate size allocation
        
        // Use a base64 decoding library here (not shown)
        // For example: base64_decode(base64Data, decodedData);
        
        // Now load the image data
        pixels = stbi_load_from_memory(
            decodedData.data(), 
            static_cast<int>(decodedData.size()),
            &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        
        imageSize = texWidth * texHeight * 4;
    } else {
        throw std::runtime_error("Unsupported image format in glTF");
    }
    
    // Calculate mip levels
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    
    // Create a staging buffer for the image data
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    Memory::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                        stagingBuffer, stagingBufferMemory);
    
    // Copy the image data to the staging buffer
    void* data;
    vkMapMemory(VK::device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(VK::device, stagingBufferMemory);
    
    // Free the pixel data if we allocated it
    if (texChannels != 4 || !image.uri.empty()) {
        stbi_image_free(pixels);
    }
    
    // Create the texture image
    std::string imageName = !image.name.empty() ? image.name : "gltf_texture";
    Image::createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, 
                      format, VK_IMAGE_TILING_OPTIMAL, 
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, imageName.c_str());
    
    // Transition image layout and copy data
    Image::transitionImageLayout(textureImage, format, 
                               VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                               mipLevels);
    
    Image::copyBufferToImage(stagingBuffer, textureImage, 
                           static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    
    // Clean up
    vkDestroyBuffer(VK::device, stagingBuffer, nullptr);
    vkFreeMemory(VK::device, stagingBufferMemory, nullptr);
    
    // Generate mipmaps for better texture rendering
    generateMipmaps(textureImage, format, texWidth, texHeight, mipLevels);
}

void Texture::createTextureImage(const char *path) {
    int texWidth, texHeight, texChannels;

    std::vector<char> buffer = std::move(Utils::readFileZip(path));

    //printf("Texture Hash: %s: %s\n", path, hashTexture(buffer.data(), buffer.size()).c_str() );
    //printf("Path Hash: %s: %s\n", path, hashTexture(path, strlen(path)).c_str() );

    stbi_uc* pixels = stbi_load_from_memory((unsigned char*) buffer.data(), buffer.size() * sizeof(char), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (!pixels) {
        printf("Filename: %s\n", path);
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    Memory::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(VK::device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(VK::device, stagingBufferMemory);

    stbi_image_free(pixels);

    Image::createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_BC3_SRGB_BLOCK, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, path);

    Image::transitionImageLayout(textureImage, VK_FORMAT_BC3_SRGB_BLOCK, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
    Image::copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

    vkDestroyBuffer(VK::device, stagingBuffer, nullptr);
    vkFreeMemory(VK::device, stagingBufferMemory, nullptr);

    generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
}
void Texture::destroy() {
    vkDestroyImageView(VK::device, textureImageView, nullptr);
    vkDestroyImage(VK::device, textureImage, nullptr);
    vkFreeMemory(VK::device, textureImageMemory, nullptr);
}

#include <bits/stdc++.h>
#include <functional>

std::string Texture::hashTexture(const char *data, size_t size) {
    std::hash<std::string> hasher;
    size_t hash = hasher(std::string(reinterpret_cast<const char*>(data), size));
    return std::to_string(hash);
}
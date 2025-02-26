#pragma once

#include "volk.h"
#include <stdexcept>
#include <fstream>
#include <vector>

#include <assimp/IOSystem.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "config.h"

namespace VK {
    inline VkDevice device;
    inline VkPhysicalDevice physicalDevice;
    inline VkCommandPool commandPool;
    inline VkQueue graphicsQueue;
    inline VkSurfaceKHR surface;
    inline std::vector<std::string> g_texturePathList;
};

namespace Command {
    inline VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = VK::commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(VK::device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    inline void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(VK::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(VK::graphicsQueue);

        vkFreeCommandBuffers(VK::device, VK::commandPool, 1, &commandBuffer);
    }
};

namespace Memory {
    inline uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(VK::physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    inline void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(VK::device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(VK::device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(VK::device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(VK::device, buffer, bufferMemory, 0);
    }

    inline void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = Command::beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        Command::endSingleTimeCommands(commandBuffer);
    }
};

#include "assimp/Importer.hpp"

namespace Utils {
    inline Assimp::Importer importer;
    inline Assimp::IOSystem* ioSystem;

    void initIOSystem(const char * data, size_t size);

    inline std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            printf("Filename: %s\n", filename.c_str());
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
    std::vector<char> readFileZip(const std::string& filename);
    bool fileExistsZip(const std::string& filename);
    
    inline void setViewPort(VkCommandBuffer commandBuffer, glm::vec2 pos, glm::vec2 size, glm::vec2 depth) {
        VkViewport viewport{};
        viewport.x = pos.x;
        viewport.y = pos.y;
        viewport.width = size.x;
        viewport.height = size.y;
        viewport.minDepth = depth.x;
        viewport.maxDepth = depth.y;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    }

    inline VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(VK::physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }
    inline VkFormat findDepthFormat() {
        return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }
    inline bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }
    inline glm::mat4 createMatrix(glm::vec3 position, glm::vec3 rotationA, glm::vec3 rotationB) {
        glm::mat4 matrix = glm::mat4(1.0);
        matrix = glm::rotate(matrix, rotationA.y, glm::vec3(1.0,0.0,0.0));
        matrix = glm::rotate(matrix, rotationA.x, glm::vec3(0.0,1.0,0.0));
        matrix = glm::rotate(matrix, rotationA.z, glm::vec3(0.0,0.0,1.0));
        matrix = glm::translate(matrix, position);
        matrix = glm::rotate(matrix, rotationB.y, glm::vec3(1.0,0.0,0.0));
        matrix = glm::rotate(matrix, rotationB.x, glm::vec3(0.0,1.0,0.0));
        matrix = glm::rotate(matrix, rotationB.z, glm::vec3(0.0,0.0,1.0));
        matrix = glm::scale(matrix, glm::vec3(WORLD_SCALE));
        return matrix;
    }
};

namespace Image {
    inline void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(VK::device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(VK::device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = Memory::findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(VK::device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(VK::device, image, imageMemory, 0);
    }
    inline void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
    VkCommandBuffer commandBuffer = Command::beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } 
    // New transition: undefined to color attachment optimal (for rendering to texture)
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } 
    // New transition: color attachment optimal to shader read only optimal (after rendering to texture)
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    // New transition: shader read only optimal to color attachment optimal (if reusing the texture)
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    // Handle depth-related transitions if needed by checking the format
    else if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || 
             format == VK_FORMAT_D24_UNORM_S8_UINT) {
        // Set aspect mask to depth for depth formats
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        
        // Add stencil aspect if format has stencil component
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        
        // Handle specific depth transitions
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    Command::endSingleTimeCommands(commandBuffer);
}
    inline void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = Command::beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        Command::endSingleTimeCommands(commandBuffer);
    }
    inline VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(VK::device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }
};

// raycasting
namespace Experiment {
    inline glm::vec3 calcBary(glm::vec2 baryPosition, glm::vec3 vert0, glm::vec3 vert1, glm::vec3 vert2) {
        float u = baryPosition.x;
        float v = baryPosition.y;
        float w = 1.0f - u - v; // w is calculated from u and v
        return w * vert0 + u * vert1 + v * vert2;
    }
    inline bool raycastRectangle(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDirection,
        const std::array<glm::vec3, 6>& vertices,
        const std::array<glm::vec2, 6>& uvs,
        const std::vector<uint32_t>& indices,
        const glm::mat4& modelMatrix,
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix,
        float& distance,
        glm::vec2& hitUV)
    {
        // Create the inverse transformation matrix to convert the ray to model space
        glm::mat4 inverseMatrix = glm::inverse(modelMatrix);
        
        // Transform ray to model space
        glm::vec4 modelSpaceOriginVec4 = inverseMatrix * glm::vec4(rayOrigin, 1.0f);
        glm::vec4 modelSpaceDirVec4 = inverseMatrix * glm::vec4(rayDirection, 0.0f);
        
        glm::vec3 modelSpaceOrigin(modelSpaceOriginVec4);
        glm::vec3 modelSpaceDir(modelSpaceDirVec4);
        
        // Normalize the direction after transformation
        modelSpaceDir = glm::normalize(modelSpaceDir);
        
        // Initialize closest hit information
        bool hasHit = false;
        float closestDistance = std::numeric_limits<float>::max();
        
        // Check both triangles in the rectangle
        for (size_t i = 0; i < indices.size(); i += 3) {
            // Get the three vertices of the current triangle
            const glm::vec3& vert0 = vertices[indices[i]];
            const glm::vec3& vert1 = vertices[indices[i + 1]];
            const glm::vec3& vert2 = vertices[indices[i + 2]];

            // Get the UVs for each vertex
            const glm::vec2& uv0 = uvs[indices[i]];
            const glm::vec2& uv1 = uvs[indices[i + 1]];
            const glm::vec2& uv2 = uvs[indices[i + 2]];
            
            // Output parameter for barycentric coordinates and distance
            glm::vec2 baryPosition;
            
            // Check for intersection
            if (glm::intersectRayTriangle(
                    modelSpaceOrigin,
                    modelSpaceDir,
                    vert0, vert1, vert2,
                    baryPosition, distance)) {
                    
                    // Calculate barycentric coordinates
                    float u = baryPosition.x;
                    float v = baryPosition.y;
                    float w = 1.0f - u - v;
                    
                    // Calculate model space intersection point using barycentric coordinates
                    glm::vec3 modelSpaceIntersection = w * vert0 + u * vert1 + v * vert2;
                    
                    // Transform the intersection point back to world space
                    glm::vec4 worldSpaceIntersection = modelMatrix * glm::vec4(modelSpaceIntersection, 1.0f);
                    
                    // Interpolate the UV coordinates based on barycentric coordinates
                    hitUV = w * uv0 + u * uv1 + v * uv2;
                return true;
            }
        }
        
        return false;
    }
};
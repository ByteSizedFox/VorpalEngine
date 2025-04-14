#pragma once

#include "volk.h"

#include <stdexcept>
#include <fstream>
#include <vector>
#include <algorithm>
#include <map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "config.h"

#include <map>

#include "Engine/Vertex.hpp"
#include "Texture.hpp"

#include "Engine/Engine.hpp"


// for terrain
#include "FastNoiseLite.h"

#include <time.h>
inline double engineGetTime(void) {
    struct timespec ts;
    
    // Use CLOCK_MONOTONIC for a steady clock that isn't affected by system time changes
    // Similar to what GLFW uses internally
    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    // Convert to seconds (same format as glfwGetTime)
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1.0e9;
}

namespace VK {
    inline VkDevice device;
    inline VkPhysicalDevice physicalDevice;
    inline VkCommandPool commandPool;
    inline VkQueue graphicsQueue;
    inline VkSurfaceKHR surface;
    inline std::vector<std::string> g_texturePathList;
    inline std::unordered_map<std::string, Texture> textureMap;
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

namespace Utils {
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
    inline void createUIMesh(float aspect, std::vector<Vertex> &m_vertices, std::vector<uint32_t> &m_indices) {
        m_vertices.clear();
        m_indices.clear();
        float width = 0.2;
        float height = (width * (1.0/aspect));
        
        m_indices = {2,1,0, 5,4,3};

        m_vertices.push_back({glm::vec3( width, -height, 0.0), glm::vec3(0.0, 0.0, 0.0), glm::vec2(1.0, 1.0), 0});
        m_vertices.push_back({glm::vec3(-width,  height, 0.0), glm::vec3(0.0, 0.0, 0.0), glm::vec2(0.0, 0.0), 0});
        m_vertices.push_back({glm::vec3( width,  height, 0.0), glm::vec3(0.0, 0.0, 0.0), glm::vec2(1.0, 0.0), 0});

        m_vertices.push_back({glm::vec3( width, -height, 0.0), glm::vec3(0.0, 0.0, 0.0), glm::vec2(1.0, 1.0), 0});
        m_vertices.push_back({glm::vec3(-width, -height, 0.0), glm::vec3(0.0, 0.0, 0.0), glm::vec2(0.0, 1.0), 0});
        m_vertices.push_back({glm::vec3(-width,  height, 0.0), glm::vec3(0.0, 0.0, 0.0), glm::vec2(0.0, 0.0), 0});
    }

    inline float noise3D(FastNoiseLite noise, float x, float y, float z) {
        float a = noise.GetNoise((float)x, (float)y);
        float b = noise.GetNoise((float)y, (float)z);
        float c = noise.GetNoise((float)x, (float)z);
        return (a+b+c) / 3.0f;
    }

    inline void createBox(glm::vec3 AA, glm::vec3 BB, std::vector<Vertex> &m_vertices, std::vector<uint32_t> &m_indices) {
        int offset = m_vertices.size();
        AA = AA * glm::vec3(4.0f);
        BB = BB * glm::vec3(4.0f);
        // face 1
        m_vertices.push_back({glm::vec3(AA.x, AA.y, AA.z), glm::vec3(0.0, 0.0, 0.0), glm::vec2(0.0, 0.0), 0});
        m_vertices.push_back({glm::vec3(AA.x, BB.y, AA.z), glm::vec3(0.0, 0.0, 1.0), glm::vec2(0.0, 0.0), 0});
        m_vertices.push_back({glm::vec3(BB.x, AA.y, AA.z), glm::vec3(0.0, 1.0, 0.0), glm::vec2(0.0, 0.0), 0});
        m_vertices.push_back({glm::vec3(BB.x, BB.y, AA.z), glm::vec3(0.0, 1.0, 1.0), glm::vec2(0.0, 0.0), 0});

        m_vertices.push_back({glm::vec3(AA.x, AA.y, BB.z), glm::vec3(1.0, 0.0, 0.0), glm::vec2(0.0, 0.0), 0});
        m_vertices.push_back({glm::vec3(AA.x, BB.y, BB.z), glm::vec3(1.0, 0.0, 1.0), glm::vec2(0.0, 0.0), 0});        
        m_vertices.push_back({glm::vec3(BB.x, AA.y, BB.z), glm::vec3(1.0, 1.0, 0.0), glm::vec2(0.0, 0.0), 0});
        m_vertices.push_back({glm::vec3(BB.x, BB.y, BB.z), glm::vec3(1.0, 1.0, 1.0), glm::vec2(0.0, 0.0), 0});

        // face 1 front
        m_indices.push_back(offset + 0);
        m_indices.push_back(offset + 1);
        m_indices.push_back(offset + 2);
        m_indices.push_back(offset + 2);
        m_indices.push_back(offset + 1);
        m_indices.push_back(offset + 3);

        // face 2 back
        m_indices.push_back(offset + 6);
        m_indices.push_back(offset + 5);
        m_indices.push_back(offset + 4);
        m_indices.push_back(offset + 7);
        m_indices.push_back(offset + 5);
        m_indices.push_back(offset + 6);

        // face 3 bottom
        m_indices.push_back(offset + 0);
        m_indices.push_back(offset + 2);
        m_indices.push_back(offset + 4);

        m_indices.push_back(offset + 4);
        m_indices.push_back(offset + 2);
        m_indices.push_back(offset + 6);

        // face 4 top
        m_indices.push_back(offset + 5);
        m_indices.push_back(offset + 3);
        m_indices.push_back(offset + 1);

        m_indices.push_back(offset + 7);
        m_indices.push_back(offset + 3);
        m_indices.push_back(offset + 5);

        // face 5 left 0 1 4 5
        m_indices.push_back(offset + 4);
        m_indices.push_back(offset + 1);
        m_indices.push_back(offset + 0);

        m_indices.push_back(offset + 5);
        m_indices.push_back(offset + 1);
        m_indices.push_back(offset + 4);

        // face 6 right
        m_indices.push_back(offset + 2);
        m_indices.push_back(offset + 3);
        m_indices.push_back(offset + 6);

        m_indices.push_back(offset + 6);
        m_indices.push_back(offset + 3);
        m_indices.push_back(offset + 7);
        
    }

    inline void createGroundPlaneMesh(float size, int subdivisions, std::vector<Vertex> &m_vertices, std::vector<uint32_t> &m_indices) {
        m_vertices.clear();
        m_indices.clear();

        FastNoiseLite noise;
        noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        
        // Ensure we have at least 1 subdivision
        subdivisions = std::max(1, subdivisions);
        
        // Calculate the size of each grid cell
        float cellSize = size / subdivisions;
        // Calculate the half size of the ground plane
        float halfSize = size / 2.0f;

        float height = 0.25;

        
        // Generate vertices
        for (int z = 0; z <= subdivisions; z++) {
            for (int x = 0; x <= subdivisions; x++) {
                // Calculate the position for this vertex
                float xPos = -halfSize + x * cellSize;
                float zPos = -halfSize + z * cellSize;
                
                // Calculate texture coordinates (UV)
                float u = static_cast<float>(x) / subdivisions;
                float v = static_cast<float>(z) / subdivisions;
                
                // Add vertex (y is 0 for a ground plane)
                // Note: normal points upward
                float a = noise.GetNoise((float)xPos, (float)zPos); //sin(sqrt( (xPos - (size/2)) * (zPos-(size/2))));
                float s = noise3D(noise, (float)xPos, (float)zPos, a);
                float b = noise.GetNoise(a * 100.0f, s * 100.0f);

                float amp = noise.GetNoise((float)xPos * 1.05f, (float)zPos * 1.05f) + b;

                float yPos = b * 1.0f;//(s + abs(amp * 10.0f)) * 8.0f;
                int texID = (int) abs( yPos * VK::g_texturePathList.size() * 0.5f ) % VK::g_texturePathList.size();

                m_vertices.push_back({
                    glm::vec3(xPos, yPos, zPos),      // Position
                    glm::vec3(0.0f, 1.0f, 0.0f),      // Normal pointing up
                    glm::vec2(u, v),                  // Texture coordinates
                    texID                                 // Additional data (kept as 0 like in your example)
                });
            }
        }
        
        // Generate indices for triangles
        for (int z = 0; z < subdivisions; z++) {
            for (int x = 0; x < subdivisions; x++) {
                // Calculate the indices for the current quad
                uint32_t topLeft = z * (subdivisions + 1) + x;
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = (z + 1) * (subdivisions + 1) + x;
                uint32_t bottomRight = bottomLeft + 1;
                
                // Create two triangles for the quad
                // Triangle 1 (top-left, bottom-left, bottom-right)
                m_indices.push_back(topLeft);
                m_indices.push_back(bottomLeft);
                m_indices.push_back(bottomRight);
                
                // Triangle 2 (top-left, bottom-right, top-right)
                m_indices.push_back(topLeft);
                m_indices.push_back(bottomRight);
                m_indices.push_back(topRight);
            }
        }
    }
};

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdio>

namespace Logger {
    // color definitions for printing
    inline const char *colorMap[] = {
        "\e[39m",       // reset
        "\e[38;5;33m",  // info (bright blue)
        "\e[38;5;40m",  // success (green)
        "\e[38;5;178m", // warning (gold)
        "\e[38;5;196m", // error (bright red)
    };

    // store start times for each key with millisecond precision
    inline std::map<std::string, std::chrono::time_point<std::chrono::system_clock>> startMap;

    // log history
    inline std::vector<std::string> log_storage;

    inline std::chrono::time_point<std::chrono::system_clock> getTimePoint() {
        return std::chrono::system_clock::now();
    }

    inline time_t getTime() {
        return std::chrono::system_clock::to_time_t(getTimePoint());
    }

    inline std::string getTimestamp() {
        time_t now = getTime();
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    inline std::string getTimeDiff(std::string key) {
        auto now = getTimePoint();
        
        // check if key needs setting
        if (startMap.find(key) == startMap.end()) {
            startMap[key] = now;
            return "+0ms";
        }
        
        // Calculate difference in milliseconds as integer
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - startMap[key]).count();
        
        // Format with milliseconds as integer
        std::stringstream ss;
        ss << "+" << diff << "ms";
        
        // Update the start time for the next call
        startMap[key] = now;
        
        return ss.str();
    }

    inline void logPrint(std::string key, int color, const char *prefix, const char *message) {
        auto timestamp = getTimestamp();
        auto time_diff = getTimeDiff(key);
    
        printf("[%s%s\e[39m] [%s] [%s] [%s] %s\n", 
                colorMap[color],
                prefix,
                timestamp.c_str(), 
                time_diff.c_str(), 
                key.c_str(),
                message);
               
        // Optionally store the log
        char buf[256];
        snprintf(buf, 256, 
            "[%s] [%s] [%s] [%s] %s\n", 
            key.c_str(),
            timestamp.c_str(), 
            time_diff.c_str(),
            prefix, 
            message);
        log_storage.push_back(std::string(buf));
    }

    // getTimeDiff updates last_time because we still want time benchmarking when loglevel is low
    inline void info(std::string key, const char *msg) {
        if (LOGLEVEL < 3) {
            getTimeDiff(key);
            return;
        }
        logPrint(key, 1, "INFO", msg);
    }
    inline void success(std::string key, const char *msg) {
        if (LOGLEVEL < 2) {
            getTimeDiff(key);
            return;
        }
        logPrint(key, 2, "SUCCESS", msg);
    }
    
    inline void warning(std::string key, const char *msg) {
        if (LOGLEVEL < 1) {
            getTimeDiff(key);
            return;
        }
        logPrint(key, 3, "WARNING", msg);
    }
    
    inline void error(std::string key, const char *msg) {
        if (LOGLEVEL < 0) {
            getTimeDiff(key);
            return;
        }
        logPrint(key, 4, "ERROR", msg);
    }
};

namespace Image {
    inline void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, const char* name) {
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
#if ENABLE_DEBUG == true
        VkDebugUtilsObjectNameInfoEXT nameInfo = {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            nullptr,
            VK_OBJECT_TYPE_IMAGE,
            reinterpret_cast<std::uint64_t>(image),
            name,
        };
        vkSetDebugUtilsObjectNameEXT(VK::device, &nameInfo);
#endif
    }

// Helper function to check if a format is compressed
inline bool isCompressedFormat(VkFormat format) {
    // Check for common compressed formats
    switch (format) {
        // BC formats
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK:
        // ETC2/EAC formats
        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
        // ASTC formats (just a subset shown for brevity)
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            return true;
        default:
            return false;
    }
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
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // Set appropriate aspect mask based on format
    if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || 
            format == VK_FORMAT_D24_UNORM_S8_UINT) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        
        // Add stencil aspect if format has stencil component
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        // For all color formats (including compressed formats)
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    // Standard transitions (works for both compressed and uncompressed formats)
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } 
    // New transition: undefined to transfer src optimal
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } 
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } 
    // New transition: undefined to color attachment optimal (for rendering to texture)
    // Note: This typically doesn't apply to compressed formats as they can't be used as render targets
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        // Check if format is compressed - if so, this is an invalid transition
        if (isCompressedFormat(format)) {
            throw std::invalid_argument("Cannot transition compressed format to COLOR_ATTACHMENT_OPTIMAL!");
        }
        
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
        // Check if format is compressed - if so, this is an invalid transition
        if (isCompressedFormat(format)) {
            throw std::invalid_argument("Cannot transition compressed format to COLOR_ATTACHMENT_OPTIMAL!");
        }
        
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    // Handle depth-related transitions
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
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
        glm::vec2& hitUV,
        glm::vec3& worldHit)
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
            const glm::vec3& vert0 = vertices[indices[i]] * glm::vec3(WORLD_SCALE);
            const glm::vec3& vert1 = vertices[indices[i + 1]] * glm::vec3(WORLD_SCALE);
            const glm::vec3& vert2 = vertices[indices[i + 2]] * glm::vec3(WORLD_SCALE);

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
                    worldHit = glm::vec3(worldSpaceIntersection) / glm::vec3(WORLD_SCALE);

                    // Interpolate the UV coordinates based on barycentric coordinates
                    hitUV = w * uv0 + u * uv1 + v * uv2;
                return true;
            }
        }
        
        return false;
    }
};

#include "tiny_gltf.h"
#include <iostream>

namespace Assets {
    inline glm::mat4 getLocalMatrix( const tinygltf::Node& node) {
        if (node.matrix.size() == 16) {
            // Matrix is provided directly
            return glm::mat4(
                node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3],
                node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7],
                node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11],
                node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]
            );
        }
        
        // Decomposed transform: TRS (Translation, Rotation, Scale)
        glm::mat4 translation = glm::mat4(1.0f);
        glm::mat4 rotation = glm::mat4(1.0f);
        glm::mat4 scale = glm::mat4(1.0f);
        
        // Apply translation
        if (node.translation.size() == 3) {
            translation = glm::translate(glm::mat4(1.0f), 
                glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
        }
        
        // Apply rotation (quaternion)
        if (node.rotation.size() == 4) {
            glm::quat q(
                node.rotation[3], // w
                node.rotation[0], // x
                node.rotation[1], // y
                node.rotation[2]  // z
            );
            rotation = glm::mat4_cast(q);
        }
        
        // Apply scale
        if (node.scale.size() == 3) {
            scale = glm::scale(glm::mat4(1.0f), 
                glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
        }
        
        // Combine transforms: T * R * S
        return translation * rotation * scale;
    };

    inline void loadModel(const char* filename, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices, glm::vec3 &AA, glm::vec3 &BB, glm::vec3 &vertexCenter) {
        // Create a tinygltf model, options and error/warning strings
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;
        bool ret = false;

        // Load the GLTF/GLB file
        // Determine if file is glb (binary) or gltf (text)
        std::string ext = filename;
        ext = ext.substr(ext.find_last_of(".") + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        std::vector<char> fileData = std::move(Utils::readFileZip(filename));
        
        if (ext == "glb") {
            // Binary glTF
            ret = loader.LoadBinaryFromMemory(&model, &err, &warn, 
                reinterpret_cast<const unsigned char*>(fileData.data()), 
                static_cast<unsigned int>(fileData.size()));
        } else {
            // ASCII glTF
            std::string json(fileData.begin(), fileData.end());
            ret = loader.LoadASCIIFromString(&model, &err, &warn, json.c_str(), 
                static_cast<unsigned int>(json.size()), "");
        }

        // Check for loading errors
        if (!ret) {
            throw std::runtime_error("TinyGLTF error: " + err);
        }

        // Print warnings if any
        if (!warn.empty()) {
            std::cerr << "TinyGLTF warning: " << warn << std::endl;
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices {};
        
        glm::vec3 minVec(99999.0f);
        glm::vec3 maxVec(-99999.0f);

        int rootNodeIdx = model.scenes[model.defaultScene].nodes[0];
        glm::mat4 rootMat = getLocalMatrix(model.nodes[rootNodeIdx]);

        
        // Process all meshes in the model
        for (size_t i = 0; i < model.meshes.size(); i++) {
            const tinygltf::Mesh &mesh = model.meshes[i];

            // Process each primitive (similar to submesh in Assimp)
            for (size_t j = 0; j < mesh.primitives.size(); j++) {
                const tinygltf::Primitive &primitive = mesh.primitives[j];
                
                // Get material for this primitive
                int materialIndex = primitive.material;
                int textureID = 0; // Default texture ID
                int normalID = -1; // default normal Map ID


                
                // Process material and texture if available
                if (materialIndex >= 0 && materialIndex < model.materials.size()) {
                    const tinygltf::Material &material = model.materials[materialIndex];
                    
                    // Check for base color texture
                    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
                        int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
                        // wip load normal map
                        int sourceIndex = model.textures[textureIndex].source;
                        
                        if (sourceIndex >= 0 && sourceIndex < model.images.size()) {
                            const tinygltf::Image &image = model.images[sourceIndex];
                            std::string texPath;
                            bool isEmbedded = true;
                            
                            // Use image name or create a name based on material
                            if (!image.name.empty()) {
                                texPath = image.name;
                            } else if (!material.name.empty()) {
                                texPath = material.name + "_texture";
                            } else {
                                texPath = "material_" + std::to_string(materialIndex) + "_texture";
                            }
                            
                            // For external textures
                            if (!image.uri.empty()) {
                                texPath = std::string("assets/textures/") + image.uri;
                                isEmbedded = false;
                            }
                            
                            // Check if texture path already exists in our global list
                            auto it = std::find(VK::g_texturePathList.begin(), VK::g_texturePathList.end(), texPath);
                            
                            if (it != VK::g_texturePathList.end()) {
                                // Texture already exists, get its index
                                textureID = static_cast<int>(std::distance(VK::g_texturePathList.begin(), it));
                            } else {
                                bool exists = false;
                                if (!isEmbedded) {
                                    exists = Utils::fileExistsZip(texPath);
                                } else {
                                    // For embedded textures, they always exist since they're in the model
                                    exists = true;
                                }
                                
                                if (exists) {
                                    // Add new texture path and get its index
                                    textureID = static_cast<int>(VK::g_texturePathList.size());
                                    
                                    Texture texture;
                                    texture.textureID = textureID;
                                    
                                    if (isEmbedded) {
                                        // Create texture from embedded image data
                                        texture.createFromGLTFImage(image, VK_FORMAT_R8G8B8A8_SRGB);
                                    } else {
                                        // Create texture from file
                                        texture.createTextureImage(texPath.c_str());
                                    }
                                    texture.createTextureImageView();
                                    
                                    VK::textureMap[texPath] = texture;
                                    VK::g_texturePathList.push_back(texPath);
                                }
                            }
                        }
                    }
                    // check for normal texture
                    if (material.normalTexture.index >= 0) {
                        printf("FOUND NORMAL TEXTURE!!!\n");
                        int textureIndex = material.normalTexture.index;
                        // wip load normal map
                        int sourceIndex = model.textures[textureIndex].source;
                        
                        if (sourceIndex >= 0 && sourceIndex < model.images.size()) {
                            const tinygltf::Image &image = model.images[sourceIndex];
                            std::string texPath;
                            bool isEmbedded = true;
                            
                            // Use image name or create a name based on material
                            if (!image.name.empty()) {
                                texPath = image.name;
                            } else if (!material.name.empty()) {
                                texPath = material.name + "_normal_texture";
                            } else {
                                texPath = "material_" + std::to_string(materialIndex) + "_normal_texture";
                            }
                            
                            // For external textures
                            if (!image.uri.empty()) {
                                texPath = std::string("assets/textures/") + image.uri;
                                isEmbedded = false;
                            }
                            
                            // Check if texture path already exists in our global list
                            auto it = std::find(VK::g_texturePathList.begin(), VK::g_texturePathList.end(), texPath);
                            
                            if (it != VK::g_texturePathList.end()) {
                                // Texture already exists, get its index
                                normalID = static_cast<int>(std::distance(VK::g_texturePathList.begin(), it));
                            } else {
                                bool exists = false;
                                if (!isEmbedded) {
                                    exists = Utils::fileExistsZip(texPath);
                                } else {
                                    // For embedded textures, they always exist since they're in the model
                                    exists = true;
                                }
                                
                                if (exists) {
                                    // Add new texture path and get its index
                                    normalID = static_cast<int>(VK::g_texturePathList.size());
                                    
                                    Texture texture;
                                    texture.textureID = normalID;
                                    
                                    if (isEmbedded) {
                                        // Create texture from embedded image data
                                        texture.createFromGLTFImage(image, VK_FORMAT_R8G8B8A8_SRGB);
                                    } else {
                                        // Create texture from file
                                        texture.createTextureImage(texPath.c_str());
                                    }
                                    texture.createTextureImageView();
                                    
                                    VK::textureMap[texPath] = texture;
                                    VK::g_texturePathList.push_back(texPath);
                                }
                            }
                        }
                    }
                }
                
                // Get indices
                if (primitive.indices >= 0) {
                    const tinygltf::Accessor &indexAccessor = model.accessors[primitive.indices];
                    const tinygltf::BufferView &indexBufferView = model.bufferViews[indexAccessor.bufferView];
                    const tinygltf::Buffer &indexBuffer = model.buffers[indexBufferView.buffer];
                    
                    const uint8_t *indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
                    
                    // Process indices based on component type
                    size_t indexCount = indexAccessor.count;
                    size_t indexStride = 0;
                    
                    switch (indexAccessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            indexStride = 2;
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                            indexStride = 4;
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            indexStride = 1;
                            break;
                        default:
                            throw std::runtime_error("Unsupported index component type");
                    }
                    
                    // Get attributes (position, texcoords, etc.)
                    // Note: POSITION accessor should always be present
                    if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
                        continue; // Skip if no positions
                    }

                    const tinygltf::Accessor &posAccessor = model.accessors[primitive.attributes.at("POSITION")];
                    const tinygltf::BufferView &posBufferView = model.bufferViews[posAccessor.bufferView];
                    const tinygltf::Buffer &posBuffer = model.buffers[posBufferView.buffer];

                    const float *normalData = nullptr;
                    size_t normalStride = 0;
                    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                        const tinygltf::Accessor &normalAccessor = model.accessors[primitive.attributes.at("NORMAL")];
                        const tinygltf::BufferView &normalBufferView = model.bufferViews[normalAccessor.bufferView];
                        const tinygltf::Buffer &normalBuffer = model.buffers[normalBufferView.buffer];

                        normalData = reinterpret_cast<const float *>(
                            &normalBuffer.data[normalBufferView.byteOffset + normalAccessor.byteOffset]);
                        
                        normalStride = normalAccessor.ByteStride(normalBufferView) ? 
                            (normalAccessor.ByteStride(normalBufferView) / sizeof(float)) : 3;
                    }

                    const float *tangentData = nullptr;
                    size_t tangentStride = 0;
                    
                    if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
                        const tinygltf::Accessor &tangentAccessor = model.accessors[primitive.attributes.at("TANGENT")];
                        const tinygltf::BufferView &tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
                        const tinygltf::Buffer &tangentBuffer = model.buffers[tangentBufferView.buffer];

                        tangentData = reinterpret_cast<const float *>(
                            &tangentBuffer.data[tangentBufferView.byteOffset + tangentAccessor.byteOffset]);
                        
                        tangentStride = tangentAccessor.ByteStride(tangentBufferView) ? 
                            (tangentAccessor.ByteStride(tangentBufferView) / sizeof(float)) : 4;
                    }
                    
                    const float *posData = reinterpret_cast<const float *>(
                        &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);
                    
                    size_t vertexCount = posAccessor.count;
                    size_t posStride = posAccessor.ByteStride(posBufferView) ? 
                        (posAccessor.ByteStride(posBufferView) / sizeof(float)) : 3;
                    
                    // Get texture coordinates if available
                    const float *texCoordData = nullptr;
                    size_t texCoordStride = 0;
                    
                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor &texCoordAccessor = 
                            model.accessors[primitive.attributes.at("TEXCOORD_0")];
                        const tinygltf::BufferView &texCoordBufferView = 
                            model.bufferViews[texCoordAccessor.bufferView];
                        const tinygltf::Buffer &texCoordBuffer = 
                            model.buffers[texCoordBufferView.buffer];
                        
                        texCoordData = reinterpret_cast<const float *>(
                            &texCoordBuffer.data[texCoordBufferView.byteOffset + texCoordAccessor.byteOffset]);
                        
                        texCoordStride = texCoordAccessor.ByteStride(texCoordBufferView) ? 
                            (texCoordAccessor.ByteStride(texCoordBufferView) / sizeof(float)) : 2;
                    }
                    
                    // Process vertices
                    std::vector<Vertex> tempVertices;
                    tempVertices.reserve(vertexCount);
                    
                    for (size_t v = 0; v < vertexCount; v++) {
                        Vertex vertex{};
                        
                        // Position
                        vertex.pos.x = posData[v * posStride];
                        vertex.pos.y = posData[v * posStride + 1];
                        vertex.pos.z = posData[v * posStride + 2];

                        vertex.pos = glm::vec3(rootMat * glm::vec4(vertex.pos, 1.0f));

                        // Update bounding box
                        minVec.x = std::min(minVec.x, vertex.pos.x);
                        minVec.y = std::min(minVec.y, vertex.pos.y);
                        minVec.z = std::min(minVec.z, vertex.pos.z);
                        
                        maxVec.x = std::max(maxVec.x, vertex.pos.x);
                        maxVec.y = std::max(maxVec.y, vertex.pos.y);
                        maxVec.z = std::max(maxVec.z, vertex.pos.z);
                        
                        vertexCenter += vertex.pos;
                        
                        // Texture coordinates
                        if (texCoordData) {
                            vertex.texCoord.x = texCoordData[v * texCoordStride];
                            vertex.texCoord.y = texCoordData[v * texCoordStride + 1];
                        } else {
                            vertex.texCoord = {0.0f, 0.0f}; // Default
                        }

                        // store the normal
                        if (normalData) {
                            vertex.normal.x = normalData[v * normalStride];
                            vertex.normal.y = normalData[v * normalStride + 1];
                            vertex.normal.z = normalData[v * normalStride + 2];
                        } else {
                            vertex.normal = {0.0f, 1.0f, 0.0f}; // default normal is up
                        }
                        if (tangentData) {
                            vertex.tangent.x = tangentData[v * tangentStride];
                            vertex.tangent.y = tangentData[v * tangentStride + 1];
                            vertex.tangent.z = tangentData[v * tangentStride + 2];
                            vertex.tangent.w = tangentData[v * tangentStride + 3];
                        } else {
                            vertex.tangent = (std::abs(vertex.normal.y) < 0.99f) ? 
                                glm::vec4(glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), vertex.normal)), 1.0) :
                                glm::vec4(glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), vertex.normal)), 1.0);
                        }

                        // vertex.normal = glm::vec3(rootMat * glm::vec4(vertex.normal, 1.0f));
                        // vertex.tangent = glm::vec3(rootMat * glm::vec4(vertex.tangent, 1.0f));
                        // vertex.bitangent = glm::normalize(glm::cross(vertex.tangent, vertex.normal));

                        glm::mat3 normalMatrix = glm::mat3(rootMat);
                        vertex.normal = normalMatrix * vertex.normal;
                        vertex.tangent = rootMat * vertex.tangent;
                        vertex.normal = glm::normalize(vertex.normal);
                        vertex.tangent = glm::normalize(vertex.tangent);

                        vertex.bitangent = glm::cross( vertex.normal, glm::vec3(vertex.tangent) ) * vertex.tangent.w;
                        vertex.bitangent = glm::normalize(vertex.bitangent);

                        if (glm::dot(glm::cross(vertex.normal, glm::vec3(vertex.tangent)), vertex.bitangent) < 0.0f){
                            vertex.tangent = vertex.tangent * -1.0f;
                        }
                        
                        // Store the texture ID and normal ID
                        vertex.textureID = textureID;
                        vertex.normalID = normalID;
                        
                        tempVertices.push_back(vertex);
                    }
                    
                    // Process indices and create final vertices and indices
                    for (size_t idx = 0; idx < indexCount; idx++) {
                        uint32_t vertexIndex = 0;
                        
                        // Extract index based on component type
                        switch (indexAccessor.componentType) {
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                                vertexIndex = *reinterpret_cast<const uint16_t *>(indexData + idx * indexStride);
                                break;
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                                vertexIndex = *reinterpret_cast<const uint32_t *>(indexData + idx * indexStride);
                                break;
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                                vertexIndex = *reinterpret_cast<const uint8_t *>(indexData + idx * indexStride);
                                break;
                        }
                        
                        const Vertex &vertex = tempVertices[vertexIndex];
                        
                        // Check if this vertex already exists
                        if (uniqueVertices.count(vertex) == 0) {
                            uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                            vertices.push_back(vertex);
                        }
                        
                        // Add index to final indices list
                        indices.push_back(uniqueVertices[vertex]);
                    }
                }
            }
        }
        
        vertices.shrink_to_fit();
        indices.shrink_to_fit();
        
        // Calculate final model metrics
        if (!vertices.empty()) {
            vertexCenter /= static_cast<float>(vertices.size());
        }
        
        // Store model AABB
        AA = minVec;
        BB = maxVec;
        
        // Center the vertices around origin
        glm::vec3 centerPoint = (AA + BB) * 0.5f;
        for (auto &vertex : vertices) {
            vertex.pos -= centerPoint;
        }
        
        // Update vertexCenter to reflect the centered position
        vertexCenter = glm::vec3(0.0f);
    }
    
};



#include "GLFW/glfw3.h"

namespace Engine {
    inline int frames;
    inline int fps;
    inline double deltaTime;
    inline double lastTimeFps = engineGetTime();
    inline double lastTimeDeltaTime = engineGetTime();
    inline void *nextScene = nullptr;

    inline glm::mat4 projectionMatrix = glm::mat4(0.0);

    inline glm::vec3 lightPos;
    inline glm::vec3 camPos;
    inline int enableNormal;
};

// physics
#include "btBulletDynamicsCommon.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"

// engine utility functions
inline btVector3 worldToPhysics(glm::vec3 pos) {
    return btVector3(pos.x, pos.y, pos.z);
}
inline glm::vec3 physicsToWorld(btVector3 pos) {
    return glm::vec3(pos.getX(),pos.getY(),pos.getZ()) / glm::vec3(1.0);
}
inline void countFrame() {
    double now = glfwGetTime();
    if (now - Engine::lastTimeFps >= 1.0) {
        Engine::fps = Engine::frames;
        Engine::frames = 0;
    }
    Engine::deltaTime = now - Engine::lastTimeDeltaTime;

    Engine::lastTimeFps = now;
    Engine::lastTimeDeltaTime = now;
}
inline double getDeltaTime() {
    return Engine::deltaTime;
}
inline int getFPS() {
    return Engine::fps;
}

// FIXME: dangerous pointer casting, make better scene queue system
inline void loadScene(void *scene) {
    Engine::nextScene = scene;
}

namespace DeviceProperties {
    inline bool enableNonUniform = false;
    inline void init() {
        VkPhysicalDeviceFeatures2 features = {};
        VkPhysicalDeviceVulkan12Features features12 = {};
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features.pNext = &features12;

        vkGetPhysicalDeviceFeatures2(VK::physicalDevice, &features);
        enableNonUniform = features12.shaderSampledImageArrayNonUniformIndexing;
    }
};
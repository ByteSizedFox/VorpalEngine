#pragma once

#include <volk.h>
#include "Engine/Window.h"
#include <vector>
#include <fstream>

namespace Engine {
    inline Window window;
    inline VkInstance vk_instance;
    inline VkDebugUtilsMessengerEXT debugMessenger;
    inline VkSurfaceKHR surface;

    inline VkPhysicalDevice vk_physicalDevice = VK_NULL_HANDLE;
    inline VkDevice device;
    
    inline VkQueue graphicsQueue;
    inline VkQueue presentQueue;

    inline VkSwapchainKHR swapChain;
    inline std::vector<VkImage> swapChainImages;
    inline VkFormat swapChainImageFormat;
    inline VkExtent2D swapChainExtent;
    inline std::vector<VkImageView> swapChainImageViews;
    inline std::vector<VkFramebuffer> swapChainFramebuffers;

    inline VkRenderPass renderPass;
    inline VkPipelineLayout pipelineLayout;
    inline VkPipeline graphicsPipeline;

    inline VkCommandPool commandPool;
    inline std::vector<VkCommandBuffer> commandBuffers;

    inline std::vector<VkSemaphore> imageAvailableSemaphores;
    inline std::vector<VkSemaphore> renderFinishedSemaphores;
    inline std::vector<VkFence> inFlightFences;
    inline uint32_t currentFrame = 0;
    
    inline bool framebufferResized = false;

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
};
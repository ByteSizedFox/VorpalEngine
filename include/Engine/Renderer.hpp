#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/hash.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>
#include <unordered_map>
#include <algorithm>

#include "Engine/Engine.hpp"
#include "Engine/Vertex.hpp"
#include "Engine/Mesh3D.hpp"
#include "Engine/Window.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Scene.hpp"
#include "Engine/PhysicsManager.hpp"
#include "VK/Validation.hpp"

// waylandTests
//#include "Engine/WindowSystem.hpp"

// imgui
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"

#include <array>

#ifdef _WIN32
#define NDEBUG
#endif

class Renderer {
public: 
    // test
    //WaylandWindow wayWin;
    // end test
    void run() {
        Logger::info("Renderer Run", "Init Window...");
        //wayWin.initSurface();
        window.init(800, 600);
        Logger::info("Renderer Run", "Init Vulkan...");
        initVulkan();
        DeviceProperties::init();

        Logger::info("Renderer Run", "Init ImGui...");
        initImGui();
        Logger::success("Renderer Run", "Init Done!");
    }

    void setScene(Scene *scene) {
        // wait for resources to finish to free buffer usage so we can unload scene
        vkDeviceWaitIdle(VK::device);

        // if scene valid and not loaded, load scene
        if (scene != nullptr && !scene->isReady) {
            scene->init();
        }

        // destroy old scene if it is valid (disabled for testing)
        if (currentScene != nullptr) {
            currentScene->destroy();
        }

        currentScene = scene;

        // neither current or new scene are valid, load fallback scene
        if (currentScene == nullptr) {
            throw std::runtime_error(std::string("Error: Scene Is Null"));
        }
        //recreateRender(enable_multisample);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            updateDescriptorSets(i);
        }

    }

//private:
    Window window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkRenderPass uiRenderPass;

    VkDescriptorSetLayout descriptorSetLayout;

    VkSampler textureSampler;

    // wip dual pipeline
    VkPipelineLayout pipelineLayout;
    VkPipelineLayout skyboxPipelineLayout;
    VkPipelineLayout uiPipelineLayout;

    // WIP dual pipeline
    VkPipeline pipeline3D;
    VkPipeline skyboxPipeline;
    VkPipeline uiPipeline;

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    
    VkDescriptorPool descriptorPool;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0;

    // descriptors
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    std::vector<VkBuffer> storageBuffers;
    std::vector<VkDeviceMemory> storageBuffersMemory;
    std::vector<void*> storageBuffersMapped;

    Scene *currentScene;
    Mesh3D skybox;

    uint64_t currentTimelineValue = 0;
    
    // render settings
    bool enable_multisample = true;
    bool enable_vsync = true;

    void recreateRender(bool multisample) {
        vkDeviceWaitIdle(VK::device);   

        enable_multisample = multisample;

        // cleanup imgui
        ImGui_ImplVulkan_Shutdown();
        ImGui::DestroyContext();
        // setup ImGui
        initImGui();

        vkDestroyPipeline(VK::device, pipeline3D, nullptr);
        vkDestroyPipeline(VK::device, skyboxPipeline, nullptr);
        vkDestroyPipelineLayout(VK::device, pipelineLayout, nullptr);
        vkDestroyPipelineLayout(VK::device, skyboxPipelineLayout, nullptr);

        vkDestroyRenderPass(VK::device, renderPass, nullptr);
        vkDestroyRenderPass(VK::device, uiRenderPass, nullptr);

        renderPass = createRenderPass(false);
        uiRenderPass = createRenderPass(true);

        recreateSwapChain();

        pipeline3D = createGraphicsPipeline(pipelineLayout, "assets/shaders/vert.spv", "assets/shaders/frag.spv", true, true);
        skyboxPipeline = createGraphicsPipeline(skyboxPipelineLayout, "assets/shaders/sky.vert.spv", "assets/shaders/sky.frag.spv", false, false);
        uiPipeline = createGraphicsPipeline(skyboxPipelineLayout, "assets/shaders/vert.spv", "assets/shaders/ui.frag.spv", true, true);
    }

    void ImGuiTheme() {
        ImGuiStyle* style = &ImGui::GetStyle();
        style->WindowRounding = 16.0f;
        style->WindowTitleAlign = ImVec2(0.5f, 0.5f);
    }

    void initImGui() {
        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = instance;
        init_info.PhysicalDevice = VK::physicalDevice;
        init_info.Device = VK::device;
        init_info.QueueFamily = findQueueFamilies(VK::physicalDevice).graphicsFamily.value();
        init_info.RenderPass = uiRenderPass;
        init_info.Queue = VK::graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = descriptorPool;
        init_info.Subpass = 0;
        init_info.MinImageCount = 2;
        init_info.ImageCount = swapChainFramebuffers.size();
        if (enable_multisample) {
            init_info.MSAASamples = msaaSamples;
        } else {
            init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        }
        ImGui_ImplVulkan_Init(&init_info);

        // Upload ImGui fonts
        VkCommandBuffer commandBuffer = Command::beginSingleTimeCommands();
        ImGui_ImplVulkan_CreateFontsTexture();
        Command::endSingleTimeCommands(commandBuffer);

        // disable config file
        io.IniFilename = NULL;
        io.LogFilename = NULL;

        ImGuiTheme();
    }

    void initVulkan() {
        enable_multisample = true;
        enable_vsync = false;

        volkInitialize();
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();

        createSwapChain(enable_vsync);
        createImageViews();

        renderPass = createRenderPass(false);
        uiRenderPass = createRenderPass(true);

        createCommandPool();
        createColorResources();
        
        createDepthResources();
        createFramebuffers();

        createCommandBuffers();
        createTextureSampler();

        descriptorSetLayout = createDescriptorSetLayout(false);

        pipeline3D = createGraphicsPipeline(pipelineLayout, "assets/shaders/vert.spv", "assets/shaders/frag.spv", true, true);
        skyboxPipeline = createGraphicsPipeline(skyboxPipelineLayout, "assets/shaders/sky.vert.spv", "assets/shaders/sky.frag.spv", false, false);
        uiPipeline = createGraphicsPipeline(uiPipelineLayout, "assets/shaders/vert.spv", "assets/shaders/ui.frag.spv", true, true);

        createDescriptorPool();        
        createSyncObjects();
        createUniformBuffers();
        setupUI(); // create UI textures before descriptorsets
        // load placeholder object for fallback textures
        skybox.init("assets/models/skybox.glb");
        skybox.setPosition({0.0f, 0.0f, -1.0f});
        skybox.setRotation({0.0, 0.0, glm::radians(0.0f)});

        createDescriptorSets();
    }

    void cleanupSwapChain() {
        vkDestroyImageView(VK::device, depthImageView, nullptr);
        vkDestroyImage(VK::device, depthImage, nullptr);
        vkFreeMemory(VK::device, depthImageMemory, nullptr);

        vkDestroyImageView(VK::device, colorImageView, nullptr);
        vkDestroyImage(VK::device, colorImage, nullptr);
        vkFreeMemory(VK::device, colorImageMemory, nullptr);

        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(VK::device, framebuffer, nullptr);
        }

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(VK::device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(VK::device, swapChain, nullptr);

        // ui cleanup
        vkDestroyFramebuffer(VK::device, uiFramebuffer, nullptr);
        vkDestroyImageView(VK::device, uiTextureView, nullptr);
        vkDestroyImage(VK::device, uiTexture, nullptr);
        vkFreeMemory(VK::device, uiTextureMemory, nullptr);
        
    }

    void cleanup() {
        // cleanup imgui
        ImGui_ImplVulkan_Shutdown();
        //ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        cleanupSwapChain();

        vkDestroyPipeline(VK::device, pipeline3D, nullptr);
        vkDestroyPipeline(VK::device, skyboxPipeline, nullptr);
        vkDestroyPipeline(VK::device, uiPipeline, nullptr);
        
        vkDestroyPipelineLayout(VK::device, pipelineLayout, nullptr);
        vkDestroyPipelineLayout(VK::device, skyboxPipelineLayout, nullptr);
        vkDestroyPipelineLayout(VK::device, uiPipelineLayout, nullptr);

        vkDestroyRenderPass(VK::device, renderPass, nullptr);
        vkDestroyRenderPass(VK::device, uiRenderPass, nullptr);

#ifdef DRAW_DEBUG
        debugMesh.destroy();
#endif
        
        vkDestroySampler(VK::device, textureSampler, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(VK::device, uniformBuffers[i], nullptr);
            vkFreeMemory(VK::device, uniformBuffersMemory[i], nullptr);
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(VK::device, storageBuffers[i], nullptr);
            vkFreeMemory(VK::device, storageBuffersMemory[i], nullptr);
        }

        currentScene->destroy();
        skybox.destroy();
        
        // texture cleanup
        for (int i = 0; i < VK::g_texturePathList.size(); i++) {
            VK::textureMap[VK::g_texturePathList[i]].destroy();
        }

        vkDestroyDescriptorPool(VK::device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(VK::device, descriptorSetLayout, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(VK::device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(VK::device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(VK::device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(VK::device, VK::commandPool, nullptr);
        vkDestroyDevice(VK::device, nullptr);

        if (ENABLE_DEBUG) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, VK::surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        window.destroy();
        glfwTerminate();
    }

    void recreateSwapChain() {
        vkDeviceWaitIdle(VK::device);

        int width = 0, height = 0;
        window.GetFramebufferSize(&width, &height);
        while (width == 0 || height == 0) {
            window.GetFramebufferSize(&width, &height);
            glfwWaitEvents();
        }

        cleanupSwapChain();

        createSwapChain(enable_vsync);
        createImageViews();
        createColorResources();
        createDepthResources();
        createFramebuffers();

        setupUI();
        uiNeedsUpdate = true;
    }

    void createInstance() {
        if (ENABLE_DEBUG && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Unnamed";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Vorpal Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (ENABLE_DEBUG) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
        volkLoadInstance(instance);
    }

    void setupDebugMessenger() {
        if (!ENABLE_DEBUG) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window.get(), nullptr, &VK::surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        //VK::surface = wayWin.getSurface(instance);
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                VK::physicalDevice = device;
                msaaSamples = getMaxUsableSampleCount();
                break;
            }
        }

        if (VK::physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(VK::physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkPhysicalDeviceVulkan12Features features {};
        memset(&features, false, sizeof(VkPhysicalDeviceVulkan12Features));
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features.timelineSemaphore = true;
        features.pNext = nullptr;
        //features.runtimeDescriptorArray = true;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
#ifdef ENABLE_VULKAN_12_FEATURES
        createInfo.pNext = &features;
#endif

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(VK::physicalDevice, &properties);

        // we disable these extensions on intel because it's messed up
        if (properties.vendorID != 0x8086) {  // Not Intel
            deviceExtensions.push_back(VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME);
            deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        }

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (ENABLE_DEBUG) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(VK::physicalDevice, &createInfo, nullptr, &VK::device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(VK::device, indices.graphicsFamily.value(), 0, &VK::graphicsQueue);
        vkGetDeviceQueue(VK::device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapChain(bool vsync) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(VK::physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode;
        if (vsync) {
            presentMode = VK_PRESENT_MODE_FIFO_KHR;
        } else {
            presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        }

        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = VK::surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        QueueFamilyIndices indices = findQueueFamilies(VK::physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(VK::device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(VK::device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(VK::device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (uint32_t i = 0; i < swapChainImages.size(); i++) {
            swapChainImageViews[i] = Image::createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
        
    }
    
    VkRenderPass createRenderPass(bool isUI) {
        VkRenderPass resultPass;

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        if (enable_multisample) {
            colorAttachment.samples = msaaSamples;
        } else {
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        }
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (enable_multisample) {
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        } else {
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = Utils::findDepthFormat();
        if (enable_multisample) {
            depthAttachment.samples = msaaSamples;
        } else {
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        }
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = swapChainImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (isUI) {
            colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        } else {
            colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        if (isUI) {
            colorAttachmentResolveRef.attachment = 1;
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        if (!isUI) {
            subpass.pDepthStencilAttachment = &depthAttachmentRef;
        }
        if (enable_multisample) {
            subpass.pResolveAttachments = &colorAttachmentResolveRef;
        }

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::vector<VkAttachmentDescription> attachments;
        if (enable_multisample) {
            if (isUI) {
                attachments = { colorAttachment, colorAttachmentResolve };
            } else {
                attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
            }
        } else {
            if (isUI) {
                attachments = { colorAttachment };
            } else {
                attachments = { colorAttachment, depthAttachment };
            }
        }

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        if (enable_multisample) {
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;
        } else {
            renderPassInfo.dependencyCount = 0;
        }
        if (vkCreateRenderPass(VK::device, &renderPassInfo, nullptr, &resultPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
        return resultPass;
    }

    // WIP UI
    VkImage uiTexture;
    VkDeviceMemory uiTextureMemory;
    VkImageView uiTextureView;
    VkFramebuffer uiFramebuffer;
    bool uiNeedsUpdate = false;

    void setupUI() {
        Image::createImage(UI_WIDTH, UI_HEIGHT, 1, VK_SAMPLE_COUNT_1_BIT, swapChainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uiTexture, uiTextureMemory, "UI Image");
        uiTextureView = Image::createImageView(uiTexture, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        std::vector<VkImageView> attachments;

        if (enable_multisample) {
            attachments = {
                colorImageView,
                uiTextureView
            };
        } else {
            attachments = {
                uiTextureView
            };
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = uiRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = UI_WIDTH;
        framebufferInfo.height = UI_HEIGHT;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(VK::device, &framebufferInfo, nullptr, &uiFramebuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

    VkDescriptorSetLayout createDescriptorSetLayout(bool isUI) {
        VkDescriptorSetLayout resultLayout;

        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 75;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 2;
        samplerBinding.descriptorCount = 1;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerBinding.pImmutableSamplers = nullptr;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding uiSamplerBinding{};
        uiSamplerBinding.binding = 3;
        uiSamplerBinding.descriptorCount = 1;
        uiSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        uiSamplerBinding.pImmutableSamplers = nullptr;
        uiSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding storageBinding{};
        storageBinding.binding = 4;
        storageBinding.descriptorCount = 1;
        storageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        storageBinding.pImmutableSamplers = nullptr;
        storageBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        if (isUI) {
            samplerBinding.binding = 0;
            bindings.resize(2);
            bindings = {samplerBinding, storageBinding};
        } else {
            bindings.resize(5);
            bindings = {uboLayoutBinding, samplerLayoutBinding, samplerBinding, uiSamplerBinding, storageBinding};
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(VK::device, &layoutInfo, nullptr, &resultLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        return resultLayout;
    }
    
    VkPipeline createGraphicsPipeline(VkPipelineLayout& layout, const char* vertex_path, const char* fragment_path, bool enableDepth, bool enableBlend) {
        VkPipeline result;

        auto vertShaderCode = Utils::readFileZip(vertex_path);
        auto fragShaderCode = Utils::readFileZip(fragment_path);

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        if (enable_multisample) {
            multisampling.rasterizationSamples = msaaSamples;
        } else {
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        }

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = enableDepth?VK_TRUE:VK_FALSE;
        depthStencil.depthWriteEnable = enableDepth?VK_TRUE:VK_FALSE;
        depthStencil.depthCompareOp = enableDepth?VK_COMPARE_OP_LESS:VK_COMPARE_OP_ALWAYS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        if (enableBlend) {
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        } else {
            colorBlendAttachment.blendEnable = VK_FALSE;
        }

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        if (enableBlend) {
            colorBlending.logicOp = VK_LOGIC_OP_SET;
        } else {
            colorBlending.logicOp = VK_LOGIC_OP_COPY;
        }
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;

        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        
        //setup push constants
	    VkPushConstantRange push_constant;
	    push_constant.offset = 0;
	    push_constant.size = sizeof(ModelBufferObject);
	    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        pipelineLayoutInfo.pPushConstantRanges = &push_constant;
	    pipelineLayoutInfo.pushConstantRangeCount = 1;

        if (vkCreatePipelineLayout(VK::device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;

        pipelineInfo.layout = layout;

        pipelineInfo.renderPass = renderPass;

        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(VK::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &result) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(VK::device, fragShaderModule, nullptr);
        vkDestroyShaderModule(VK::device, vertShaderModule, nullptr);

        return result;
    }
    
    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::vector<VkImageView> attachments;

            if (enable_multisample) {
                attachments = {
                    colorImageView,
                    depthImageView,
                    swapChainImageViews[i]
                };
            } else {
                attachments = {
                    swapChainImageViews[i],
                    depthImageView,
                };
            }

            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();

            if (vkCreateFramebuffer(VK::device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createTextureSampler() {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(VK::physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(20.0);
        samplerInfo.mipLodBias = 0.0f;

        if (vkCreateSampler(VK::device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(VK::physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(VK::device, &poolInfo, nullptr, &VK::commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }
    }

    void createColorResources() {
        VkFormat colorFormat = swapChainImageFormat;
        if (enable_multisample) {
            Image::createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory, "Color Image");
        } else {
            Image::createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory, "Color Image");
        }
        colorImageView = Image::createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void createDepthResources() {
        VkFormat depthFormat = Utils::findDepthFormat();
        if (enable_multisample) {
            Image::createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory, "DEPTH image");
        } else {
            Image::createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory, "DEPTH image");
        }
        depthImageView = Image::createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }

    VkSampleCountFlagBits getMaxUsableSampleCount() {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(VK::physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 5> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 75);

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        poolSizes[3].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[4].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 75);
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        if (vkCreateDescriptorPool(VK::device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = VK::commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(VK::device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    bool window_open = true;
    int incr = 0;
    bool enablePopup = false;
    float lastPlayerSpeed = 0.0;

    void drawUI() {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(VK::physicalDevice, &properties);
        
        ImGui::SetNextWindowSize(ImVec2(UI_WIDTH, UI_HEIGHT));
        ImGui::SetNextWindowPos(ImVec2(0,0));
        
        ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);
        
        glm::vec3 pos = currentScene->camera.getPosition();

        glm::vec3 vel = glm::pow(currentScene->camera.getVelocity(), glm::vec3(2));
        float playerSpeed = (sqrt(vel.x+vel.y+vel.z) + lastPlayerSpeed) / 2.0f;
        lastPlayerSpeed = playerSpeed;

        ImGui::Text("XYZ: %.2f %.2f %.2f", pos.x, pos.y, pos.z);
        ImGui::Text("Speed: %.2f m/s", playerSpeed);

        ImGui::Text("Press Escape To Exit");
        ImGui::Text("Device Info:");
        ImGui::Text("GPU VendorID: 0x%04X", properties.vendorID);
        ImGui::Text("GPU DeviceID: 0x%04X", properties.deviceID);
        ImGui::Text("GPU Name: %s", properties.deviceName);
        ImGui::Text("GPU Driver Version: %i", properties.driverVersion);

        if (ImGui::Button("Press Me!")) {
            enablePopup = true;
        }

        ImGui::End();
    }

    void recordUI(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer frameBuffer) {
        Image::transitionImageLayout(uiTexture, swapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1); 
        VkViewport viewport{};
        VkRect2D scissor{};
        VkRenderPassBeginInfo renderPassInfo{};

        if (true) {
            // ui renderpass
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = frameBuffer;
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {UI_WIDTH, UI_HEIGHT};
            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
            clearValues[1].depthStencil = {1.0f, 0};

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    viewport.x = 0.0f;
                    viewport.y = 0.0f;
                    viewport.width = (float) UI_WIDTH;
                    viewport.height = (float) UI_HEIGHT;
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                scissor.offset = {0, 0};
                scissor.extent = {
                    UI_WIDTH,
                    UI_HEIGHT
                };
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                // draw imgui
                ImGuiIO& io = ImGui::GetIO();
                io.DisplaySize = ImVec2(UI_WIDTH, UI_HEIGHT);

                ImGui_ImplVulkan_NewFrame();
                ImGui::NewFrame();

                //drawUI();
                currentScene->drawUI(&window);

                ImGui::Render();
                ImDrawData* drawData = ImGui::GetDrawData();
                ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
            vkCmdEndRenderPass(commandBuffer);
            uiNeedsUpdate = false;
        }
        // transition UI framebuffer back
        //Image::transitionImageLayout(uiTexture, swapChainImageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);   
    }
    
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        recordUI(commandBuffer, uiRenderPass, uiFramebuffer);

        VkImageMemoryBarrier imageBarrier = {};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = uiTexture;
        imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange.baseMipLevel = 0;
        imageBarrier.subresourceRange.levelCount = 1;
        imageBarrier.subresourceRange.baseArrayLayer = 0;
        imageBarrier.subresourceRange.layerCount = 1;
        imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // wait for renderpass #1 finishes writing to color attachment
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // wait at VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            0, 
            0, nullptr,
            0, nullptr,
            1, &imageBarrier); // Do not need any layout transitions. Do we need VkMemoryBarrier? If so, what srcAccess/dstAccess ?

        // update the gui when fps changes
        if (uiNeedsUpdate && window_open) {
            updateDescriptorSets(currentFrame);
        }

        //Image::transitionImageLayout(uiTexture, swapChainImageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

        VkRenderPassBeginInfo renderPassInfo{};

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.53f, 0.81f, 0.92f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        // 3d renderpass
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            // descriptors contain all per-frame data
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &(descriptorSets)[currentFrame], 0, nullptr);

            // render skybox
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);
            skybox.draw(commandBuffer, pipelineLayout, 1);

            // begin main draw
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline3D);

#ifdef DRAW_DEBUG
            prepareDebugMesh();
#endif

            if (currentScene != nullptr && currentScene->isReady) {
                currentScene->draw(commandBuffer, pipelineLayout, &window);
            }

            // render ui mesh
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, uiPipeline);
            currentScene->uiMesh.draw(commandBuffer, uiPipelineLayout, 1);

#ifdef DRAW_DEBUG
            if (debugMesh.m_indices.size() != 0) {
                debugMesh.draw(commandBuffer, pipelineLayout);
                physManager->debugDrawer->clear();
            }
#endif

        vkCmdEndRenderPass(commandBuffer);
        
        //blitRenderTargetToSwapChain(commandBuffer, imageIndex);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(VK::device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(VK::device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(VK::device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    Mesh3D debugMesh;
    void prepareDebugMesh() {
        if (debugMesh.m_indices.size() != 0) {
            debugMesh.destroy();
        }

        MyDebugDrawer *debugDrawer = currentScene->physManager->debugDrawer;
        const std::vector<btVector3>& vertices = debugDrawer->getVertices();
        const std::vector<btVector3>& colors = debugDrawer->getColors();
        const std::vector<int>& indices = debugDrawer->getIndices();

        std::vector<Vertex> rawVertices(vertices.size());
        std::vector<uint32_t> rawIndices(indices.size());
        for (int i = 0; i < indices.size(); i++) {
            btVector3 vert = vertices[indices[i]];
            btVector3 color = colors[indices[i]];
            rawVertices[indices[i]].pos = glm::vec3(vert.getX(), vert.getY(), vert.getZ());
            rawVertices[indices[i]].normal = glm::vec3(color.getX(), color.getY(), color.getZ());
            rawIndices[i] = indices[i];
        }
        
        debugMesh.loadRaw(rawVertices, rawIndices, "Debug Mesh");
        debugMesh.setPosition({0.0,0.0,0.0});
        debugMesh.setRotation({0.0,0.0,0.0});
        debugMesh.isDebug = true;
    }

    void waitForFrame() {
        vkWaitForFences(VK::device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    }
    void resetFences() {
        vkResetFences(VK::device, 1, &inFlightFences[currentFrame]);
    }

    void drawFrame() {
        // can't draw non-scene
        if (currentScene == nullptr) {
            return;
        }

        waitForFrame();

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(VK::device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        resetFences();

        // if (window.wasResized()) {
        //     window.clearResized();
        //     window.updateProjectionMatrix(swapChainExtent.width, swapChainExtent.height);
        //     recreateSwapChain(enable_multisample);
        //     updateDescriptorSets(currentFrame);
        // }

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};

        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(VK::graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasResized()) {
            window.clearResized();
            window.updateProjectionMatrix(swapChainExtent.width, swapChainExtent.height);

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                updateUniformBuffer(i);
            }
            recreateSwapChain();
            updateDescriptorSets(currentFrame);
            //wayWin.commit();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        // load scene if not loaded
        if (!currentScene->isReady) {
            Logger::info("Renderer", "Loading Main Scene...");
            currentScene->init();
            Logger::success("Renderer", "Loaded Main Scene");
        }

        //wayWin.update();
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(VK::device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        // disable to force vsync
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            window.GetFramebufferSize(&width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    // descriptors
    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(VK::device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            updateDescriptorSets(i);
        }
    }

    void updateDescriptorSets(int frame) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[frame];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            std::vector<VkDescriptorImageInfo> imageInfos(75);
            for (size_t j = 0; j < VK::g_texturePathList.size(); ++j) {
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                Texture tex = VK::textureMap[VK::g_texturePathList[j]];
                int texID = tex.textureID;

                if (texID < VK::textureMap.size()) {
                    imageInfos[texID].imageView = tex.textureImageView;
                } else {
                    if (VK::textureMap.size() <= 0) {
                        printf("UH, NOT GOOD!, texture %i doesnt exist\n", (int) j);
                        imageInfos[texID].imageView = uiTextureView; //VK::textureMap[VK::g_texturePathList[0]].textureImageView;
                    } else {
                        imageInfos[texID].imageView = tex.textureImageView;
                    }
                }
                imageInfos[texID].sampler = textureSampler;
            }
            // fill in remaining unused images
            for (size_t j = VK::textureMap.size(); j < 75; ++j) {
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos[j].imageView = VK::textureMap.begin()->second.textureImageView;
                imageInfos[j].sampler = textureSampler;
            }

            std::array<VkWriteDescriptorSet, 5> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[frame];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[frame];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrites[1].descriptorCount = imageInfos.size();
            descriptorWrites[1].pImageInfo = (imageInfos.data());

            VkDescriptorImageInfo samplerInfo = {};

    	    samplerInfo.sampler = textureSampler;

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = descriptorSets[frame];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pImageInfo = &samplerInfo;

            VkDescriptorImageInfo uiImageInfo{};
            uiImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            uiImageInfo.imageView = uiTextureView;
            uiImageInfo.sampler = nullptr;

            descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[3].dstSet = descriptorSets[frame];
            descriptorWrites[3].dstBinding = 3;
            descriptorWrites[3].dstArrayElement = 0;
            descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrites[3].descriptorCount = 1;
            descriptorWrites[3].pImageInfo = &uiImageInfo;

            // instance wip
            VkDescriptorBufferInfo storageInfo;
            storageInfo.buffer = storageBuffers[frame];
            storageInfo.offset = 0;
            storageInfo.range = sizeof(StorageBufferObject);

            descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[4].dstSet = descriptorSets[frame];
            descriptorWrites[4].dstBinding = 4;
            descriptorWrites[4].dstArrayElement = 0;
            descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[4].descriptorCount = 1;
            descriptorWrites[4].pBufferInfo = &storageInfo;

            vkUpdateDescriptorSets(VK::device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            Memory::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
            vkMapMemory(VK::device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }

        // storage buffers
        bufferSize = sizeof(StorageBufferObject);
        storageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        storageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        storageBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            Memory::createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, storageBuffers[i], storageBuffersMemory[i]);
            vkMapMemory(VK::device, storageBuffersMemory[i], 0, bufferSize, 0, &storageBuffersMapped[i]);
        }
    }

    double deltaTime = 0.0;
    bool canJump = true;

    void handle_input() {
        currentScene->physManager->process(deltaTime);

        // handle mouse input
        glm::vec2 mouseVec = window.getMouseVector();
        currentScene->camera.pitch(-mouseVec.y * 0.001);
        currentScene->camera.yaw(mouseVec.x * 0.001);

        // handle user input
        glm::vec3 camPos = physicsToWorld(currentScene->camera.rigidBody->getInterpolationWorldTransform().getOrigin()) + glm::vec3(0.0, 0.5, 0.0);

        const glm::vec3 forward = currentScene->camera.getForward();

        // avg walk speed
        float speed = 1.42f;
        if (window.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
            speed = 3.0f;
        }
        speed *= 2.1336; // adjust for height scale difference

        glm::vec3 camVec = glm::vec3(0.0);

        glm::vec3 vel = glm::vec3(0.0);
        if (window.isKeyPressed(GLFW_KEY_W)) {
            vel += glm::normalize(glm::vec3(-forward.x, 0.0f, -forward.z)) * glm::vec3(1);
        }
        if (window.isKeyPressed(GLFW_KEY_S)) {
            vel -= glm::normalize(glm::vec3(-forward.x, 0.0f, -forward.z)) * glm::vec3(1);
        }
        if (window.isKeyPressed(GLFW_KEY_A)) {
            vel -= glm::cross(-forward, glm::vec3(0.0,1.0,0.0)) * glm::vec3(1);
        }
        if (window.isKeyPressed(GLFW_KEY_D)) {
            vel += glm::cross(-forward, glm::vec3(0.0,1.0,0.0)) * glm::vec3(1);
        }
        if (glm::length(vel) > 0.0001f) {
            vel = glm::normalize(vel) * speed;
        }
        
        currentScene->camera.rigidBody->setLinearVelocity(btVector3(vel.x, currentScene->camera.getVelY(), vel.z));

        // 1ft
        float jump_height = 3.048f;

        // ground test
        currentScene->camera.isGrounded(currentScene->physManager->dynamicsWorld);
        if (window.isKeyPressed(GLFW_KEY_SPACE)) { // jump
            if (currentScene->camera.grounded && canJump) {
                currentScene->camera.rigidBody->setLinearVelocity(btVector3(currentScene->camera.getVelX(), jump_height,  currentScene->camera.getVelZ()));
            }
            canJump = false;
        } else {
            canJump = true;
        }

        if (window.isKeyPressed(GLFW_KEY_J)) { // jump
            currentScene->camera.rigidBody->setLinearVelocity(btVector3(currentScene->camera.getVelX(), jump_height,  currentScene->camera.getVelZ()));
            canJump = false;
        }
        
        // skip UI stuff if window not open
        if (!window_open) {
            return;
        }

        // // raycast to 
        // glm::vec2 result;
        // glm::vec3 worldResult;
        // float distance = 1.0f;
        // bool hit = Experiment::raycastRectangle(camPos * glm::vec3(WORLD_SCALE), -forward, currentScene->quadVertices, currentScene->quadUVs, currentScene->uiMesh.m_indices, currentScene->uiMesh.getModelMatrix().model, currentScene->camera.getViewMatrix(), window.getProjectionMatrix(), distance, result, worldResult);

        // ImGuiIO& io = ImGui::GetIO();
        // if (distance <= 1.0 && hit) {
        //     uiNeedsUpdate = true;
        //     io.AddMousePosEvent(result.x * UI_WIDTH, result.y * UI_HEIGHT);
        //     io.MouseDrawCursor = true;
        // } else {
        //     if (io.MouseDrawCursor == true) {
        //         uiNeedsUpdate = true; // update UI to hide cursor
        //     }
        //     io.AddMousePosEvent(999.0, 999.0);
        //     io.MouseDrawCursor = false;
        // }

        // uiNeedsUpdate = true;
        // window_open = true;

        // if (window.isKeyPressed(GLFW_KEY_E)) {
        //     // dont use cursor when dragging
        //     io.MouseDrawCursor = false;
        //     io.AddMousePosEvent(999.0, 999.0);

        //     // get location in front of camera
        //     float distance = 1.0f;
        //     currentScene->uiMesh.setPosition(camPos + -forward * distance);
        //     glm::vec3 objectToCamera = camPos - currentScene->uiMesh.getPosition();

        //     // billboard rotation for the UI to the camera
        //     float yaw = std::atan2(objectToCamera.x, objectToCamera.z);
        //     float pitch = std::atan2(objectToCamera.y, std::sqrt(objectToCamera.x * objectToCamera.x + objectToCamera.z * objectToCamera.z));

        //     // Set the rotation
        //     currentScene->uiMesh.setRotation({-pitch, yaw, glm::radians(0.0f)});
        //     currentScene->uiMesh.updateModelMatrix();
        // }
    }

    void updateUniformBuffer(uint32_t currentImage) {
        UniformBufferObject ubo{};

        ubo.view = currentScene->camera.getViewMatrix();
        ubo.proj = Engine::projectionMatrix;
        ubo.proj[1][1] *= -1;
        
        ubo.time = glfwGetTime() + 300.0; // additional 5 minutes to start half way through the day

        // shader enabled features
        ubo.lightPos = Engine::lightPos;
        ubo.camPos = Engine::camPos;

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));  

        StorageBufferObject sbo{};
        for (int i = 0; i < 10; i++) {
            StorageBufferData data;
            data.isDebug = false;
            data.isUI = false;
            data.model = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 1.0, 0.0));
            sbo.data[i] = data;
        }
        sbo.data[0].isUI = true;
        memcpy(storageBuffersMapped[currentImage], &sbo, sizeof(sbo));
    }
};

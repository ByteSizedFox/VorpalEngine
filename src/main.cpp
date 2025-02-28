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

#include "Engine/Engine.hpp"
#include "Engine/Vertex.hpp"
#include "Engine/Mesh3D.hpp"
#include "Engine/Window.hpp"
#include "Engine/Camera.hpp"
#include "VK/Validation.hpp"

// imgui
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"

#include <thread>

#include <array>
#include <deque>

#ifdef _WIN32
#define NDEBUG
#endif

std::vector<Texture> textures;

class Application {
public:
    void run() {
        window.init(800, 600);
        initVulkan();
        initImGui();
        mainLoop();
        cleanup();
    }

private:
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
    VkPipelineLayout uiPipelineLayout;


    // WIP dual pipeline
    VkPipeline pipeline3D;

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

    Mesh3D mesh;
    Mesh3D mesh1;
    Mesh3D uiMesh;

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
#ifdef ENABLE_MULTISAMPLE
        init_info.MSAASamples = msaaSamples;
#else
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
#endif
        ImGui_ImplVulkan_Init(&init_info);

        // Upload ImGui fonts
        VkCommandBuffer commandBuffer = Command::beginSingleTimeCommands();
        ImGui_ImplVulkan_CreateFontsTexture();
        Command::endSingleTimeCommands(commandBuffer);

        // disable config file
        io.IniFilename = NULL;
        io.LogFilename = NULL;
    }

    void initVulkan() {
        printf("Volk Init\n");
        volkInitialize();
        printf("Instance Init\n");
        createInstance();
        printf("Debug Init\n");
        setupDebugMessenger();
        printf("Surface Init\n");
        createSurface();
        printf("Device Init\n");
        pickPhysicalDevice();
        createLogicalDevice();
        printf("SwapChain Init\n");
        createSwapChain();
        printf("ImageView Init\n");
        createImageViews();

        printf("Renderpass Init\n");
        renderPass = createRenderPass(false);
        uiRenderPass = createRenderPass(true);

        printf("CommandPool Init\n");
        createCommandPool();
        printf("Color Init\n");
        createColorResources();
        printf("Depth Init\n");
        createDepthResources();
        printf("Framebuffer Init\n");
        createFramebuffers();
        printf("CommandBuffer Init\n");
        createCommandBuffers();

        createTextureSampler();

        // we need UImesh immediately
        printf("uiMesh Init\n");
        uiMesh.init("assets/models/Cube.glb");
        mesh.init("assets/models/male_07.glb");
        printf("Mesh1 Init\n");
        mesh1.init("assets/models/flatgrass.glb");
            
        printf("Texture Init\n");
        textures.resize(VK::textureMap.size());
        for (auto& tex: VK::textureMap) {
            textures[tex.second.textureID] = tex.second;
            printf("TextureID: %i\n", tex.second.textureID);
        }

        printf("Descriptorset Layout Init\n");
        descriptorSetLayout = createDescriptorSetLayout(false);

        printf("Pipeline Init\n");
        pipeline3D = createGraphicsPipeline("assets/shaders/vert.spv", "assets/shaders/frag.spv");

        printf("DescriptorPool Init\n");
        createDescriptorPool();        
        printf("Sync Init\n");
        createSyncObjects();
        printf("Uniform Init\n");
        createUniformBuffers();
        
        printf("GUI Init\n");
        setupUI(); // create UI textures before descriptorsets
        printf("DescriptorSet Init\n");
        createDescriptorSets();
        
        printf("Uniform Init #2\n");
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            updateUniformBuffer(i);
        }

    }

    double last_time = 0;
    int frames = 0;
    int fps = 0;

    void mainLoop() {
        // update camera matrix
        window.getCamera()->updateMatrix();

        while (!window.ShouldClose()) {
            double now = glfwGetTime();
            frames++;

            glfwPollEvents();
            drawFrame();

            if (now - last_time >= 1.0) {
                fps = frames;
                frames = 0;
                last_time = now;
                printf("FPS: %i\n", fps);
                uiNeedsUpdate = true;
            }

            
        }

        vkDeviceWaitIdle(VK::device);
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
    }

    void cleanup() {
        // cleanup imgui
        ImGui_ImplVulkan_Shutdown();
        //ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        cleanupSwapChain();

        vkDestroyImageView(VK::device, uiTextureView, nullptr);
        vkDestroyImage(VK::device, uiTexture, nullptr);
        vkDestroyFramebuffer(VK::device, uiFramebuffer, nullptr);
        vkFreeMemory(VK::device, uiTextureMemory, nullptr);

        vkDestroyPipeline(VK::device, pipeline3D, nullptr);
        
        vkDestroyPipelineLayout(VK::device, pipelineLayout, nullptr);
        vkDestroyRenderPass(VK::device, renderPass, nullptr);
        vkDestroyRenderPass(VK::device, uiRenderPass, nullptr);

        mesh.destroy();
        mesh1.destroy();
        uiMesh.destroy();
        
        vkDestroySampler(VK::device, textureSampler, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(VK::device, uniformBuffers[i], nullptr);
            vkFreeMemory(VK::device, uniformBuffersMemory[i], nullptr);
        }

        for (int i = 0; i < textures.size(); i++) {
            Texture *texture = &textures[i];
            texture->destroy();
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
        int width = 0, height = 0;
        window.GetFramebufferSize(&width, &height);
        while (width == 0 || height == 0) {
            window.GetFramebufferSize(&width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(VK::device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createColorResources();
        createDepthResources();
        createFramebuffers();

        // recreate UI framebuffer
        vkDestroyImageView(VK::device, uiTextureView, nullptr);
        vkDestroyImage(VK::device, uiTexture, nullptr);
        vkDestroyFramebuffer(VK::device, uiFramebuffer, nullptr);
        vkFreeMemory(VK::device, uiTextureMemory, nullptr);
        
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
        //memset(&features, false, sizeof(VkPhysicalDeviceVulkan12Features));
        //features.pNext = nullptr;
        //features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        //features.runtimeDescriptorArray = true;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        //createInfo.pNext = &features;

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

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(VK::physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
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
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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
#ifdef ENABLE_MULTISAMPLE
        colorAttachment.samples = msaaSamples;
#else
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
#endif
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
#ifdef ENABLE_MULTISAMPLE
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#else
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
#endif

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = Utils::findDepthFormat();
#ifdef ENABLE_MULTISAMPLE
        depthAttachment.samples = msaaSamples;
#else
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
#endif
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
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        

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
#ifdef ENABLE_MULTISAMPLE
        subpass.pResolveAttachments = &colorAttachmentResolveRef;
#endif

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::vector<VkAttachmentDescription> attachments;
#ifdef ENABLE_MULTISAMPLE
        if (isUI) {
            
            attachments = { colorAttachment, colorAttachmentResolve };
        } else {
            attachments = {colorAttachment, depthAttachment, colorAttachmentResolve };
        }
#else
        if (isUI) {
            attachments = { colorAttachment };
        } else {
            attachments = { colorAttachment, depthAttachment };
        }
#endif

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
#ifdef ENABLE_MULTISAMPLE
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
#else
        renderPassInfo.dependencyCount = 0;
#endif
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
        
        Image::createImage(200, 400, 1, VK_SAMPLE_COUNT_1_BIT, swapChainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uiTexture, uiTextureMemory);
        uiTextureView = Image::createImageView(uiTexture, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

#ifdef ENABLE_MULTISAMPLE
        std::array<VkImageView, 2> attachments = {
            colorImageView,
            uiTextureView
        };
#else
        std::array<VkImageView, 1> attachments = {
            uiTextureView
        };
#endif

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = uiRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = 200;
        framebufferInfo.height = 400;
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
        samplerLayoutBinding.descriptorCount = 75; //textures.size();
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

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        if (isUI) {
            samplerBinding.binding = 0;
            bindings.resize(1);
            bindings = {samplerBinding};
        } else {
            bindings.resize(4);
            bindings = {uboLayoutBinding, samplerLayoutBinding, samplerBinding, uiSamplerBinding};
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
    
    VkPipeline createGraphicsPipeline(const char* vertex_path, const char* fragment_path) {
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
#ifdef ENABLE_MULTISAMPLE
        multisampling.rasterizationSamples = msaaSamples;
#else
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
#endif

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_SET;
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

        if (vkCreatePipelineLayout(VK::device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
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

        pipelineInfo.layout = pipelineLayout;

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

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
#ifdef ENABLE_MULTISAMPLE
            std::array<VkImageView, 3> attachments = {
                colorImageView,
                depthImageView,
                swapChainImageViews[i]
            };
#else
            std::array<VkImageView, 2> attachments = {
                swapChainImageViews[i],
                depthImageView,
            };
#endif

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

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
#ifdef ENABLE_MULTISAMPLE
        Image::createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
#else
        Image::createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
#endif
        colorImageView = Image::createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void createDepthResources() {
        VkFormat depthFormat = Utils::findDepthFormat();
#ifdef ENABLE_MULTISAMPLE
        Image::createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
#else
        Image::createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
#endif
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
        std::array<VkDescriptorPoolSize, 4> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 75);

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        poolSizes[3].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

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
    std::deque<std::string> logText;
    int incr = 0;
    void drawUI() {
        ImGui::SetNextWindowSize(ImVec2(200, 400));
        ImGui::SetNextWindowPos(ImVec2(0,0));
        
        if (window_open) {
            ImGui::Begin("Stats", &window_open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
            ImGui::Text("FPS: %i", fps);
            glm::vec3 pos = window.getCamera()->getPosition();
            ImGui::Text("XYZ:\n%f \n%f \n%f", pos.x, pos.y, pos.z);
            ImGui::Text("Press Escape To Exit");
            if (ImGui::Button("Press Me!")) {
                logText.push_back("Hello World! " + std::to_string(incr++));
                while (logText.size() >= 5) {
                    logText.pop_front();
                }
            }
            
            ImGui::BeginChild("Log", ImVec2(200, 50), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

            for (int i = 0; i < logText.size(); i++) {
                ImGui::Text("%s",logText[i].c_str());
            }
            // autoscroll
            ImGui::SetScrollHereY(1.0f);

            ImGui::EndChild();

            ImGui::End();
        }
    }

    void recordUI(VkCommandBuffer commandBuffer) {
        Image::transitionImageLayout(uiTexture, swapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1); 
        VkViewport viewport{};
        VkRect2D scissor{};
        VkRenderPassBeginInfo renderPassInfo{};

        if (uiNeedsUpdate && window_open) {
            // ui renderpass
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = uiRenderPass;
            renderPassInfo.framebuffer = uiFramebuffer;
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {200, 400};
            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
            clearValues[1].depthStencil = {1.0f, 0};

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    viewport.x = 0.0f;
                    viewport.y = 0.0f;
                    viewport.width = (float) 200;
                    viewport.height = (float) 400;
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                scissor.offset = {0, 0};
                scissor.extent = {
                    200,
                    400
                };
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                // draw imgui
                ImGuiIO& io = ImGui::GetIO();
                io.DisplaySize = ImVec2(200,400);

                ImGui_ImplVulkan_NewFrame();
                ImGui::NewFrame();

                drawUI();

                ImGui::Render();
                ImDrawData* drawData = ImGui::GetDrawData();
                ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
            vkCmdEndRenderPass(commandBuffer);
            uiNeedsUpdate = false;
        }
        // transition UI framebuffer back
        Image::transitionImageLayout(uiTexture, swapChainImageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);   
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        
        VkViewport viewport{};
        VkRect2D scissor{};

        // update the gui when fps changes
        if (uiNeedsUpdate && window_open) {
            updateDescriptorSets(currentFrame);
        }
        

        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        // 3d renderpass
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = (float) swapChainExtent.width;
                viewport.height = (float) swapChainExtent.height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            scissor.offset = {0, 0};
            scissor.extent = swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline3D);
        
            uiMesh.isUI = true;

            // temporary: use only the first model descriptorsets
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &(descriptorSets)[currentFrame], 0, nullptr);

            mesh.draw(commandBuffer, pipelineLayout);
            mesh1.draw(commandBuffer, pipelineLayout);

            if (window_open) {
                uiMesh.draw(commandBuffer, pipelineLayout);
            }

        vkCmdEndRenderPass(commandBuffer);
        recordUI(commandBuffer);

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

    void drawFrame() {
        vkWaitForFences(VK::device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(VK::device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(VK::device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
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

        // move camera
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            updateUniformBuffer(i);
        }

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasResized()) {
            window.clearResized();
            window.updateProjectionMatrix(swapChainExtent.width, swapChainExtent.height);

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                updateUniformBuffer(i);
            }
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        
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
            for (size_t j = 0; j < 75; ++j) {
                imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                if (j < textures.size()) {
                    imageInfos[j].imageView = textures[j].textureImageView;
                } else {
                    if (textures.size() <= 0) {
                        //printf("UH, NOT GOOD!, textures don't exist\n");
                        imageInfos[j].imageView = nullptr;
                    } else {
                        imageInfos[j].imageView = textures[0].textureImageView;
                    }
                }
                imageInfos[j].sampler = textureSampler;
            }

            std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

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
    }

    glm::vec2 camRot = glm::vec3(0.0,0.0,0.0);
    glm::vec3 camPos = glm::vec3(0.0,0.0,0.0);

    void handle_input() {
        glm::vec2 camRot = window.getCamera()->getRotation();
        glm::vec3 camPos = window.getCamera()->getPosition();

        //glm::vec2 mouseVec = window.getMouseVector();
        //camRot += (mouseVec * glm::vec2(-0.001f, 0.001f));
        //camRot.y = glm::clamp(camRot.y, glm::radians(-75.0f), glm::radians(75.0f));

        const glm::vec3 forward = window.getCamera()->getForward();

        if (window.isKeyPressed(GLFW_KEY_E)) {
            window_open = true;

            // 1. Position the object in front of the camera
            float distance = 0.8f;

            uiMesh.setPosition((camPos * glm::vec3(-100.0)) + -forward * distance);
            uiMesh.updateModelMatrix();

            glm::vec3 objectToCamera = (camPos * glm::vec3(-100.0)) - uiMesh.getPosition();
            float yaw = std::atan2(objectToCamera.x, objectToCamera.z);
            float pitch = std::atan2(objectToCamera.y, std::sqrt(objectToCamera.x * objectToCamera.x + objectToCamera.z * objectToCamera.z));

            // 4. Set the rotation (assuming your rotation is in radians)
            uiMesh.setRotation({0.0, yaw, glm::radians(90.0f)});
        }

        float speed = 0.0005f;

        if (window.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
            speed *= 2;
        }

        if (window.isKeyPressed(GLFW_KEY_W)) {
            camPos += forward * glm::vec3(speed);
        }
        if (window.isKeyPressed(GLFW_KEY_S)) {
            camPos -= forward * glm::vec3(speed);
        }
        if (window.isKeyPressed(GLFW_KEY_A)) {
            camPos -= glm::cross(forward, glm::vec3(0.0,1.0,0.0)) * glm::vec3(speed);
        }
        if (window.isKeyPressed(GLFW_KEY_D)) {
            camPos += glm::cross(forward, glm::vec3(0.0,1.0,0.0)) * glm::vec3(speed);
        }

        // up
        if (window.isKeyPressed(GLFW_KEY_SPACE)) {
            camPos.y -= speed;
        }
        if (window.isKeyPressed(GLFW_KEY_LEFT_CONTROL)) {
            camPos.y += speed;
        }

        // apply changes
        //if (camera.getRotation() != camRot) {
        //    camera.setRotation(camRot);
        //}
        if (window.getCamera()->getPosition() != camPos) {
            window.getCamera()->setPosition(camPos);
        }
        
        // skip UI stuff if window not open
        if (!window_open) {
            return;
        }

        std::array<glm::vec3, 6> quadVertices;
        std::array<glm::vec2, 6> quadUVs;

        for (int i = 0; i < 6; i++) {
            quadVertices[i] = uiMesh.m_vertices[i].pos;
            quadUVs[i] = uiMesh.m_vertices[i].texCoord;
        }

        // raycast to 
        glm::vec2 result;
        float distance;
        bool hit = Experiment::raycastRectangle(camPos * glm::vec3(-1.0), -forward, quadVertices, quadUVs, uiMesh.m_indices, uiMesh.getModelMatrix().model, window.getCamera()->getMatrix(), window.getProjectionMatrix(), distance, result);
        
        if (ImGui::GetCurrentContext() == nullptr) {
            return;
        }
        ImGuiIO& io = ImGui::GetIO();
        if (distance <= 1.0 && hit) {
            uiNeedsUpdate = true;
            io.AddMousePosEvent(result.x * 200.0f, result.y * 400.0f);
        } else {
            io.AddMousePosEvent(999.0, 999.0);
        }

        io.MouseDrawCursor = true;
        
    }

    void updateUniformBuffer(uint32_t currentImage) {
        UniformBufferObject ubo{};

        handle_input();

        ubo.view = window.getCamera()->getMatrix();
        ubo.proj = window.getProjectionMatrix();
        ubo.proj[1][1] *= -1;
        
        ubo.sampler = textureSampler;
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));    
    }

};

extern const char _binary_a_bin_start[];
extern const char _binary_a_bin_end[];

int main() {
    // test, NOTE: this code loads a model from a zip archive
    const size_t size = _binary_a_bin_end - _binary_a_bin_start;
    Utils::initIOSystem(_binary_a_bin_start, size);

    Application app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
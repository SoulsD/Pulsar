// glfwGetWin32Window
#define GLFW_EXPOSE_NATIVE_WIN32

#define VULKAN_HPP_TYPESAFE_CONVERSION
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>  // glfwGetWin32Window
#include <glm/glm.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <list>
#include <set>
#include <stdexcept>

// TODO:
// - Validation Layers / vk_layer_settings.txt
// - Create own DispatchLoader ? (vkGetInstanceProcAddr)
// - recreate a new swap chain while drawing commands on an image from the old swap chain
//      are still in-flight : pass the previousswap chain to the oldSwapChain field in
//      swapchainCreateInfoKHR then destroy old swapchain
//      http://disq.us/p/1iozw03
// - Custom Allocator

#define REQUIRED_EXTENTIONS \
    {}

#ifndef NDEBUG
#define ADD_VALIDATION_LAYERS
#define VALIDATION_LAYERS                     \
    {                                         \
        "VK_LAYER_LUNARG_standard_validation" \
    }

#define VALIDATION_LAYERS_REQUIRED_EXTENTIONS \
    {                                         \
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME    \
    }

#endif

#define DEVICE_REQUIRED_EXTENSIONS      \
    {                                   \
        VK_KHR_SWAPCHAIN_EXTENSION_NAME \
    }

#define WIN_WIDTH 800
#define WIN_HEIGHT 450

struct Vertex_t {
    glm::vec2 position;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription;

        bindingDescription.setBinding(0)
            .setStride(sizeof(Vertex_t))
            .setInputRate(vk::VertexInputRate::eVertex);
        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;

        attributeDescriptions[0]
            .setBinding(0)
            .setLocation(0)
            .setFormat(vk::Format::eR32G32Sfloat)  // vec2
            .setOffset(offsetof(Vertex_t, position));
        attributeDescriptions[1]
            .setBinding(0)
            .setLocation(1)
            .setFormat(vk::Format::eR32G32B32Sfloat)  // vec3
            .setOffset(offsetof(Vertex_t, color));
        return attributeDescriptions;
    }
};

// interleaving vertex attributes
const std::vector<Vertex_t> vertices = { { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
                                         { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
                                         { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
                                         { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f } } };

const std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

class PulsarApp {
private:
    GLFWwindow* _window;
    vk::DispatchLoaderDynamic _dispatchLoader;
    vk::Instance _instance;
    vk::PhysicalDevice _physicalDevice;
    vk::Device _device;
    vk::Queue _graphicsQueue;
    vk::Queue _transferQueue;
    vk::Queue _presentQueue;

    vk::SurfaceKHR _surface;
    vk::SwapchainKHR _swapChain;
    vk::Format _swapChainFormat;
    vk::Extent2D _swapChainExtent;
    std::vector<vk::Image> _swapChainImages;
    std::vector<vk::ImageView> _swapChainImageViews;

    static constexpr auto vertShaderFile = "build/shaders/default.vert.spv";
    static constexpr auto fragShaderFile = "build/shaders/default.frag.spv";

    vk::RenderPass _renderPass;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _graphicsPipeline;
    std::vector<vk::Framebuffer> _swapChainFramebuffers;

    vk::CommandPool _commandPool;
    vk::CommandPool _transferCommandPool;
    std::vector<vk::CommandBuffer> _commandBuffers;

    std::vector<vk::Semaphore> _imageAvailableSemaphores;
    std::vector<vk::Semaphore> _renderFinishedSemaphores;
    std::vector<vk::Fence> _inFlightFences;

    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
    size_t _currentFrame                         = 0;
    bool _framebufferResized                     = false;

    vk::Buffer _vertexBuffer;
    vk::DeviceMemory _vertexBufferMemory;
    vk::Buffer _indexBuffer;
    vk::DeviceMemory _indexBufferMemory;

#ifdef ADD_VALIDATION_LAYERS
    const std::vector<const char*> requiredValidationLayers = VALIDATION_LAYERS;
#endif

public:
    int run()
    {
        try {
            initWindow();
            initVulkan();
        }
        catch (vk::Error /*vk::SystemError || vk::LogicError*/ const& e) {
            std::cerr << "Error : " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        catch (std::runtime_error const& e) {
            std::cerr << "Error : " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        mainLoop();
        cleanup();
        return EXIT_SUCCESS;
    }

private:
    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        this->_window
            = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(this->_window, this);
        glfwSetFramebufferSizeCallback(
            this->_window,
            [](GLFWwindow* window,
               int width __attribute__((unused)),
               int height __attribute__((unused))) {
                auto app = reinterpret_cast<PulsarApp*>(glfwGetWindowUserPointer(window));
                app->setFramebufferResized(true);
            });
    }

    void setFramebufferResized(bool value) { this->_framebufferResized = value; }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(this->_window)) {
            glfwPollEvents();
            drawFrame();
        }

        this->_device.waitIdle();
    }

    void drawFrame()
    {
        this->_device.waitForFences({ this->_inFlightFences[this->_currentFrame] },
                                    VK_TRUE,
                                    std::numeric_limits<uint64_t>::max());
        this->_device.resetFences({ this->_inFlightFences[this->_currentFrame] });

        uint32_t imageIndex;

        try {
            vk::ResultValue<uint32_t> result = this->_device.acquireNextImageKHR(
                this->_swapChain,
                std::numeric_limits<uint64_t>::max(),
                this->_imageAvailableSemaphores[this->_currentFrame],
                nullptr);
            imageIndex = result.value;
            if (result.result != vk::Result::eSuccess) {
                std::cout << "res : " << result.result << std::endl;
            }
            if (result.result == vk::Result::eErrorOutOfDateKHR) {
                recreateSwapChain();
                return;
            }
        }
        catch (vk::SystemError const& e) {
            std::cout << "error : " << e.what() << std::endl;
            // stop | runtime_error
            return;
        }

        vk::Semaphore waitSemaphores[]
            = { this->_imageAvailableSemaphores[this->_currentFrame] };
        vk::Semaphore signalSemaphores[]
            = { this->_renderFinishedSemaphores[this->_currentFrame] };
        vk::PipelineStageFlags waitStages[]
            = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        vk::SubmitInfo submitInfo;

        submitInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(waitSemaphores)
            .setPWaitDstStageMask(waitStages)
            .setCommandBufferCount(1)
            .setPCommandBuffers(&(this->_commandBuffers[imageIndex]))
            .setSignalSemaphoreCount(1)
            .setPSignalSemaphores(signalSemaphores);

        this->_graphicsQueue.submit({ submitInfo },
                                    this->_inFlightFences[this->_currentFrame]);

        vk::PresentInfoKHR presentInfo;
        vk::SwapchainKHR swapChains[] = { this->_swapChain };

        presentInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(signalSemaphores)
            .setSwapchainCount(1)
            .setPSwapchains(swapChains)
            .setPImageIndices(&imageIndex)
            .setPResults(nullptr);

        try {
            vk::Result result = this->_presentQueue.presentKHR(presentInfo);

            if (result != vk::Result::eSuccess) {
                std::cout << "res : " << result << std::endl;
            }
            if (result == vk::Result::eErrorOutOfDateKHR
                || result == vk::Result::eSuboptimalKHR || this->_framebufferResized) {
                this->_framebufferResized = false;
                recreateSwapChain();
            }
        }
        catch (vk::SystemError const& e) {
            std::cout << "error : " << e.what() << std::endl;
            // stop | runtime_error
        }
        // this->_presentQueue.waitIdle();
        this->_currentFrame = (this->_currentFrame + 1) % PulsarApp::MAX_FRAMES_IN_FLIGHT;
    }

    void initVulkan()
    {
#ifdef ADD_VALIDATION_LAYERS
        if (!checkValidationLayerSupport(VALIDATION_LAYERS)) {
            throw std::runtime_error(
                "not all the required validation layers are available");
        }
#endif
        createInstance();
        _dispatchLoader.init(this->_instance);
#ifdef ADD_VALIDATION_LAYERS
        setupDebugCallback();
#endif
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPools();
        createVertexBuffer();
        createIndexBuffer();
        createCommandBuffers();
        createSyncObjects();
    }

    void handleMinimization()
    {
        int width = 0, height = 0;

        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(this->_window, &width, &height);
            glfwWaitEvents();
        }
    }

    void recreateSwapChain()
    {
        handleMinimization();
        std::cout << "Recreating swapchain" << std::endl;
        this->_device.waitIdle();

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
    }

    void createInstance()
    {
        vk::ApplicationInfo appInfo;  // static const

        appInfo.setPApplicationName("Pulsar")
            .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
            .setPEngineName("Pulsar")
            .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
            .setApiVersion(VK_API_VERSION_1_1);

        std::vector<const char*> requiredExtensions = getRequiredExtensions();

        if (!checkExtensionSupport(requiredExtensions)) {
            throw std::runtime_error("not all the required extensions are available");
        }

        vk::InstanceCreateInfo instInfo;

        instInfo.setPApplicationInfo(&appInfo)
            .setEnabledExtensionCount(requiredExtensions.size())
            .setPpEnabledExtensionNames(requiredExtensions.data())
#ifdef ADD_VALIDATION_LAYERS
            .setEnabledLayerCount(requiredValidationLayers.size())
            .setPpEnabledLayerNames(requiredValidationLayers.data());
#else
            .setEnabledLayerCount(0);
#endif

        this->_instance = vk::createInstance(instInfo);
    }

    void cleanup()
    {
        cleanupSwapChain();

        this->_device.destroyBuffer(this->_vertexBuffer);
        this->_device.freeMemory(this->_vertexBufferMemory);
        this->_device.destroyBuffer(this->_indexBuffer);
        this->_device.freeMemory(this->_indexBufferMemory);

        for (size_t i = 0; i < PulsarApp::MAX_FRAMES_IN_FLIGHT; ++i) {
            this->_device.destroySemaphore(this->_imageAvailableSemaphores[i]);
            this->_device.destroySemaphore(this->_renderFinishedSemaphores[i]);
            this->_device.destroyFence(this->_inFlightFences[i]);
        }
        this->_imageAvailableSemaphores.clear();
        this->_renderFinishedSemaphores.clear();
        this->_inFlightFences.clear();

        this->_device.destroyCommandPool(this->_commandPool);
        this->_device.destroyCommandPool(this->_transferCommandPool);

        this->_device.destroy();
#ifdef ADD_VALIDATION_LAYERS
        this->_instance.destroyDebugReportCallbackEXT(
            debugReportCallback, nullptr, this->_dispatchLoader);
#endif
        this->_instance.destroySurfaceKHR(this->_surface);
        this->_instance.destroy();

        glfwDestroyWindow(this->_window);
        glfwTerminate();
    }

    void cleanupSwapChain()
    {
        for (auto& framebuffer : this->_swapChainFramebuffers) {
            this->_device.destroyFramebuffer(framebuffer);
        }
        this->_swapChainFramebuffers.clear();

        this->_device.freeCommandBuffers(this->_commandPool, this->_commandBuffers);

        this->_device.destroyPipeline(this->_graphicsPipeline);
        this->_device.destroyPipelineLayout(this->_pipelineLayout);
        this->_device.destroyRenderPass(this->_renderPass);

        for (auto& imageView : this->_swapChainImageViews) {
            this->_device.destroyImageView(imageView);
        }
        this->_swapChainImageViews.clear();

        this->_device.destroySwapchainKHR(this->_swapChain);
    }

    std::vector<const char*> getRequiredExtensions()
    {
        std::vector<const char*> requiredExtensions;
        std::uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        requiredExtensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);

        requiredExtensions.insert(requiredExtensions.end(), REQUIRED_EXTENTIONS);

#ifdef ADD_VALIDATION_LAYERS
        requiredExtensions.insert(requiredExtensions.end(),
                                  VALIDATION_LAYERS_REQUIRED_EXTENTIONS);
#endif

        return requiredExtensions;
    }

    // private:
    template <typename T>
    bool checkSupport(std::vector<const char*> const& required,
                      std::vector<T> const& available,
                      std::string (*accessName)(T),
                      bool print            = true,
                      const char* alignment = "\t")
    {
        std::list<std::string> notAvailable(required.begin(), required.end());

        for (T const& t : available) {
            bool isRequired;
            std::string name  = accessName(t);
            auto findRequired = std::find(notAvailable.begin(), notAvailable.end(), name);

            if ((isRequired = (findRequired != notAvailable.end()))) {
                notAvailable.remove(name);
            }
            if (print) {
                std::cout << alignment << (isRequired ? "[v] " : "[ ] ") << name
                          << std::endl;
            }
        }

        if (print) {
            for (auto& t : notAvailable) {
                std::cout << alignment << "[x] " << t << std::endl;
            }
        }

        return notAvailable.empty();
    }

    bool checkExtensionSupport(std::vector<const char*> const& requiredExtensions)
    {
        const std::vector<vk::ExtensionProperties> availableExtensions
            = vk::enumerateInstanceExtensionProperties();
        std::string (*getExtensionName)(vk::ExtensionProperties)
            = [](vk::ExtensionProperties p) -> std::string { return p.extensionName; };

        std::cout << "Extensions :" << std::endl;

        return checkSupport(requiredExtensions, availableExtensions, getExtensionName);
    }

#ifdef ADD_VALIDATION_LAYERS

    vk::DebugReportCallbackEXT debugReportCallback;

    bool checkValidationLayerSupport(std::vector<const char*> const& requiredLayers)
    {
        const std::vector<vk::LayerProperties> availableLayers
            = vk::enumerateInstanceLayerProperties();
        std::string (*getLayerName)(vk::LayerProperties)
            = [](vk::LayerProperties p) -> std::string { return p.layerName; };

        std::cout << "Layers :" << std::endl;

        return checkSupport(requiredLayers, availableLayers, getLayerName);
    }

    // Debug callback for validation layer messages
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugReportFlagsEXT flags,
                  VkDebugReportObjectTypeEXT objType,
                  uint64_t obj __attribute__((unused)),
                  size_t location __attribute__((unused)),
                  int32_t code __attribute__((unused)),
                  const char* layerPrefix,
                  const char* msg,
                  void* userData __attribute__((unused)))
    {
        std::string flagsPrefix = vk::to_string(vk::DebugReportFlagsEXT(flags));

        std::cerr << flagsPrefix.substr(1, flagsPrefix.size() - 2) << ": [" << layerPrefix
                  << "][" << code << "]["
                  << to_string(vk::DebugReportObjectTypeEXT(objType)) << "]: " << msg
                  << std::endl;
        return VK_FALSE;
    }

    void setupDebugCallback()
    {
        GLFWerrorfun error_callback = [](int code, const char* desc) -> void {
            std::cout << "glfw: [" << code << "] " << desc << std::endl;
        };
        glfwSetErrorCallback(error_callback);

        vk::DebugReportCallbackCreateInfoEXT info(
            vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning
                | vk::DebugReportFlagBitsEXT::ePerformanceWarning,
            // | vk::DebugReportFlagBitsEXT::eInformation
            // | vk::DebugReportFlagBitsEXT::eDebug,
            &PulsarApp::debugCallback,
            nullptr);

        this->debugReportCallback = this->_instance.createDebugReportCallbackEXT(
            info, nullptr, this->_dispatchLoader);
    }

#endif

    /* FIXME: */
    void __win32CreateWindowSurface()
    {
        vk::Win32SurfaceCreateInfoKHR surfaceInfo;

        surfaceInfo.setHinstance(GetModuleHandle(nullptr))
            .setHwnd(glfwGetWin32Window(this->_window));

        this->_surface = this->_instance.createWin32SurfaceKHR(
            surfaceInfo, nullptr, this->_dispatchLoader);
    }

    void createSurface() { __win32CreateWindowSurface(); }

    struct QueueFamilyIndices_t {
        int graphicsFamily = -1;
        int transferFamily = -1;
        int presentFamily  = -1;

        bool isComplete()
        {
            return graphicsFamily >= 0 && transferFamily >= 0 && presentFamily >= 0;
        }
    };

    QueueFamilyIndices_t findQueueFamilies(vk::PhysicalDevice const& device)
    {
        QueueFamilyIndices_t indices;
        std::vector<vk::QueueFamilyProperties> queueFamilies
            = device.getQueueFamilyProperties();

        int i = 0;
        // TODO: prefer a physical device that supports drawing and presentation in the
        // same queue for improved performance.
        for (auto const& queueFamily : queueFamilies) {
            if (queueFamily.queueCount > 0) {
                if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                    indices.graphicsFamily = i;
                }
                /* else if  | TODO: add fallback : transfert queue to graphics queue */
                if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) {
                    indices.transferFamily = i;
                }
                if (device.getSurfaceSupportKHR(i, this->_surface)) {
                    indices.presentFamily = i;
                }
            }
            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    bool checkDeviceExtensionSupport(std::vector<const char*> const& requiredExtentions,
                                     vk::PhysicalDevice const& device,
                                     bool print)
    {
        const std::vector<vk::ExtensionProperties> availableExtensions
            = device.enumerateDeviceExtensionProperties();
        std::string (*getExtensionName)(vk::ExtensionProperties)
            = [](vk::ExtensionProperties p) -> std::string { return p.extensionName; };

        return checkSupport(
            requiredExtentions, availableExtensions, getExtensionName, print, "\t\t");
    }

    bool isDeviceSuitable(vk::PhysicalDevice const& device)
    {
        std::vector<const char*> requiredDeviceExtensions(DEVICE_REQUIRED_EXTENSIONS);

        auto properties = device.getProperties();
        // auto features   = device.getFeatures();
        bool isSuitable = false;

        QueueFamilyIndices_t indices = findQueueFamilies(device);

        if (indices.isComplete() && checkDeviceExtensionSupport(requiredDeviceExtensions, device, false)
            /*properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
             && features.geometryShader*/)
        {
            isSuitable = true;
        }

        std::cout << "\t" << (isSuitable ? "[v] " : "[ ] ") << properties.deviceName
                  << " | " << vk::to_string(properties.deviceType) << std::endl;
        // FIXME: call for print only
        checkDeviceExtensionSupport(requiredDeviceExtensions, device, true);

        if (isSuitable) {
            SwapChainSupportDetails_t swapChainSupport = querySwapChainSupport(device);
            bool swapChainAdequate(!swapChainSupport.formats.empty()
                                   && !swapChainSupport.presentModes.empty());
            if (!swapChainAdequate) {
                std::cout << "\t/!\\ SwapChain not adequate" << std::endl;
                isSuitable = false;
            }
        }

        return isSuitable;
    }

    void pickPhysicalDevice()
    {
        std::vector<vk::PhysicalDevice> devices
            = this->_instance.enumeratePhysicalDevices(this->_dispatchLoader);

        if (devices.empty()) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        bool selected = false;

        std::cout << "Physical Devices :" << std::endl;
        // ?TODO: Use an ordered map to automatically sort candidates by increasing score

        for (vk::PhysicalDevice const& device : devices) {
            if (isDeviceSuitable(device) && !selected) {
                this->_physicalDevice = device;

                selected = true;
            }
        }

        if (!(this->_physicalDevice)) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
        std::cout << "Selected : " << this->_physicalDevice.getProperties().deviceName
                  << std::endl;
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices_t indices = findQueueFamilies(this->_physicalDevice);
        std::set<int> uniqueQueueFamilies
            = { indices.graphicsFamily, indices.presentFamily };

        if (indices.graphicsFamily != indices.transferFamily) {
            uniqueQueueFamilies.insert(indices.transferFamily);
        }

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

        vk::PhysicalDeviceFeatures features = this->_physicalDevice.getFeatures();
        vk::DeviceCreateInfo deviceInfo;
        std::vector<const char*> requiredDeviceExtensions(DEVICE_REQUIRED_EXTENSIONS);

        for (auto const& queueFamily : uniqueQueueFamilies) {
            const std::vector<float> queuePriorities = { 1.0f };
            vk::DeviceQueueCreateInfo queueInfo;

            queueInfo.setQueueFamilyIndex(queueFamily)
                .setQueueCount(1)
                .setPQueuePriorities(queuePriorities.data());
            queueCreateInfos.push_back(queueInfo);
        }

        deviceInfo.setPQueueCreateInfos(queueCreateInfos.data())
            .setQueueCreateInfoCount(queueCreateInfos.size())
            .setPEnabledFeatures(&features)
            .setEnabledExtensionCount(requiredDeviceExtensions.size())
            .setPpEnabledExtensionNames(requiredDeviceExtensions.data())
#ifdef ADD_VALIDATION_LAYERS
            .setEnabledLayerCount(requiredValidationLayers.size())
            .setPpEnabledLayerNames(requiredValidationLayers.data());
#else
            .setEnabledLayerCount(0);
#endif
        this->_device = this->_physicalDevice.createDevice(deviceInfo);

        this->_graphicsQueue = this->_device.getQueue(indices.graphicsFamily, 0);
        this->_transferQueue = (indices.graphicsFamily != indices.transferFamily
                                    ? this->_device.getQueue(indices.transferFamily, 0)
                                    : this->_graphicsQueue);
        this->_presentQueue  = this->_device.getQueue(indices.presentFamily, 0);
    }

    struct SwapChainSupportDetails_t {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    SwapChainSupportDetails_t querySwapChainSupport(vk::PhysicalDevice const& device)
    {
        SwapChainSupportDetails_t details;

        details.capabilities = device.getSurfaceCapabilitiesKHR(this->_surface);
        details.formats      = device.getSurfaceFormatsKHR(this->_surface);
        details.presentModes = device.getSurfacePresentModesKHR(this->_surface);

        return details;
    }

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
        std::vector<vk::SurfaceFormatKHR> const& availableFormats)
    {
        if (availableFormats.size() == 1
            && availableFormats[0].format == vk::Format::eUndefined) {
            return vk::SurfaceFormatKHR{ vk::Format::eB8G8R8A8Unorm,
                                         vk::ColorSpaceKHR::eSrgbNonlinear };
        }

        for (auto const& availableFormat : availableFormats) {
            if (availableFormat.format == vk::Format::eB8G8R8A8Unorm
                && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }
        // default
        return availableFormats[0];
    }

    vk::PresentModeKHR chooseSwapPresentMode(
        std::vector<vk::PresentModeKHR> const& availablePresentModes)
    {
        vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

        for (auto const& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            }
            else if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
                bestMode = availablePresentMode;
            }
        }
        return bestMode;
    }

    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            VkExtent2D actualExtent;
            int width, height;

            glfwGetWindowSize(this->_window, &width, &height);

            actualExtent.width  = width > 0 ? static_cast<uint32_t>(width) : WIN_WIDTH;
            actualExtent.height = height > 0 ? static_cast<uint32_t>(height) : WIN_HEIGHT;

            actualExtent.width = std::max(
                capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(
                capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actualExtent.height));
            return actualExtent;
        }
    }

    void createSwapChain()
    {
        SwapChainSupportDetails_t swapChainSupport
            = querySwapChainSupport(this->_physicalDevice);

        vk::SurfaceFormatKHR surfaceFormat
            = chooseSwapSurfaceFormat(swapChainSupport.formats);
        vk::PresentModeKHR presentMode
            = chooseSwapPresentMode(swapChainSupport.presentModes);
        vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        if (swapChainSupport.capabilities.maxImageCount > 0
            && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR swapChainInfo;

        swapChainInfo.setSurface(this->_surface)
            .setMinImageCount(imageCount)
            .setImageFormat(surfaceFormat.format)
            .setImageColorSpace(surfaceFormat.colorSpace)
            .setImageExtent(extent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

        QueueFamilyIndices_t indices = findQueueFamilies(this->_physicalDevice);
        uint32_t queueFamilyIndices[]
            = { (uint32_t) indices.graphicsFamily, (uint32_t) indices.presentFamily };

        if (indices.graphicsFamily != indices.presentFamily) {
            swapChainInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(2)
                .setPQueueFamilyIndices(queueFamilyIndices);
        }
        else {
            swapChainInfo.setImageSharingMode(vk::SharingMode::eExclusive)
                .setQueueFamilyIndexCount(0)       // Optional
                .setPQueueFamilyIndices(nullptr);  // Optional
        }
        swapChainInfo.setPreTransform(swapChainSupport.capabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(presentMode)
            .setClipped(VK_TRUE)
            // .setOldSwapchain()
            ;
        this->_swapChain       = this->_device.createSwapchainKHR(swapChainInfo);
        this->_swapChainImages = this->_device.getSwapchainImagesKHR(this->_swapChain);

        this->_swapChainFormat = surfaceFormat.format;
        this->_swapChainExtent = extent;
    }

    void createImageViews()
    {
        this->_swapChainImageViews.reserve(this->_swapChainImages.size());

        for (vk::Image const& image : this->_swapChainImages) {
            vk::ImageViewCreateInfo imageViewInfo;

            imageViewInfo.setImage(image)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(this->_swapChainFormat)
                .setComponents(vk::ComponentMapping())
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1));

            this->_swapChainImageViews.push_back(
                this->_device.createImageView(imageViewInfo));
        }
    }

    void createRenderPass()
    {
        vk::AttachmentDescription colorAttachment;
        {
            colorAttachment.setFormat(this->_swapChainFormat)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
        }

        vk::AttachmentReference colorAttachmentRef;
        {
            colorAttachmentRef.setAttachment(0).setLayout(
                vk::ImageLayout::eColorAttachmentOptimal);
        }

        vk::SubpassDescription subpass;
        {
            subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                .setColorAttachmentCount(1)
                .setPColorAttachments(&colorAttachmentRef);
        }

        vk::SubpassDependency subpassDependency;
        {
            subpassDependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
                .setDstSubpass(0)
                .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                .setSrcAccessMask(vk::AccessFlags())
                .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead);
        }

        vk::RenderPassCreateInfo renderPassInfo;
        {
            renderPassInfo.setAttachmentCount(1)
                .setPAttachments(&colorAttachment)
                .setSubpassCount(1)
                .setPSubpasses(&subpass)
                .setDependencyCount(1)
                .setPDependencies(&subpassDependency);
        }

        this->_renderPass = this->_device.createRenderPass(renderPassInfo);
    }

    static std::vector<char> readFile(std::string const& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }
        size_t fileSize = file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    vk::ShaderModule createShaderModule(std::vector<char> const& code)
    {
        vk::ShaderModuleCreateInfo shaderModuleInfo;

        shaderModuleInfo
            .setCodeSize(code.size())  // byte size
            .setPCode(reinterpret_cast<uint32_t const*>(code.data()));

        return this->_device.createShaderModule(shaderModuleInfo);
    }

    void createGraphicsPipeline()
    {
        /* Vertex Shader */
        vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
        vk::ShaderModule vertShaderModule;
        {
            auto vertShaderCode = readFile(PulsarApp::vertShaderFile);
            vertShaderModule    = createShaderModule(vertShaderCode);

            vertShaderStageInfo.setStage(vk::ShaderStageFlagBits::eVertex)
                .setModule(vertShaderModule)
                .setPName("main")
                .setPSpecializationInfo(nullptr);
        }

        /* Fragment Shader */
        vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
        vk::ShaderModule fragShaderModule;
        {
            auto fragShaderCode = readFile(PulsarApp::fragShaderFile);
            fragShaderModule    = createShaderModule(fragShaderCode);

            fragShaderStageInfo.setStage(vk::ShaderStageFlagBits::eFragment)
                .setModule(fragShaderModule)
                .setPName("main")
                .setPSpecializationInfo(nullptr);
        }

        vk::PipelineShaderStageCreateInfo shaderStages[]
            = { vertShaderStageInfo, fragShaderStageInfo };

        /* Fixed functions */
        /* -> Vertex input */
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        auto bindingDescription    = Vertex_t::getBindingDescription();
        auto attributeDescriptions = Vertex_t::getAttributeDescriptions();
        {
            vertexInputInfo.setVertexBindingDescriptionCount(1)
                .setPVertexBindingDescriptions(&bindingDescription)
                .setVertexAttributeDescriptionCount(attributeDescriptions.size())
                .setPVertexAttributeDescriptions(attributeDescriptions.data());
        }

        /* -> Input assembly */
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        {
            inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eTriangleList)
                .setPrimitiveRestartEnable(VK_FALSE);
        }

        /* -> Viewports & scissors */
        vk::PipelineViewportStateCreateInfo viewportStateInfo;
        vk::Viewport viewport;
        vk::Rect2D scissor;
        {
            viewport.setX(0.0f)
                .setY(0.0f)
                .setWidth(this->_swapChainExtent.width)
                .setHeight(this->_swapChainExtent.height)
                .setMinDepth(0.0f)
                .setMaxDepth(1.0f);

            scissor.setOffset(vk::Offset2D(0, 0)).setExtent(this->_swapChainExtent);

            viewportStateInfo.setViewportCount(1)
                .setPViewports(&viewport)
                .setScissorCount(1)
                .setPScissors(&scissor);
        }

        /* -> Rasterizer */
        vk::PipelineRasterizationStateCreateInfo rasterizerInfo;
        {
            rasterizerInfo.setDepthClampEnable(VK_FALSE)
                .setRasterizerDiscardEnable(VK_FALSE)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setLineWidth(1.0f)
                .setCullMode(vk::CullModeFlagBits::eBack)
                .setFrontFace(vk::FrontFace::eClockwise)
                .setDepthBiasEnable(VK_FALSE)
                .setDepthBiasConstantFactor(0.0f)
                .setDepthBiasClamp(0.0f)
                .setDepthBiasSlopeFactor(0.0f);
        }

        /* -> Multisampling */
        vk::PipelineMultisampleStateCreateInfo multisamplingInfo;
        {
            multisamplingInfo.setSampleShadingEnable(VK_FALSE)
                .setRasterizationSamples(vk::SampleCountFlagBits::e1)
                .setMinSampleShading(1.0f)
                .setPSampleMask(nullptr)
                .setAlphaToCoverageEnable(VK_FALSE)
                .setAlphaToOneEnable(VK_FALSE);
        }

        /* -> Depth and stencil testing */
        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
        {
            //
        }

        /* -> Color blending */
        vk::PipelineColorBlendStateCreateInfo colorBlendingInfo;
        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        {
            colorBlendAttachment
                .setColorWriteMask(
                    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                    | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
                .setBlendEnable(VK_FALSE)
                .setSrcColorBlendFactor(vk::BlendFactor::eOne)
                .setDstColorBlendFactor(vk::BlendFactor::eZero)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                .setAlphaBlendOp(vk::BlendOp::eAdd);

            colorBlendingInfo.setLogicOpEnable(VK_FALSE)
                .setLogicOp(vk::LogicOp::eCopy)
                .setAttachmentCount(1)
                .setPAttachments(&colorBlendAttachment)
                .setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });
        }

        /* -> Dynamic state */
        vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
        {
            // vk::DynamicState dynamicStates[] = {
            //     vk::DynamicState::eViewport,
            //     vk::DynamicState::eLineWidth
            // };

            // dynamicStateInfo.setDynamicStateCount(2).setPDynamicStates(dynamicStates);
        }

        /* -> Pipeline layout */
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        {
            pipelineLayoutInfo.setSetLayoutCount(0)
                .setPSetLayouts(nullptr)
                .setPushConstantRangeCount(0)
                .setPPushConstantRanges(nullptr);
            this->_pipelineLayout
                = this->_device.createPipelineLayout(pipelineLayoutInfo);
        }
        /* !Fixed functions */

        /* Graphics pipeline creation */
        vk::GraphicsPipelineCreateInfo pipelineInfo;
        {
            pipelineInfo.setStageCount(2)
                .setPStages(shaderStages)
                .setPVertexInputState(&vertexInputInfo)
                .setPInputAssemblyState(&inputAssemblyInfo)
                .setPViewportState(&viewportStateInfo)
                .setPRasterizationState(&rasterizerInfo)
                .setPMultisampleState(&multisamplingInfo)
                .setPDepthStencilState(nullptr)
                .setPColorBlendState(&colorBlendingInfo)
                .setPDynamicState(nullptr)
                .setLayout(this->_pipelineLayout)
                .setRenderPass(this->_renderPass)
                .setSubpass(0)
                // .setBasePipelineHandle(nullptr)
                // .setBasePipelineIndex(0)
                ;
        }

        this->_graphicsPipeline
            = this->_device.createGraphicsPipeline(vk::PipelineCache(), pipelineInfo);

        this->_device.destroyShaderModule(vertShaderModule);
        this->_device.destroyShaderModule(fragShaderModule);
    }

    void createFramebuffers()
    {
        this->_swapChainFramebuffers.reserve(this->_swapChainImageViews.size());

        for (vk::ImageView const& imageView : this->_swapChainImageViews) {
            vk::FramebufferCreateInfo framebufferInfo;
            vk::ImageView attachments[] = { imageView };

            framebufferInfo.setRenderPass(this->_renderPass)
                .setAttachmentCount(1)
                .setPAttachments(attachments)
                .setWidth(this->_swapChainExtent.width)
                .setHeight(this->_swapChainExtent.height)
                .setLayers(1);

            this->_swapChainFramebuffers.push_back(
                this->_device.createFramebuffer(framebufferInfo));
        }
    }

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        vk::PhysicalDeviceMemoryProperties memoryProperties;

        memoryProperties = this->_physicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            if (typeFilter & (1 << i)
                && (memoryProperties.memoryTypes[i].propertyFlags & properties)
                       == properties) {
                return i;
            }
        }
        throw std::runtime_error("no suitable memory type");
    }

    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize bufferSize)
    {
        vk::CommandBuffer transferCommandBuffer;
        {
            vk::CommandBufferAllocateInfo commandBufferInfo;

            commandBufferInfo.setCommandPool(this->_transferCommandPool)
                .setLevel(vk::CommandBufferLevel::ePrimary)
                .setCommandBufferCount(1);

            transferCommandBuffer
                = (this->_device.allocateCommandBuffers(commandBufferInfo)).front();
        }

        vk::CommandBufferBeginInfo commandBufferBeginInfo;

        commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
            .setPInheritanceInfo(nullptr);

        transferCommandBuffer.begin(commandBufferBeginInfo);
        {
            vk::BufferCopy copyRegion;

            copyRegion.setSrcOffset(0).setDstOffset(0).setSize(bufferSize);
            transferCommandBuffer.copyBuffer(srcBuffer, dstBuffer, { copyRegion });
        }
        transferCommandBuffer.end();

        vk::SubmitInfo submitInfo;

        submitInfo.setCommandBufferCount(1).setPCommandBuffers(&transferCommandBuffer);
        this->_transferQueue.submit({ submitInfo }, nullptr);
        this->_transferQueue.waitIdle();

        this->_device.freeCommandBuffers(this->_transferCommandPool,
                                         { transferCommandBuffer });
    }

    void createBuffer(vk::DeviceSize bufferSize,
                      vk::BufferUsageFlags usageFlags,
                      vk::MemoryPropertyFlags memoryPropertyFlags,
                      vk::Buffer& buffer,
                      vk::DeviceMemory& bufferMemory)
    {
        {
            vk::BufferCreateInfo bufferInfo;

            QueueFamilyIndices_t indices = findQueueFamilies(this->_physicalDevice);
            std::vector<uint32_t> uniqueQueueFamilies
                = { static_cast<uint32_t>(indices.graphicsFamily) };

            if (indices.graphicsFamily != indices.transferFamily) {
                uniqueQueueFamilies.push_back(
                    static_cast<uint32_t>(indices.transferFamily));
                bufferInfo.setSharingMode(vk::SharingMode::eConcurrent);
            }
            else {
                bufferInfo.setSharingMode(vk::SharingMode::eExclusive);
            }

            bufferInfo.setSize(bufferSize)
                .setUsage(usageFlags)
                .setQueueFamilyIndexCount(uniqueQueueFamilies.size())
                .setPQueueFamilyIndices(uniqueQueueFamilies.data());

            buffer = this->_device.createBuffer(bufferInfo);
        }

        {
            vk::MemoryAllocateInfo memoryAllocateInfo;
            vk::MemoryRequirements memoryRequirements
                = this->_device.getBufferMemoryRequirements(buffer);
            uint32_t memoryTypeIndex
                = findMemoryType(memoryRequirements.memoryTypeBits, memoryPropertyFlags);

            memoryAllocateInfo.setAllocationSize(memoryRequirements.size)
                .setMemoryTypeIndex(memoryTypeIndex);

            bufferMemory = this->_device.allocateMemory(memoryAllocateInfo);
        }

        this->_device.bindBufferMemory(buffer, bufferMemory, 0);
    }

    void createVertexBuffer()
    {
        vk::DeviceSize bufferSize = sizeof(Vertex_t) * vertices.size();
        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;

        createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible
                | vk::MemoryPropertyFlagBits::eHostCoherent,  // Avoid explicit flushing
            stagingBuffer,
            stagingBufferMemory);

        void* data;

        data = this->_device.mapMemory(
            stagingBufferMemory, 0, bufferSize, vk::MemoryMapFlags());
        {
            std::memcpy(data, vertices.data(), bufferSize);
        }
        this->_device.unmapMemory(stagingBufferMemory);

        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eTransferDst
                         | vk::BufferUsageFlagBits::eVertexBuffer,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     this->_vertexBuffer,
                     this->_vertexBufferMemory);

        copyBuffer(stagingBuffer, this->_vertexBuffer, bufferSize);

        this->_device.destroyBuffer(stagingBuffer);
        this->_device.freeMemory(stagingBufferMemory);
    }

    void createIndexBuffer()
    {
        vk::DeviceSize bufferSize = sizeof(uint32_t) * indices.size();
        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;

        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible
                         | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer,
                     stagingBufferMemory);

        void* data;

        data = this->_device.mapMemory(
            stagingBufferMemory, 0, bufferSize, vk::MemoryMapFlags());
        {
            std::memcpy(data, indices.data(), bufferSize);
        }
        this->_device.unmapMemory(stagingBufferMemory);

        createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            this->_indexBuffer,
            this->_indexBufferMemory);

        copyBuffer(stagingBuffer, this->_indexBuffer, bufferSize);

        this->_device.destroyBuffer(stagingBuffer);
        this->_device.freeMemory(stagingBufferMemory);
    }

    void createCommandPools()
    {
        vk::CommandPoolCreateInfo commandPoolInfo;
        QueueFamilyIndices_t queueFamillyIndices
            = findQueueFamilies(this->_physicalDevice);

        {
            commandPoolInfo.setFlags(vk::CommandPoolCreateFlags())
                .setQueueFamilyIndex(queueFamillyIndices.graphicsFamily);

            this->_commandPool = this->_device.createCommandPool(commandPoolInfo);
        }
        {
            commandPoolInfo.setFlags(vk::CommandPoolCreateFlagBits::eTransient)
                .setQueueFamilyIndex(queueFamillyIndices.transferFamily);

            this->_transferCommandPool = this->_device.createCommandPool(commandPoolInfo);
        }
    }

    void createCommandBuffers()
    {
        vk::CommandBufferAllocateInfo commandBufferInfo;

        commandBufferInfo.setCommandPool(this->_commandPool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(this->_swapChainFramebuffers.size());

        this->_commandBuffers = this->_device.allocateCommandBuffers(commandBufferInfo);

        size_t i = 0;
        for (auto const& commandBuffer : this->_commandBuffers) {
            vk::CommandBufferBeginInfo commandBufferBeginInfo;

            commandBufferBeginInfo
                .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
                .setPInheritanceInfo(nullptr);

            commandBuffer.begin(commandBufferBeginInfo);
            {
                vk::RenderPassBeginInfo renderPassBeginInfo;
                {
                    vk::ClearValue clearColor = vk::ClearColorValue(
                        std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });

                    renderPassBeginInfo.setRenderPass(this->_renderPass)
                        .setFramebuffer(this->_swapChainFramebuffers[i])
                        .setRenderArea(
                            vk::Rect2D(vk::Offset2D(0, 0), this->_swapChainExtent))
                        .setClearValueCount(1)
                        .setPClearValues(&clearColor);
                }

                commandBuffer.beginRenderPass(renderPassBeginInfo,
                                              vk::SubpassContents::eInline);
                {
                    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                               this->_graphicsPipeline);

                    commandBuffer.bindVertexBuffers(0, { this->_vertexBuffer }, { 0 });
                    commandBuffer.bindIndexBuffer(
                        this->_indexBuffer, 0, vk::IndexType::eUint32);

                    commandBuffer.drawIndexed(indices.size(), 1, 0, 0, 0);
                }
                commandBuffer.endRenderPass();
            }
            commandBuffer.end();

            i += 1;
        }
    }

    void createSyncObjects()
    {
        this->_imageAvailableSemaphores.reserve(PulsarApp::MAX_FRAMES_IN_FLIGHT);
        this->_renderFinishedSemaphores.reserve(PulsarApp::MAX_FRAMES_IN_FLIGHT);
        this->_inFlightFences.reserve(PulsarApp::MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < PulsarApp::MAX_FRAMES_IN_FLIGHT; ++i) {
            vk::SemaphoreCreateInfo semaphoreInfo;

            this->_imageAvailableSemaphores[i]
                = this->_device.createSemaphore(semaphoreInfo);
            this->_renderFinishedSemaphores[i]
                = this->_device.createSemaphore(semaphoreInfo);

            vk::FenceCreateInfo fenceInfo;

            fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

            this->_inFlightFences[i] = this->_device.createFence(fenceInfo);
        }
    }
};

int main()
{
    PulsarApp app;

    return app.run();
}

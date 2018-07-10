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

#define REQUIRED_EXTENTIONS \
    {}

#ifndef NDEBUG
// TODO: enable layers
#define ADD_VALIDATION_LAYERS
#define VALIDATION_LAYERS                         \
    { /* "VK_LAYER_LUNARG_standard_validation" */ \
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

class PulsarApp {
private:
    GLFWwindow* _window;
    vk::DispatchLoaderDynamic _dispatchLoader;
    vk::Instance _instance;
    vk::PhysicalDevice _physicalDevice;
    vk::Device _device;
    vk::Queue _graphicsQueue;
    vk::Queue _presentQueue;

    vk::SurfaceKHR _surface;
    vk::SwapchainKHR _swapChain;
    vk::Format _swapChainFormat;
    vk::Extent2D _swapChainExtent;
    std::vector<vk::Image> _swapChainImages;
    std::vector<VkImageView> _swapChainImageViews;

    static constexpr auto vertShaderFile = "build/shaders/default.vert.spv";
    static constexpr auto fragShaderFile = "build/shaders/default.frag.spv";

    vk::RenderPass _renderPass;
    vk::PipelineLayout _pipelineLayout;

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
        catch (vk::Error /*vk::SystemError*/ const& e) {
            std::cerr << "Error : " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        catch (std::runtime_error const& e) {
            std::cerr << "Error : " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        // mainLoop();
        cleanup();
        return EXIT_SUCCESS;
    }

private:
    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        this->_window
            = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(this->_window)) {
            glfwPollEvents();
        }
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
        this->_device.destroyPipelineLayout(this->_pipelineLayout);
        this->_device.destroyRenderPass(this->_renderPass);
        for (auto& imageView : this->_swapChainImageViews) {
            this->_device.destroyImageView(imageView);
        }
        this->_device.destroySwapchainKHR(this->_swapChain);
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

    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugReportFlagsEXT flags __attribute__((unused)),
                  VkDebugReportObjectTypeEXT objType __attribute__((unused)),
                  uint64_t obj __attribute__((unused)),
                  size_t location __attribute__((unused)),
                  int32_t code __attribute__((unused)),
                  const char* layerPrefix __attribute__((unused)),
                  const char* msg,
                  void* userData __attribute__((unused)))
    {
        std::cerr << "validation layer: " << msg << std::endl;
        return VK_FALSE;
    }

    void setupDebugCallback()
    {
        GLFWerrorfun error_callback = [](int code, const char* desc) -> void {
            std::cout << "glfw: [" << code << "] " << desc << std::endl;
        };
        glfwSetErrorCallback(error_callback);

        vk::DebugReportFlagsEXT();
        vk::DebugReportCallbackCreateInfoEXT info(
            vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning,
            &PulsarApp::debugCallback,
            this);

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

    struct QueueFamilyIndices {
        int graphicsFamily = -1;
        int presentFamily  = -1;

        bool isComplete() { return graphicsFamily >= 0 && presentFamily >= 0; }
    };

    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice const& device)
    {
        QueueFamilyIndices indices;
        std::vector<vk::QueueFamilyProperties> queueFamilies
            = device.getQueueFamilyProperties();

        int i = 0;
        // TODO: prefer a physical device that supports drawing and presentation in the
        // same queue for improved performance.
        for (auto const& queueFamily : queueFamilies) {
            if (queueFamily.queueCount > 0
                && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphicsFamily = i;
            }
            if (queueFamily.queueCount > 0
                && device.getSurfaceSupportKHR(i, this->_surface)) {
                indices.presentFamily = i;
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

        QueueFamilyIndices indices = findQueueFamilies(device);

        if (indices.isComplete() && checkDeviceExtensionSupport(requiredDeviceExtensions, device, false)
            /*properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
             && features.geometryShader*/)
        {
            isSuitable = true;
        }

        std::cout << "\t" << (isSuitable ? "[v] " : "[ ] ") << properties.deviceName
                  << " | " << vk::to_string(properties.deviceType) << std::endl;
        // call for print only
        checkDeviceExtensionSupport(requiredDeviceExtensions, device, true);

        if (isSuitable) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
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
        QueueFamilyIndices indices = findQueueFamilies(this->_physicalDevice);
        std::set<int> uniqueQueueFamilies
            = { indices.graphicsFamily, indices.presentFamily };
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
        this->_presentQueue  = this->_device.getQueue(indices.presentFamily, 0);
    }

    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice const& device)
    {
        SwapChainSupportDetails details;

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
            VkExtent2D actualExtent{ WIN_WIDTH, WIN_HEIGHT };

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
        SwapChainSupportDetails swapChainSupport
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

        QueueFamilyIndices indices = findQueueFamilies(this->_physicalDevice);
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
        this->_swapChainImageViews.resize(this->_swapChainImages.size());

        for (vk::Image image : this->_swapChainImages) {
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

        vk::RenderPassCreateInfo renderPassInfo;
        {
            renderPassInfo.setAttachmentCount(1)
                .setPAttachments(&colorAttachment)
                .setSubpassCount(1)
                .setPSubpasses(&subpass);
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
        {
            vertexInputInfo.setVertexBindingDescriptionCount(0)
                .setPVertexBindingDescriptions(nullptr)
                .setVertexAttributeDescriptionCount(0)
                .setPVertexAttributeDescriptions(nullptr);
        }

        /* -> Input assembly */
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        {
            inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eTriangleList)
                .setPrimitiveRestartEnable(VK_FALSE);
        }

        /* -> Viewports & scissors */
        vk::PipelineViewportStateCreateInfo viewportStateInfo;
        {
            vk::Viewport viewport;
            vk::Rect2D scissor;

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
        {
            vk::PipelineColorBlendAttachmentState colorBlendAttachment;

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
            vk::DynamicState dynamicStates[] = {
                // vk::DynamicState::eViewport,
                // vk::DynamicState::eLineWidth
            };

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

        this->_device.destroyShaderModule(vertShaderModule);
        this->_device.destroyShaderModule(fragShaderModule);
    }
};

int main()
{
    PulsarApp app;

    return app.run();
}

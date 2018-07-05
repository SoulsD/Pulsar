// glfwGetWin32Window
#define GLFW_EXPOSE_NATIVE_WIN32

#define VULKAN_HPP_TYPESAFE_CONVERSION
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>  // glfwGetWin32Window
#include <glm/glm.hpp>
#include <iostream>
#include <algorithm>
#include <set>
#include <stdexcept>

// TODO:
// - Validation Layers / vk_layer_settings.txt
// - Create own DispatchLoader ? (vkGetInstanceProcAddr)

#ifndef NDEBUG
#include <list>

#define REQUIRED_EXTENTIONS \
    {}

// TODO: enable layers
#define ADD_VALIDATION_LAYERS
#define VALIDATION_LAYERS \
    {}
// #define VALIDATION_LAYERS
//     {
//         "VK_LAYER_LUNARG_standard_validation"
//     }
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
            std::cout << "glfw: " << code << " | " << desc << std::endl;
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
};

int main()
{
    PulsarApp app;

    return app.run();
}

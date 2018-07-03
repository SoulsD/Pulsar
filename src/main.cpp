#define GLFW_INCLUDE_VULKAN
#define VULKAN_HPP_TYPESAFE_CONVERSION
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <algorithm>
#include <set>
#include <stdexcept>

// TODO:
// - Validation Layers / vk_layer_settings.txt
// - Create own DispatchLoader ? (vkGetInstanceProcAddr)

#ifndef NDEBUG
#include <list>

// TODO: enable layers
#define ADD_VALIDATION_LAYERS
#define VALIDATION_LAYERS \
    {}
// #define VALIDATION_LAYERS
//     {
//         "VK_LAYER_LUNARG_standard_validation"
//     }

#endif

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

#ifdef ADD_VALIDATION_LAYERS
    const std::vector<const char*> requiredValidationLayers = VALIDATION_LAYERS;
#endif


public:
    int run()
    {
        try {
            initWindow();
            initVulkan();
        } catch (vk::Error /*vk::SystemError*/ const& e) {
            std::cerr << "Error : " << e.what() << std::endl;
            return EXIT_FAILURE;
        } catch (std::runtime_error const& e) {
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

#ifdef ADD_VALIDATION_LAYERS
        requiredExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

        return requiredExtensions;
    }

    // private:
    template <typename T>
    bool checkSupport(std::vector<const char*> const& required,
                      std::vector<T> const& available,
                      std::string (*accessName)(T))
    {
        std::list<std::string> notAvailable(required.begin(), required.end());

        for (T const& t : available) {
            bool isRequired;
            std::string name  = accessName(t);
            auto findRequired = std::find(notAvailable.begin(), notAvailable.end(), name);

            if ((isRequired = (findRequired != notAvailable.end()))) {
                notAvailable.remove(name);
            }
            std::cout << "\t " << (isRequired ? "[v] " : "[ ] ") << name << std::endl;
        }

        if (!notAvailable.empty()) {
            for (auto& t : notAvailable) {
                std::cout << "\t [x] " << t << std::endl;
            }
            return false;
        }

        return true;
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
    debugCallback(VkDebugReportFlagsEXT flags,
                  VkDebugReportObjectTypeEXT objType,
                  uint64_t obj,
                  size_t location,
                  int32_t code,
                  const char* layerPrefix,
                  const char* msg,
                  void* userData)
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

    void createSurface()
    {
        VkSurfaceKHR surface;
        int32_t result;

        if ((result = glfwCreateWindowSurface(
                 (VkInstance) this->_instance, this->_window, nullptr, &surface))
            != VK_SUCCESS) {
            std::cout << "result : " << result << std::endl;
            throw std::runtime_error("failed to create window surface!");
        }
        this->_surface = surface;
    }

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

    bool isDeviceSuitable(vk::PhysicalDevice const& device)
    {
        auto properties = device.getProperties();
        // auto features   = device.getFeatures();
        bool isSuitable = false;

        QueueFamilyIndices indices = findQueueFamilies(device);

        if (indices.isComplete()
            /*properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
             && features.geometryShader*/) {
            isSuitable = true;
        }

        std::cout << "\t" << (isSuitable ? "[v] " : "[] ") << properties.deviceName
                  << " | " << vk::to_string(properties.deviceType) << std::endl;

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

        for (auto const& device : devices) {
            if (!selected && isDeviceSuitable(device)) {
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
            .setEnabledExtensionCount(0)
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
};

int main()
{
    PulsarApp app;

    return app.run();
}

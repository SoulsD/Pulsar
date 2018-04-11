#include <string>
#include <iostream>
#include <vulkan/vulkan.h>

int     main(int ac, char **av) {
    VkInstanceCreateInfo    vk_info;
    VkInstance              inst = 0;
    VkResult                res;

    vk_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vk_info.pNext = NULL;
    vk_info.pApplicationInfo = NULL;
    vk_info.enabledLayerCount = 0;
    vk_info.ppEnabledLayerNames = NULL;
    vk_info.enabledExtensionCount = 0;
    vk_info.ppEnabledExtensionNames = NULL;

    res = vkCreateInstance(&vk_info, NULL, &inst);

    if (res != VK_SUCCESS) {
        std::cout << "Error : " << res << std::endl;
        return 1;
    };

    std::cout << "Device created : " << inst << std::endl;

    vkDestroyInstance(inst, NULL);
    return EXIT_SUCCESS;
}

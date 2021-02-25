#include <hdvw/instance.hpp>
using namespace hd;

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <cstdlib>

bool Instance_t::checkValidationLayerSupport(std::vector<const char*>* validationLayers) {
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    for (auto layerName : *validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void Instance_t::populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo) {
    /* createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose */
    createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning 
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
        | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    createInfo.pfnUserCallback = debugCallback;
}

Instance_t::Instance_t(InstanceCreateInfo ci) {
    evl = ci.validationLayers.size() > 0;

    vk::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    if (evl && !checkValidationLayerSupport(&ci.validationLayers)) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    vk::ApplicationInfo appInfo = {};
    appInfo.pApplicationName = ci.applicationName;
    appInfo.applicationVersion = ci.applicationVersion;
    appInfo.pEngineName = ci.engineName;
    appInfo.engineVersion = ci.engineVersion;
    appInfo.apiVersion = ci.apiVersion;

    vk::InstanceCreateInfo createInfo = {};
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = ci.extensions;
    if (evl) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (evl) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(ci.validationLayers.size());
        createInfo.ppEnabledLayerNames = ci.validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    _instance = vk::createInstance(createInfo, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_instance);

    if (evl)
        debugMessenger = _instance.createDebugUtilsMessengerEXT(debugCreateInfo, nullptr);
}

Instance_t::~Instance_t() {
    if (evl)
        _instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr);
    _instance.destroy();
}

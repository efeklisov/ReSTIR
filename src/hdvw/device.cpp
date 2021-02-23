#include <hdvw/device.hpp>
using namespace hd;

#include <string>
#include <set>
#include <iostream>
#include <utility>

bool QueueFamilyIndices::isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value() 
        && computeFamily.has_value() && transferFamily.has_value();
}

QueueFamilyIndices Device_t::findQueueFamilies(vk::PhysicalDevice physicalDevice, Surface surface) {
    QueueFamilyIndices indices = {};

    std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

    int i = -1;
    for (const auto& queueFamily : queueFamilies) {
        i++;
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
            indices.graphicsCount = queueFamily.queueCount;
        }

        if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) {
            indices.transferFamily = i;
            indices.transferCount = queueFamily.queueCount;
        }

        if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) {
            indices.computeFamily = i;
            indices.computeCount = queueFamily.queueCount;
        }

        vk::Bool32 presentSupport;
        if (surface != nullptr)
            presentSupport = physicalDevice.getSurfaceSupportKHR(i, surface->raw());

        if (presentSupport) {
            indices.presentFamily = i;
            indices.presentCount = queueFamily.queueCount;
        }

        if (indices.isComplete()) {
            break;
        }
    }

    return indices;
}

bool Device_t::checkExtensionSupport(vk::PhysicalDevice physicalDevice, const std::vector<const char*>& extensions) {
    std::vector<vk::ExtensionProperties> availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

    for (const auto& ext : availableExtensions) {
        requiredExtensions.erase(static_cast<std::string>(ext.extensionName));
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails Device_t::querySwapChainSupport(vk::PhysicalDevice physicalDevice, Surface surface) {
    SwapChainSupportDetails details = {};

    details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface->raw());
    details.formats = physicalDevice.getSurfaceFormatsKHR(surface->raw());
    details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface->raw());

    return details;
}

Device_t::DeviceSuitableReturn Device_t::deviceSuitable(vk::PhysicalDevice physicalDevice, const DeviceCreateInfo& ci) {
    QueueFamilyIndices indices = {};
    if (ci.findQueueFamilies != nullptr)
        indices = ci.findQueueFamilies(physicalDevice, ci.surface);
    else indices = findQueueFamilies(physicalDevice, ci.surface);

    bool extensionsAvailable = checkExtensionSupport(physicalDevice, ci.extensions);

    bool swapChainAdequate = false;
    if (extensionsAvailable && (ci.surface != nullptr)) {
        _swapChainSupport = querySwapChainSupport(physicalDevice, ci.surface);
        swapChainAdequate = !_swapChainSupport.formats.empty() && !_swapChainSupport.presentModes.empty();
    }

    vk::PhysicalDeviceFeatures supportedFeatures = physicalDevice.getFeatures();

    return {indices.isComplete() && extensionsAvailable && (swapChainAdequate || (ci.surface == nullptr))
        && supportedFeatures.samplerAnisotropy, indices};
}

Device_t::Device_t(const DeviceCreateInfo& ci) {
    _surface = ci.surface->raw();
    std::vector<vk::PhysicalDevice> physDevices = ci.instance->raw().enumeratePhysicalDevices();

    if (physDevices.size() == 0)  {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    bool deviceChosen = false;
    for (const auto& iter: physDevices) {
        DeviceSuitableReturn res = deviceSuitable(iter, ci);
        if (res.suitable) {
            _physicalDevice = iter;
            _indices = res.indices;
            deviceChosen = true;
            break;
        }
    }
    if (!deviceChosen)
        throw std::runtime_error("Suitable physical device not found");

    vk::PhysicalDeviceProperties info = _physicalDevice.getProperties();
    std::cout << info.deviceName << std::endl;

    _rayTracingProperties = _physicalDevice.getProperties2<vk::PhysicalDeviceProperties2, 
                       vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>().get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    _rayTracingFeatures = _physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, 
                       vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>().get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();

    _aStructProperties = _physicalDevice.getProperties2<vk::PhysicalDeviceProperties2, 
                       vk::PhysicalDeviceAccelerationStructurePropertiesKHR>().get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();

    _aStructFeatures = _physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, 
                       vk::PhysicalDeviceAccelerationStructureFeaturesKHR>().get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();

    _physicalMemProps = _physicalDevice.getMemoryProperties();

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<std::pair<uint32_t, uint32_t>> uniqueQueueFamilies = {
        { _indices.graphicsFamily.value(), _indices.graphicsCount.value() }, 
        { _indices.presentFamily.value(), _indices.presentCount.value() },
        { _indices.computeFamily.value(), _indices.computeCount.value() },
        { _indices.transferFamily.value(), _indices.transferCount.value() },
    };

    auto count = std::numeric_limits<uint32_t>::min();
    for (auto& queueFamily: uniqueQueueFamilies) {
        count = std::max(count, queueFamily.second);
    }

    std::vector<float> priorities(count, 1.0f);
    for (auto& queueFamily: uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.queueFamilyIndex = queueFamily.first;
        queueCreateInfo.queueCount = queueFamily.second;
        queueCreateInfo.pQueuePriorities = priorities.data();
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::DeviceCreateInfo createInfo = {};
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pNext = &ci.features;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(ci.extensions.size());
    createInfo.ppEnabledExtensionNames = ci.extensions.data();

    if (ci.validationLayers.size()) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(ci.validationLayers.size());
        createInfo.ppEnabledLayerNames = ci.validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    _device = _physicalDevice.createDevice(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_device);
}

void Device_t::waitIdle() {
    _device.waitIdle();
}

vk::ResultValue<uint32_t> Device_t::acquireNextImage(vk::SwapchainKHR swapChain, vk::Semaphore semaphore) {
    return _device.acquireNextImageKHR(swapChain, UINT64_MAX, semaphore, nullptr);
}

void Device_t::updateSurfaceInfo() {
    _swapChainSupport.capabilities = _physicalDevice.getSurfaceCapabilitiesKHR(_surface);
}

vk::DeviceAddress Device_t::getBufferDeviceAddress(vk::BufferDeviceAddressInfo& bi) {
    return _device.getBufferAddress(bi);
}

Device_t::~Device_t() {
    _device.destroy();
}

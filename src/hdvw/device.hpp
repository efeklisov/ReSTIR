#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/instance.hpp>
#include <hdvw/surface.hpp>

#include <vector>
#include <optional>
#include <memory>

namespace hd {
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> computeFamily;
        std::optional<uint32_t> transferFamily;

        std::optional<uint32_t> graphicsCount;
        std::optional<uint32_t> presentCount;
        std::optional<uint32_t> computeCount;
        std::optional<uint32_t> transferCount;

        bool isComplete();
    };

    struct DeviceCreateInfo {
        Instance instance;
        Surface surface = nullptr;
        QueueFamilyIndices (*findQueueFamilies) (vk::PhysicalDevice, Surface) = nullptr;
        std::vector<const char*> extensions;
        vk::PhysicalDeviceFeatures2 features = {};
        std::vector<const char*> validationLayers;
    };

    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    class Device_t;
    typedef std::shared_ptr<Device_t> Device;

    class Device_t {
        private:
            vk::PhysicalDevice _physicalDevice;
            vk::Device _device;
            vk::SurfaceKHR _surface;
            QueueFamilyIndices _indices;
            SwapChainSupportDetails _swapChainSupport;
            vk::PhysicalDeviceMemoryProperties _physicalMemProps;

            QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice physicalDevice, Surface surface);

            bool checkExtensionSupport(vk::PhysicalDevice physicalDevice, std::vector<const char*>& extensions);

            SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice physicalDevice, Surface surface);

            struct DeviceSuitableReturn {
                bool suitable = true;
                QueueFamilyIndices indices;
            };

            DeviceSuitableReturn deviceSuitable(vk::PhysicalDevice physicalDevice, DeviceCreateInfo& ci);

        public:
            vk::PhysicalDeviceRayTracingPipelinePropertiesKHR _rayTracingProperties{};
            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR _rayTracingFeatures{};
            vk::PhysicalDeviceAccelerationStructurePropertiesKHR _aStructProperties{};
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR _aStructFeatures{};

            static Device conjure(DeviceCreateInfo ci) {
                return std::make_shared<Device_t>(ci);
            }

            Device_t(DeviceCreateInfo ci);

            void waitIdle();

            vk::PhysicalDevice physical();

            vk::ResultValue<uint32_t> acquireNextImage(vk::SwapchainKHR swapChain, vk::Semaphore semaphore);

            vk::Device raw();

            QueueFamilyIndices indices();

            void updateSurfaceInfo();

            SwapChainSupportDetails swapChainSupport();

            vk::DeviceAddress getBufferDeviceAddress(vk::BufferDeviceAddressInfo& bi);

            ~Device_t();
    };
}

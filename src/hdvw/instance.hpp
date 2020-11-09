#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>
#include <iostream>
#include <memory>

namespace hd {
    struct InstanceCreateInfo {
        const char* applicationName;
        uint32_t applicationVersion;
        const char* engineName;
        uint32_t engineVersion;
        uint32_t apiVersion;
        std::vector<const char*> validationLayers;
        std::vector<const char*> extensions;
    };

    class Instance_t;
    typedef std::shared_ptr<Instance_t> Instance;

    class Instance_t {
        private:
            vk::Instance _instance;
            vk::DebugUtilsMessengerEXT debugMessenger;
            bool evl = false;

            bool checkValidationLayerSupport(std::vector<const char*>* validationLayers);

            static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
                std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

                return VK_FALSE;
            }


            void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);

        public:
            static Instance conjure(InstanceCreateInfo ci) {
                return std::make_shared<Instance_t>(ci);
            }

            Instance_t(InstanceCreateInfo ci);

            vk::Instance raw();

            ~Instance_t();
    };
};

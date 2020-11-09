#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>

#include <memory>
#include <utility>

namespace hd {
    struct DescriptorSetCreateInfo {
        vk::DescriptorSet set;
        vk::Device device;
    };

    struct UpdateDescriptorBufferInfo {
        vk::WriteDescriptorSet writeSet = {};
        vk::DescriptorBufferInfo bufferInfo = {};
    };

    struct UpdateDescriptorImageInfo {
        vk::WriteDescriptorSet writeSet = {};
        vk::DescriptorImageInfo imageInfo = {};
    };

    class DescriptorSet_t;
    typedef std::shared_ptr<DescriptorSet_t> DescriptorSet;

    class DescriptorSet_t {
        private:
            vk::DescriptorSet _set;
            vk::Device _device;

        public:
            static DescriptorSet conjure(DescriptorSetCreateInfo ci) {
                return std::make_shared<DescriptorSet_t>(ci);
            }

            DescriptorSet_t(DescriptorSetCreateInfo ci);

            void update(UpdateDescriptorBufferInfo ci);

            void update(UpdateDescriptorImageInfo ci);

            vk::DescriptorSet raw();
    };
}

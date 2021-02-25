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
            static DescriptorSet conjure(const DescriptorSetCreateInfo& ci) {
                return std::make_shared<DescriptorSet_t>(ci);
            }

            DescriptorSet_t(const DescriptorSetCreateInfo& ci);

            vk::WriteDescriptorSet writeInfo(uint32_t binding, vk::DescriptorType type, uint32_t index = 0, uint32_t count = 1);

            inline const auto raw() {
                return _set;
            }
    };

    inline DescriptorSet conjure(const DescriptorSetCreateInfo& ci) {
        return DescriptorSet_t::conjure(ci);
    }
}

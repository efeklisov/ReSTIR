#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>
#include <hdvw/descriptorlayout.hpp>
#include <hdvw/descriptorset.hpp>

#include <memory>
#include <utility>

namespace hd {
    struct DescriptorPoolCreateInfo {
        Device device;
        std::vector<std::pair<DescriptorLayout, uint32_t>> layouts;
    };

    class DescriptorPool_t;
    typedef std::shared_ptr<DescriptorPool_t> DescriptorPool;

    class DescriptorPool_t {
        private:
            vk::DescriptorPool _pool;
            vk::Device _device;

        public:
            static DescriptorPool conjure(const DescriptorPoolCreateInfo& ci) {
                return std::make_shared<DescriptorPool_t>(ci);
            }

            DescriptorPool_t(const DescriptorPoolCreateInfo& ci);

            std::vector<DescriptorSet> allocate(uint32_t count, DescriptorLayout layout);

            inline const auto raw() {
                return _pool;
            }

            ~DescriptorPool_t();
    };

    inline DescriptorPool conjure(const DescriptorPoolCreateInfo& ci) {
        return DescriptorPool_t::conjure(ci);
    }
}

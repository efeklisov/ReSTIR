#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>

#include <memory>
#include <map>

namespace hd {
    struct DescriptorLayoutCreateInfo {
        Device device;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        vk::DescriptorSetLayoutCreateFlags flags = vk::DescriptorSetLayoutCreateFlags{0};
    };

    class DescriptorLayout_t;
    typedef std::shared_ptr<DescriptorLayout_t> DescriptorLayout;

    class DescriptorLayout_t {
        private:
            vk::DescriptorSetLayout _layout;
            vk::Device _device;

            std::map<vk::DescriptorType, uint32_t> _types;

        public:
            static DescriptorLayout conjure(DescriptorLayoutCreateInfo ci) {
                return std::make_shared<DescriptorLayout_t>(ci);
            }

            DescriptorLayout_t(DescriptorLayoutCreateInfo ci);

            inline auto types() {
                return _types;
            }

            inline auto raw() {
                return _layout;
            }

            ~DescriptorLayout_t();
    };

    inline DescriptorLayout conjure(DescriptorLayoutCreateInfo ci) {
        return DescriptorLayout_t::conjure(ci);
    }
}

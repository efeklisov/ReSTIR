#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>

#include <vector>
#include <memory>

namespace hd {
    struct PipelineLayoutCreateInfo {
        Device device;
        std::vector<vk::DescriptorSetLayout> descriptorLayouts = {};
        std::vector<vk::PushConstantRange> pushConstants = {};
    };

    class PipelineLayout_t;
    typedef std::shared_ptr<PipelineLayout_t> PipelineLayout;

    class PipelineLayout_t {
        private:
            vk::Device _device;
            vk::PipelineLayout _pipelineLayout;

        public:
            static PipelineLayout conjure(const PipelineLayoutCreateInfo& ci) {
                return std::make_shared<PipelineLayout_t>(ci);
            }

            PipelineLayout_t(const PipelineLayoutCreateInfo& ci);

            inline const auto raw() {
                return _pipelineLayout;
            }

            ~PipelineLayout_t();
    };

    inline PipelineLayout conjure(const PipelineLayoutCreateInfo& ci) {
        return PipelineLayout_t::conjure(ci);
    }
};

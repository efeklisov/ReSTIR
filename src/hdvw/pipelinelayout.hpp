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
            static PipelineLayout conjure(PipelineLayoutCreateInfo ci) {
                return std::make_shared<PipelineLayout_t>(ci);
            }

            PipelineLayout_t(PipelineLayoutCreateInfo ci);

            vk::PipelineLayout raw();

            ~PipelineLayout_t();
    };
};

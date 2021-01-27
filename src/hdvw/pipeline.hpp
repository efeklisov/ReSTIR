#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>
#include <hdvw/pipelinelayout.hpp>
#include <hdvw/renderpass.hpp>
#include <hdvw/vertex.hpp>

#include <vector>
#include <memory>

namespace hd {
    class Pipeline_t {
        public:
            virtual vk::Pipeline raw() = 0;
    };

    typedef std::shared_ptr<Pipeline_t> Pipeline;

    struct DefaultPipelineCreateInfo {
        PipelineLayout pipelineLayout;
        RenderPass renderPass;
        Device device;
        std::vector<vk::PipelineShaderStageCreateInfo> shaderInfo;
        vk::Extent2D extent;
        vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
        vk::FrontFace frontFace = vk::FrontFace::eClockwise;
        vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
        bool checkDepth = true;
    };

    class DefaultPipeline_t : public Pipeline_t {
        private:
            vk::Pipeline _pipeline;
            vk::Device _device;

        public:
            static Pipeline conjure(DefaultPipelineCreateInfo ci) {
                return std::static_pointer_cast<Pipeline_t>(std::make_shared<DefaultPipeline_t>(ci));
            }

            DefaultPipeline_t(DefaultPipelineCreateInfo ci);

            inline vk::Pipeline raw() {
                return _pipeline;
            }

            ~DefaultPipeline_t();
    };

    inline Pipeline conjure(DefaultPipelineCreateInfo ci) {
        return DefaultPipeline_t::conjure(ci);
    }

    struct RaytraycingPipelineCreateInfo {
        PipelineLayout pipelineLayout;
        Device device;
        std::vector<vk::PipelineShaderStageCreateInfo> shaderInfos;
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;
    };

    class RaytraycingPipeline_t : public Pipeline_t {
        private:
            vk::Pipeline _pipeline;
            vk::Device _device;

            vk::DeviceSize _groupCount;

        public:
            static Pipeline conjure(RaytraycingPipelineCreateInfo ci) {
                return std::static_pointer_cast<Pipeline_t>(std::make_shared<RaytraycingPipeline_t>(ci));
            }

            RaytraycingPipeline_t(RaytraycingPipelineCreateInfo ci);

            inline auto getGroupCount() {
                return _groupCount;
            }

            inline vk::Pipeline raw() {
                return _pipeline;
            }

            ~RaytraycingPipeline_t();
    };

    inline Pipeline conjure(RaytraycingPipelineCreateInfo ci) {
        return RaytraycingPipeline_t::conjure(ci);
    }
}

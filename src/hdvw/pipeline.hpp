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
            virtual const vk::Pipeline raw() = 0;
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
            static Pipeline conjure(const DefaultPipelineCreateInfo& ci) {
                return std::static_pointer_cast<Pipeline_t>(std::make_shared<DefaultPipeline_t>(ci));
            }

            DefaultPipeline_t(const DefaultPipelineCreateInfo& ci);

            inline const vk::Pipeline raw() {
                return _pipeline;
            }

            ~DefaultPipeline_t();
    };

    inline Pipeline conjure(const DefaultPipelineCreateInfo& ci) {
        return DefaultPipeline_t::conjure(ci);
    }

    struct ComputePipelineCreateInfo {
        PipelineLayout pipelineLayout;
        Device device;
        vk::PipelineShaderStageCreateInfo shaderInfo;
    };

    class ComputePipeline_t : public Pipeline_t {
        private:
            vk::Pipeline _pipeline;
            vk::Device _device;

        public:
            static Pipeline conjure(const ComputePipelineCreateInfo& ci) {
                return std::static_pointer_cast<Pipeline_t>(std::make_shared<ComputePipeline_t>(ci));
            }

            ComputePipeline_t(const ComputePipelineCreateInfo& ci);

            inline const vk::Pipeline raw() {
                return _pipeline;
            }

            ~ComputePipeline_t();
    };

    inline Pipeline conjure(const ComputePipelineCreateInfo& ci) {
        return ComputePipeline_t::conjure(ci);
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
            static Pipeline conjure(const RaytraycingPipelineCreateInfo& ci) {
                return std::static_pointer_cast<Pipeline_t>(std::make_shared<RaytraycingPipeline_t>(ci));
            }

            RaytraycingPipeline_t(const RaytraycingPipelineCreateInfo& ci);

            inline const auto getGroupCount() {
                return _groupCount;
            }

            inline const vk::Pipeline raw() {
                return _pipeline;
            }

            ~RaytraycingPipeline_t();
    };

    inline Pipeline conjure(const RaytraycingPipelineCreateInfo& ci) {
        return RaytraycingPipeline_t::conjure(ci);
    }
}

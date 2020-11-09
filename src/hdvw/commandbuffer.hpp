#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/renderpass.hpp>
#include <hdvw/framebuffer.hpp>
#include <hdvw/buffer.hpp>
#include <hdvw/image.hpp>

#include <memory>

namespace hd {
    struct CommandBufferCreateInfo {
        vk::CommandBuffer commandBuffer;
        vk::CommandPool commandPool;
        vk::Device device;
    };

    struct BarrierCreateInfo {
        vk::AccessFlags srcAccess;
        vk::AccessFlags dstAccess;
        vk::PipelineStageFlags srcStage;
        vk::PipelineStageFlags dstStage;
    };

    class CommandBuffer_t;
    typedef std::shared_ptr<CommandBuffer_t> CommandBuffer;

    struct RenderPassBeginInfo {
        RenderPass renderPass;
        Framebuffer framebuffer;
        vk::Offset2D offset{ 0, 0 };
        vk::Extent2D extent;
    };

    struct TransitionImageLayoutInfo {
        Image image;
        vk::ImageLayout layout;
    };

    struct TransitionRawImageLayoutInfo {
        vk::Image image;
        vk::ImageLayout srcLayout;
        vk::ImageLayout dstLayout;
        vk::ImageSubresourceRange range;
    };

    struct CopyBufferToBufferInfo {
        Buffer srcBuffer;
        Buffer dstBuffer;
        vk::DeviceSize srcOffset = 0;
        vk::DeviceSize dstOffset = 0;
        vk::DeviceSize size = 0;
    };

    struct CopyBufferToImageInfo {
        Buffer buffer;
        Image image;
    };

    class CommandBuffer_t {
        private:
            vk::CommandBuffer _buffer;
            vk::CommandPool _cmdpool;
            vk::Device _device;

        public:
            static CommandBuffer conjure(CommandBufferCreateInfo ci) {
                return std::make_shared<CommandBuffer_t>(ci);
            }

            CommandBuffer_t(CommandBufferCreateInfo ci);

            void barrier(BarrierCreateInfo ci);

            void transitionImageLayout(TransitionImageLayoutInfo ci);

            void transitionImageLayout(TransitionRawImageLayoutInfo ci);

            void begin();

            void begin(vk::CommandBufferUsageFlags flags);

            void end();

            void reset(bool release = true);

            void copy(CopyBufferToBufferInfo ci);

            void copy(CopyBufferToImageInfo ci);

            void beginRenderPass(RenderPassBeginInfo bi);

            void endRenderPass(CommandBuffer buffer);

            vk::CommandBuffer raw();

            ~CommandBuffer_t();
    };
}

#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>
#include <hdvw/swapchain.hpp>

#include <memory>

namespace hd {
    class RenderPass_t {
        public:
            virtual vk::RenderPass raw() = 0;

    };

    typedef std::shared_ptr<RenderPass_t> RenderPass;

    struct SwapChainRenderPassCreateInfo {
        SwapChain swapChain;
        Device device;
        vk::ImageLayout colorFinalLayout = vk::ImageLayout::ePresentSrcKHR;
        vk::ImageLayout depthFinalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    };

    class SwapChainRenderPass_t : public RenderPass_t {
        private:
            vk::RenderPass _renderPass;
            vk::Device _device;

        public:
            static RenderPass conjure(SwapChainRenderPassCreateInfo ci) {
                return std::static_pointer_cast<RenderPass_t>(std::make_shared<SwapChainRenderPass_t>(ci));
            }

            SwapChainRenderPass_t(SwapChainRenderPassCreateInfo ci);

            vk::RenderPass raw();

            ~SwapChainRenderPass_t();
    };
}

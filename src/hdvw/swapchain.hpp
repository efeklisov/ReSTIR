#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>
#include <hdvw/allocator.hpp>
#include <hdvw/window.hpp>
#include <hdvw/attachment.hpp>

#include <memory>

namespace hd {
    struct SwapChainCreateInfo {
        Window window;
        Surface surface;
        Allocator allocator;
        Device device;
        vk::PresentModeKHR presentMode;
        std::optional<uint32_t> imageCount;
    };

    class SwapChain_t;
    typedef std::shared_ptr<SwapChain_t> SwapChain;

    class SwapChain_t {
        private:
            vk::Device _device;
            vk::SwapchainKHR _swapChain;

            std::vector<Attachment> _colorImages;
            std::vector<Attachment> _depthImages;

            vk::Extent2D _extent;
            vk::Format _format;
            vk::Format _depthFormat;

            vk::SurfaceFormatKHR chooseSwapSurfaceFormat(SwapChainSupportDetails& support);

            vk::PresentModeKHR chooseSwapPresentMode(SwapChainSupportDetails& support, vk::PresentModeKHR presentMode);

            vk::Extent2D chooseSwapExtent(SwapChainSupportDetails& support, Window window);

        public:
            static SwapChain conjure(SwapChainCreateInfo ci) {
                return std::make_shared<SwapChain_t>(ci);
            }

            SwapChain_t(SwapChainCreateInfo ci);

            vk::Format format();

            vk::Format depthFormat();

            uint32_t length();

            vk::Extent2D extent();

            Attachment colorAttachment(uint32_t index);

            Attachment depthAttachment(uint32_t index);

            vk::SwapchainKHR raw();

            ~SwapChain_t();
    };
}

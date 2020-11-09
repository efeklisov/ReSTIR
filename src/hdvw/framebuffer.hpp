#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/renderpass.hpp>
#include <hdvw/attachment.hpp>

#include <memory>

namespace hd {
    struct FramebufferCreateInfo {
        RenderPass renderPass;
        Device device;
        std::vector<vk::ImageView> attachments;
        vk::Extent2D extent;
    };

    class Framebuffer_t;
    typedef std::shared_ptr<Framebuffer_t> Framebuffer;

    class Framebuffer_t {
        private:
            vk::Framebuffer _framebuffer;
            vk::Device _device;

        public:
            static Framebuffer conjure(FramebufferCreateInfo ci) {
                return std::make_shared<Framebuffer_t>(ci);
            }

            Framebuffer_t(FramebufferCreateInfo ci);

            vk::Framebuffer raw();

            ~Framebuffer_t();
    };
}

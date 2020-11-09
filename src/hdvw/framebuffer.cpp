#include <hdvw/framebuffer.hpp>
using namespace hd;

Framebuffer_t::Framebuffer_t(FramebufferCreateInfo ci) {
    _device = ci.device->raw();

    vk::FramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.renderPass = ci.renderPass->raw();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(ci.attachments.size());
    framebufferInfo.pAttachments = ci.attachments.data();
    framebufferInfo.width = ci.extent.width;
    framebufferInfo.height = ci.extent.height;
    framebufferInfo.layers = 1;

    _framebuffer = _device.createFramebuffer(framebufferInfo);
}

vk::Framebuffer Framebuffer_t::raw() {
    return _framebuffer;
}

Framebuffer_t::~Framebuffer_t() {
    _device.destroy(_framebuffer);
}

#include <hdvw/attachment.hpp>
using namespace hd;

Attachment_t::Attachment_t(AttachmentCreateInfo ci) {
    _device = ci.device->raw();

    if (ci.allocator != nullptr) {
        _image = hd::Image_t::conjure({
                .allocator = ci.allocator,
                .extent = ci.extent,
                .format = ci.format,
                .aspect = ci.aspect,
                .imageUsage = ci.usage,
                .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                });
        _imageHandle = _image->raw();

        _view = ImageView_t::conjure({
                .image = _imageHandle,
                .device = ci.device,
                .format = ci.format,
                .range = _image->range(),
                .type = vk::ImageViewType::e2D,
                });
    } else {
        _imageHandle = ci.image;

        vk::ImageSubresourceRange sr = {};
        sr.aspectMask = ci.aspect;
        sr.baseMipLevel = 0;
        sr.levelCount = 1;
        sr.baseArrayLayer = 0;
        sr.layerCount = 1;

        _view = ImageView_t::conjure({
                .image = _imageHandle,
                .device = ci.device,
                .format = ci.format,
                .range = sr,
                .type = vk::ImageViewType::e2D,
                });
    }
}

vk::Image Attachment_t::raw() {
    return _imageHandle;
}

vk::ImageView Attachment_t::view() {
    return _view->raw();
}

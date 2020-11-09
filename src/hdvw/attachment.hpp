#pragma once

#include <external/vk_mem_alloc.h>

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>
#include <hdvw/allocator.hpp>
#include <hdvw/image.hpp>

#include <memory>

namespace hd {
    struct AttachmentCreateInfo {
        vk::Image image = nullptr;
        Device device;
        hd::Allocator allocator = nullptr;
        vk::Format format;
        vk::ImageUsageFlags usage;
        vk::ImageAspectFlags aspect;
        vk::Extent2D extent;
    };

    class Attachment_t;
    typedef std::shared_ptr<Attachment_t> Attachment;

    class Attachment_t {
        private:
            Image _image;
            vk::Image _imageHandle;
            ImageView _view;

            vk::Device _device;

        public:
            static Attachment conjure(AttachmentCreateInfo ci) {
                return std::make_shared<Attachment_t>(ci);
            }

            Attachment_t(AttachmentCreateInfo ci);

            vk::Image raw();

            vk::ImageView view();
    };
}

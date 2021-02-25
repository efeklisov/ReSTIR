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
            static Attachment conjure(const AttachmentCreateInfo& ci) {
                return std::make_shared<Attachment_t>(ci);
            }

            Attachment_t(const AttachmentCreateInfo& ci);

            inline const auto raw() {
                return _imageHandle;
            }

            inline const auto& imageRef() {
                return _image;
            }

            inline const auto view() {
                return _view->raw();
            }
    };

    inline Attachment conjure(const AttachmentCreateInfo& ci) {
        return Attachment_t::conjure(ci);
    }
}

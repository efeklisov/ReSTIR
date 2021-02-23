#pragma once

#include <vulkan/vulkan.hpp>
#include <external/stb_image.h>

#include <hdvw/commandpool.hpp>
#include <hdvw/queue.hpp>
#include <hdvw/device.hpp>
#include <hdvw/allocator.hpp>
#include <hdvw/image.hpp>
#include <hdvw/buffer.hpp>

#include <memory>

namespace hd {
    struct TextureCreateInfo {
        const char* filename;
        CommandPool commandPool;
        Queue queue;
        Allocator allocator;
        Device device;
    };

    class Texture_t;
    typedef std::shared_ptr<Texture_t> Texture;

    class Texture_t {
        private:
            Image _image;
            ImageView _imageView;
            Sampler _sampler;

        public:
            static Texture conjure(const TextureCreateInfo& ci) {
                return std::make_shared<Texture_t>(ci);
            }

            Texture_t(const TextureCreateInfo& ci);

            vk::DescriptorImageInfo writeInfo(vk::ImageLayout layout) {
                vk::DescriptorImageInfo info{};
                info.imageView = view();
                info.sampler = sampler();
                info.imageLayout = layout;

                return info;
            }

            inline const vk::Sampler sampler() {
                return _sampler->raw();
            }

            inline const vk::ImageView view() {
                return _imageView->raw();
            }

            inline const vk::Image raw() {
                return _image->raw();
            }
    };

    inline Texture conjure(const TextureCreateInfo& ci) {
        return Texture_t::conjure(ci);
    }
}

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
            static Texture conjure(TextureCreateInfo ci) {
                return std::make_shared<Texture_t>(ci);
            }

            Texture_t(TextureCreateInfo ci);

            vk::Sampler sampler();

            vk::ImageView view();

            vk::Image raw();
    };
}

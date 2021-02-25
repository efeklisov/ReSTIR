#include <hdvw/texture.hpp>
using namespace hd;

Texture_t::Texture_t(const TextureCreateInfo& ci) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(ci.filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    Buffer stagingBuffer = Buffer_t::conjure({
            .allocator = ci.allocator,
            .size = imageSize,
            .bufferUsage = vk::BufferUsageFlagBits::eTransferSrc,
            .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
            });

    void* data;
    ci.allocator->map(stagingBuffer->memory(), data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    ci.allocator->unmap(stagingBuffer->memory());

    stbi_image_free(pixels);

    _image = hd::conjure({
            .allocator = ci.allocator,
            .extent = {(uint32_t) texWidth, (uint32_t) texHeight},
            .format = vk::Format::eR8G8B8A8Srgb,
            .imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
            });

    auto buff = ci.commandPool->singleTimeBegin();
    buff->transitionImageLayout({
            .image = _image,
            .dstLayout = vk::ImageLayout::eTransferDstOptimal,
            .range = _image->range(),
            });
    buff->copy({
            .buffer = stagingBuffer,
            .image = _image,
            });
    buff->transitionImageLayout({
            .image = _image,
            .dstLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .range = _image->range(),
            });
    ci.commandPool->singleTimeEnd(buff, ci.queue);

    _imageView = hd::conjure({
            .image = _image->raw(),
            .device = ci.device,
            .format = _image->format(),
            .range = _image->range(),
            .type = vk::ImageViewType::e2D,
            });

    _sampler = hd::conjure({
            .device = ci.device,
            .addressMode = vk::SamplerAddressMode::eRepeat,
            });
}

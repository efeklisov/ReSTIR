#include <hdvw/texture.hpp>
using namespace hd;

Texture_t::Texture_t(TextureCreateInfo ci) {
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

    _image = Image_t::conjure({
            .allocator = ci.allocator,
            .extent = {(uint32_t) texWidth, (uint32_t) texHeight},
            .format = vk::Format::eR8G8B8A8Srgb,
            .imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
            });

    auto buff = ci.commandPool->singleTimeBegin();
    buff->transitionImageLayout({
            .image = _image,
            .layout = vk::ImageLayout::eTransferDstOptimal,
            });
    buff->copy({
            .buffer = stagingBuffer,
            .image = _image,
            });
    buff->transitionImageLayout({
            .image = _image,
            .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
            });
    ci.commandPool->singleTimeEnd(buff, ci.queue);

    _imageView = ImageView_t::conjure({
            .image = _image->raw(),
            .device = ci.device,
            .format = _image->format(),
            .range = _image->range(),
            .type = vk::ImageViewType::e2D,
            });

    _sampler = Sampler_t::conjure({
            .device = ci.device,
            .addressMode = vk::SamplerAddressMode::eRepeat,
            });
}

vk::Sampler Texture_t::sampler() {
    return _sampler->raw();
}

vk::ImageView Texture_t::view() {
    return _imageView->raw();
}

vk::Image Texture_t::raw() {
    return _image->raw();
}

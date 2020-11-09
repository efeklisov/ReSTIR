#include <hdvw/image.hpp>
using namespace hd;

Image_t::Image_t(ImageCreateInfo ci) {
    _allocator = ci.allocator;
    _imageSize = ci.extent;
    _layout = vk::ImageLayout::eUndefined;
    _format = ci.format;
    _range.aspectMask = ci.aspect;
    _range.baseArrayLayer = 0;
    _range.baseMipLevel = 0;
    _range.layerCount = ci.layers;
    _range.levelCount = 1;

    vk::ImageCreateInfo ici = {};
    ici.imageType = vk::ImageType::e2D;
    ici.extent.width = ci.extent.width;
    ici.extent.height = ci.extent.height;
    ici.extent.depth = 1;
    ici.mipLevels = 1;
    ici.arrayLayers = _range.layerCount;
    ici.format = _format;
    ici.tiling = vk::ImageTiling::eOptimal;
    ici.initialLayout = _layout;
    ici.usage = ci.imageUsage;
    ici.samples = vk::SampleCountFlagBits::e1;
    ici.sharingMode = vk::SharingMode::eExclusive;
    ici.flags = ci.flags;

    auto result = _allocator->create(ici, ci.memoryUsage);
    _image = result.image;
    _imageMemory = result.allocation;
}

vk::Extent2D Image_t::extent() {
    return _imageSize;
}

vk::ImageLayout Image_t::layout() {
    return _layout;
}

void Image_t::setLayout(vk::ImageLayout layout) {
    _layout = layout;
}

vk::ImageSubresourceRange Image_t::range() {
    return _range;
}

vk::Format Image_t::format() {
    return _format;
}

VmaAllocation Image_t::memory() {
    return _imageMemory;
}

vk::Image Image_t::raw() {
    return _image;
}

Image_t::~Image_t() {
    _allocator->destroy(_image, _imageMemory);
}

ImageView_t::ImageView_t(ImageViewCreateInfo ci) {
    _device = ci.device->raw();

    vk::ImageViewCreateInfo ivci;
    ivci.image = ci.image;
    ivci.viewType = ci.type;
    ivci.format = ci.format;
    ivci.subresourceRange = ci.range;

    _view = _device.createImageView(ivci);
}

vk::ImageView ImageView_t::raw() {
    return _view;
}

ImageView_t::~ImageView_t() {
    _device.destroy(_view);
}

Sampler_t::Sampler_t(SamplerCreateInfo ci) {
    _device = ci.device->raw();

    vk::SamplerCreateInfo si = {};
    si.magFilter = vk::Filter::eNearest;
    si.minFilter = vk::Filter::eNearest;
    si.addressModeU = ci.addressMode;
    si.addressModeV = ci.addressMode;
    si.addressModeW = ci.addressMode;
    si.anisotropyEnable = VK_TRUE;
    si.maxAnisotropy = 16;
    si.borderColor = vk::BorderColor::eIntOpaqueBlack;
    si.unnormalizedCoordinates = VK_FALSE;
    si.compareEnable = VK_FALSE;
    si.compareOp = vk::CompareOp::eAlways;
    si.mipmapMode = vk::SamplerMipmapMode::eLinear;

    _sampler = _device.createSampler(si);
}

vk::Sampler Sampler_t::raw() {
    return _sampler;
}

Sampler_t::~Sampler_t() {
    _device.destroy(_sampler);
}

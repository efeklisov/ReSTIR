#include <hdvw/image.hpp>
using namespace hd;

Image_t::Image_t(const ImageCreateInfo& ci) {
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
    ici.tiling = ci.tiling;
    ici.initialLayout = _layout;
    ici.usage = ci.imageUsage;
    ici.samples = vk::SampleCountFlagBits::e1;
    ici.sharingMode = vk::SharingMode::eExclusive;
    ici.flags = ci.flags;

    auto result = _allocator->create(ici, ci.memoryUsage);
    _image = result.image;
    _imageMemory = result.allocation;
}

void Image_t::setLayout(vk::ImageLayout layout) {
    _layout = layout;
}

Image_t::~Image_t() {
    _allocator->destroy(_image, _imageMemory);
}

ImageView_t::ImageView_t(const ImageViewCreateInfo& ci) {
    _device = ci.device->raw();

    vk::ImageViewCreateInfo ivci;
    ivci.image = ci.image;
    ivci.viewType = ci.type;
    ivci.format = ci.format;
    ivci.subresourceRange = ci.range;

    _view = _device.createImageView(ivci);
}

ImageView_t::~ImageView_t() {
    _device.destroy(_view);
}

Sampler_t::Sampler_t(const SamplerCreateInfo& ci) {
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

Sampler_t::~Sampler_t() {
    _device.destroy(_sampler);
}

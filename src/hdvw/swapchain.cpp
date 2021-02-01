#include <hdvw/swapchain.hpp>
using namespace hd;

#include <GLFW/glfw3.h>

vk::SurfaceFormatKHR SwapChain_t::chooseSwapSurfaceFormat(SwapChainSupportDetails& support) {
    if ((support.formats.size() == 1) && (support.formats[0].format == vk::Format::eUndefined)) {
        vk::SurfaceFormatKHR ret{};
        ret.format = vk::Format::eB8G8R8A8Unorm; 
        ret.colorSpace = support.formats[0].colorSpace;
        return ret;
    }

    for (const auto& format : support.formats)
        if (format.format == vk::Format::eB8G8R8A8Unorm)
            return format;

    return support.formats[0];
}

vk::PresentModeKHR SwapChain_t::chooseSwapPresentMode(SwapChainSupportDetails& support, vk::PresentModeKHR presentMode) {
    for (const auto& availablePresentMode : support.presentModes) {
        if (availablePresentMode == presentMode) {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D SwapChain_t::chooseSwapExtent(SwapChainSupportDetails& support, Window window) {
    if (support.capabilities.currentExtent.width != UINT32_MAX) {
        return support.capabilities.currentExtent;
    } else {
        uint32_t width, height;
        window->getFramebufferSize(width, height);

        vk::Extent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(support.capabilities.minImageExtent.width,
                std::min(support.capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(support.capabilities.minImageExtent.height,
                std::min(support.capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

SwapChain_t::SwapChain_t(SwapChainCreateInfo ci) {
    _device = ci.device->raw();
    auto indices = ci.device->indices();
    auto support = ci.device->swapChainSupport();

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(support);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(support, ci.presentMode);
    vk::Extent2D extent = chooseSwapExtent(support, ci.window);

    uint32_t imageCount;

    if (ci.imageCount.has_value())
        imageCount = ci.imageCount.value();
    else imageCount = support.capabilities.minImageCount + 1;

    if ((support.capabilities.maxImageCount > 0) && (imageCount > support.capabilities.maxImageCount)) {
        imageCount = support.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo = {};
    createInfo.surface = ci.surface->raw();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    _swapChain = _device.createSwapchainKHR(createInfo);
    _format = surfaceFormat.format;
    _extent = extent;

    for (vk::Format format : { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }) {
        vk::FormatProperties props = ci.device->physical().getFormatProperties(format);

        if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            _depthFormat = format;
            break;
        }
    }

    auto images = _device.getSwapchainImagesKHR(_swapChain);
    _colorImages.reserve(images.size());
    _depthImages.reserve(images.size());

    for (auto& img: images) {
        _colorImages.push_back(hd::conjure({
                    .image = img,
                    .device = ci.device,
                    .format = _format,
                    .usage = vk::ImageUsageFlagBits::eColorAttachment,
                    .aspect = vk::ImageAspectFlagBits::eColor,
                    }));

        _depthImages.push_back(hd::conjure({
                    .device = ci.device,
                    .allocator = ci.allocator,
                    .format = _depthFormat,
                    .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                    .aspect = vk::ImageAspectFlagBits::eDepth,
                    .extent = _extent,
                    }));
    }
}

uint32_t SwapChain_t::length() {
    return _colorImages.size();
}

Attachment SwapChain_t::colorAttachment(uint32_t index) {
    return _colorImages.at(index);
}

Attachment SwapChain_t::depthAttachment(uint32_t index) {
    return _depthImages.at(index);
}

SwapChain_t::~SwapChain_t() {
    _device.destroy(_swapChain);
}

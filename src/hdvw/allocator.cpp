#include <hdvw/allocator.hpp>
using namespace hd;

Allocator_t::Allocator_t(AllocatorCreateInfo ci) {
    VmaVulkanFunctions funcs;
    funcs.vkAllocateMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory;
    funcs.vkBindBufferMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory;
    funcs.vkBindImageMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory;
    funcs.vkCmdCopyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer;
    funcs.vkCreateBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer;
    funcs.vkCreateImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage;
    funcs.vkDestroyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer;
    funcs.vkDestroyImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage;
    funcs.vkFlushMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges;
    funcs.vkFreeMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory;
    funcs.vkGetBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements;
    funcs.vkGetImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements;
    funcs.vkGetPhysicalDeviceMemoryProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties;
    funcs.vkGetPhysicalDeviceProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties;
    funcs.vkInvalidateMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges;
    funcs.vkMapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory;
    funcs.vkUnmapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory;


    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = ci.device->physical();
    allocatorInfo.device = ci.device->raw();
    allocatorInfo.instance = ci.instance->raw();
    allocatorInfo.pVulkanFunctions = &funcs;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    if (vmaCreateAllocator(&allocatorInfo, &_allocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the memory allocator");
    }
}

ReturnImage Allocator_t::create(vk::ImageCreateInfo ici, VmaMemoryUsage flag) {
    VkImage img;
    VmaAllocation alloc;

    VmaAllocationCreateInfo aci = {};
    aci.usage = flag;

    auto c_ici = static_cast<VkImageCreateInfo>(ici);

    if (vmaCreateImage(_allocator, &c_ici, &aci, &img, &alloc, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate an image");
    }

    return { static_cast<vk::Image>(img), alloc };
}

ReturnBuffer Allocator_t::create(vk::BufferCreateInfo ici, VmaMemoryUsage flag) {
    VkBuffer buff;
    VmaAllocation alloc;

    VmaAllocationCreateInfo aci = {};
    aci.usage = flag;

    auto c_ici = static_cast<VkBufferCreateInfo>(ici);

    if (vmaCreateBuffer(_allocator, &c_ici, &aci, &buff, &alloc, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate a buffer");
    }

    return { static_cast<vk::Buffer>(buff), alloc };
}

void Allocator_t::map(VmaAllocation alloc, void* &data) {
    vmaMapMemory(_allocator, alloc, &data);
}

void Allocator_t::unmap(VmaAllocation alloc) {
    vmaUnmapMemory(_allocator, alloc);
}

void Allocator_t::destroy(vk::Image img, VmaAllocation alloc) {
    vmaDestroyImage(_allocator, static_cast<VkImage>(img), alloc);
}

void Allocator_t::destroy(vk::Buffer buff, VmaAllocation alloc) {
    vmaDestroyBuffer(_allocator, static_cast<VkBuffer>(buff), alloc);
}

ReturnMemory Allocator_t::allocate(vk::MemoryRequirements memoryRequirements) {
    auto memReq = static_cast<VkMemoryRequirements>(memoryRequirements);

    VmaAllocationCreateInfo allocCI = {};
    allocCI.memoryTypeBits = memReq.memoryTypeBits;

    ReturnMemory ret;

    vmaAllocateMemory(_allocator, &memReq, &allocCI, &ret.allocation, &ret.allocationInfo);
    return ret;

}

void Allocator_t::free(const VmaAllocation allocation) {
    vmaFreeMemory(_allocator, allocation);
}

VmaAllocator& Allocator_t::raw() {
    return _allocator;
}

Allocator_t::~Allocator_t() {
    vmaDestroyAllocator(_allocator);
}

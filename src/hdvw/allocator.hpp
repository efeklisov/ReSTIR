#pragma once

#include <external/vk_mem_alloc.h>

#include <vulkan/vulkan.hpp>

#include <hdvw/instance.hpp>
#include <hdvw/device.hpp>

#include <memory>

namespace hd {
    struct AllocatorCreateInfo {
        Instance instance;
        Device device;
    };

    struct ReturnImage {
        vk::Image image;
        VmaAllocation allocation;
    };

    struct ReturnBuffer {
        vk::Buffer buffer;
        VmaAllocation allocation;
    };

    struct ReturnMemory {
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
    };

    class Allocator_t;
    typedef std::shared_ptr<Allocator_t> Allocator;

    class Allocator_t {
        private:
            VmaAllocator _allocator;

        public:
            static Allocator conjure(const AllocatorCreateInfo& ci) {
                return std::make_shared<Allocator_t>(ci);
            }

            Allocator_t(const AllocatorCreateInfo& ci);

            ReturnImage create(vk::ImageCreateInfo ici, VmaMemoryUsage flag);

            ReturnBuffer create(vk::BufferCreateInfo ici, VmaMemoryUsage flag);

            void map(VmaAllocation alloc, void* &data);

            void unmap(VmaAllocation alloc);

            void destroy(vk::Image img, VmaAllocation alloc);

            void destroy(vk::Buffer buff, VmaAllocation alloc);

            ReturnMemory allocate(vk::MemoryRequirements memoryRequirements);

            void free(const VmaAllocation);

            inline const auto raw() {
                return _allocator;
            }

            ~Allocator_t();
    };

    inline Allocator conjure(const AllocatorCreateInfo& ci) {
        return Allocator_t::conjure(ci);
    }
}

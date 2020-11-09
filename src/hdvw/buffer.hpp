#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/allocator.hpp>

#include <memory>

namespace hd {
    struct BufferCreateInfo {
        Allocator allocator;
        vk::DeviceSize size;
        vk::BufferUsageFlags bufferUsage;
        VmaMemoryUsage memoryUsage;
    };

    class Buffer_t;
    typedef std::shared_ptr<Buffer_t> Buffer;

    class Buffer_t {
        private:
            vk::Buffer _buffer;
            VmaAllocation _bufferMemory;

            vk::DeviceSize _bufferSize;
            Allocator _allocator;

        public:
            static Buffer conjure(BufferCreateInfo ci) {
                return std::make_shared<Buffer_t>(ci);
            }

            Buffer_t(BufferCreateInfo ci);

            vk::DeviceSize size();

            VmaAllocation memory();

            void* map();

            void unmap();

            vk::Buffer raw();

            ~Buffer_t();
    };
}

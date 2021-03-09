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
            static Buffer conjure(BufferCreateInfo const & ci) {
                return std::make_shared<Buffer_t>(ci);
            }

            Buffer_t(BufferCreateInfo const & ci);

            inline auto size() {
                return _bufferSize;
            }

            inline auto memory() {
                return _bufferMemory;
            }

            void* map();

            void unmap();

            inline auto raw() {
                return _buffer;
            }

            auto writeInfo() {
                vk::DescriptorBufferInfo info{};
                info.buffer = _buffer;
                info.offset = 0;
                info.range = size();

                return info;
            }

            ~Buffer_t();
    };

    inline Buffer conjure(BufferCreateInfo const & ci) {
        return Buffer_t::conjure(ci);
    }
}

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
            static Buffer conjure(const BufferCreateInfo& ci) {
                return std::make_shared<Buffer_t>(ci);
            }

            Buffer_t(const BufferCreateInfo& ci);

            inline const auto size() {
                return _bufferSize;
            }

            inline const auto memory() {
                return _bufferMemory;
            }

            void* map();

            void unmap();

            inline const auto raw() {
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

    inline Buffer conjure(const BufferCreateInfo& ci) {
        return Buffer_t::conjure(ci);
    }
}

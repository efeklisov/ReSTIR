#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/allocator.hpp>
#include <hdvw/commandpool.hpp>
#include <hdvw/queue.hpp>
#include <hdvw/buffer.hpp>
#include <hdvw/device.hpp>

#include <memory>

namespace hd {
    template<class Data>
    struct DataBufferCreateInfo {
        CommandPool commandPool;
        Queue queue;
        Allocator allocator;
        Device device;
        std::vector<Data> data;
        vk::BufferUsageFlags usage;
        VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    };

    template<class Data>
    class DataBuffer_t;

    template<class Data>
    using DataBuffer = std::shared_ptr<DataBuffer_t<Data>>;

    template<class Data>
    class DataBuffer_t {
        private:
            Buffer _buffer;
            uint64_t _entities;
            vk::DeviceOrHostAddressKHR _address;

        public:
            static DataBuffer<Data> conjure(const DataBufferCreateInfo<Data>& ci) {
                return std::make_shared<DataBuffer_t>(ci);
            }

            DataBuffer_t(const DataBufferCreateInfo<Data>& ci) {
                auto device = ci.device->raw();

                _entities = ci.data.size();
                uint64_t _size = sizeof(ci.data[0]) * ci.data.size();

                Buffer stagingBuffer = hd::conjure({
                        .allocator = ci.allocator,
                        .size = _size,
                        .bufferUsage = vk::BufferUsageFlagBits::eTransferSrc,
                        .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
                        });

                void* data = nullptr;
                ci.allocator->map(stagingBuffer->memory(), data);
                memcpy(data, ci.data.data(), (size_t) _size);
                ci.allocator->unmap(stagingBuffer->memory());

                _buffer = hd::conjure({
                        .allocator = ci.allocator,
                        .size = _size,
                        .bufferUsage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress | ci.usage,
                        .memoryUsage = ci.memoryUsage,
                        });

                CommandBuffer cmd = ci.commandPool->singleTimeBegin();
                cmd->copy({
                        .srcBuffer = stagingBuffer,
                        .dstBuffer = _buffer,
                        });
                ci.commandPool->singleTimeEnd(cmd, ci.queue);

                vk::BufferDeviceAddressInfo bufferDeviceAI{};
                bufferDeviceAI.buffer = _buffer->raw();
                
                _address = ci.device->getBufferDeviceAddress(bufferDeviceAI);
            }

            inline const vk::DeviceSize size() {
                return _buffer->size();
            }

            inline const uint64_t count() {
                return _entities;
            }

            inline void* map() {
                return _buffer->map();
            }

            inline void unmap() {
                _buffer->unmap();
            }

            inline const vk::Buffer raw() {
                return _buffer->raw();
            }

            inline const auto writeInfo() {
                return _buffer->writeInfo();
            }

            inline const VmaAllocation memory() {
                return _buffer->memory();
            }

            inline const vk::DeviceOrHostAddressKHR address() {
                return _address;
            }
    };

    template<class T>
    inline DataBuffer<T> conjure(const DataBufferCreateInfo<T>& ci) {
        return DataBuffer_t<T>::conjure(ci);
    }
}

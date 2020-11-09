#include <hdvw/buffer.hpp>
using namespace hd;

Buffer_t::Buffer_t(BufferCreateInfo ci) {
    _allocator = ci.allocator;
    _bufferSize = ci.size;

    vk::BufferCreateInfo bi = {};
    bi.size = _bufferSize;
    bi.usage = ci.bufferUsage;
    bi.sharingMode = vk::SharingMode::eExclusive;

    auto result = _allocator->create(bi, ci.memoryUsage);
    _buffer = result.buffer;
    _bufferMemory = result.allocation;
}

vk::DeviceSize Buffer_t::size() {
    return _bufferSize;
}

VmaAllocation Buffer_t::memory() {
    return _bufferMemory;
}

void* Buffer_t::map() {
    void *data;
    _allocator->map(_bufferMemory, data);
    return data;
}

void Buffer_t::unmap() {
    _allocator->unmap(_bufferMemory);
}

vk::Buffer Buffer_t::raw() {
    return _buffer;
}

Buffer_t::~Buffer_t() {
    _allocator->destroy(_buffer, _bufferMemory);
}

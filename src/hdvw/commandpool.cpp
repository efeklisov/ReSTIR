#include <hdvw/commandpool.hpp>
using namespace hd;

CommandPool_t::CommandPool_t(CommandPoolCreateInfo ci) {
    _device = ci.device->raw();

    vk::CommandPoolCreateInfo poolInfo = {};
    poolInfo.flags = ci.flags;

    switch (ci.family) {
        case PoolFamily::eGraphics:
            poolInfo.queueFamilyIndex = ci.device->indices().graphicsFamily.value();
            break;
        case PoolFamily::eTransfer:
            poolInfo.queueFamilyIndex = ci.device->indices().transferFamily.value();
            break;
        case PoolFamily::eCompute:
            poolInfo.queueFamilyIndex = ci.device->indices().computeFamily.value();
            break;
        case PoolFamily::ePresent:
            poolInfo.queueFamilyIndex = ci.device->indices().presentFamily.value();
            break;
    }

    _commandPool = _device.createCommandPool(poolInfo);
}

std::vector<CommandBuffer> CommandPool_t::allocate(uint32_t count, vk::CommandBufferLevel level) {
    vk::CommandBufferAllocateInfo ai = {};
    ai.level = level;
    ai.commandPool = _commandPool;
    ai.commandBufferCount = count;

    std::vector<CommandBuffer> buffers;
    buffers.reserve(count);

    for (auto& buf: _device.allocateCommandBuffers(ai)) {
        buffers.push_back(CommandBuffer_t::conjure({
                    .commandBuffer = buf,
                    .commandPool = _commandPool,
                    .device = _device,
                    }));
    }

    return buffers;
}

CommandBuffer CommandPool_t::singleTimeBegin() {
    auto buffer = allocate(1, vk::CommandBufferLevel::ePrimary).at(0);

    buffer->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    return buffer;
}

void CommandPool_t::singleTimeEnd(CommandBuffer buffer, Queue queue) {
    buffer->end();

    auto raw = buffer->raw();
    vk::SubmitInfo si = {};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &raw;
    queue->submit(si, nullptr);
    queue->waitIdle();
}

vk::CommandPool CommandPool_t::raw() {
    return _commandPool;
}

CommandPool_t::~CommandPool_t() {
    _device.destroy(_commandPool);
}

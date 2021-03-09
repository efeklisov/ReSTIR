#include <sbt.hpp>

namespace hd {
    SBT_t::SBT_t(SBTCreateInfo const & ci) {
        auto device = ci.device->raw();

        const uint32_t handleSize = ci.device->_rayTracingProperties.shaderGroupHandleSize;
        const uint32_t handleAlignment = ci.device->_rayTracingProperties.shaderGroupHandleAlignment;
        const uint32_t groupCount = dynamic_cast<RaytraycingPipeline_t*>(ci.pipeline.get())->getGroupCount();
        const uint32_t sbtSize = handleSize * groupCount;

        std::vector<uint8_t> shaderHandleStorage(sbtSize);
        if (device.getRayTracingShaderGroupHandlesKHR(ci.pipeline->raw(), 0, groupCount, sbtSize, shaderHandleStorage.data()) != vk::Result::eSuccess)
            throw std::runtime_error("Couldn't retrieve shader groups' handles");

        _raygen.buffer = hd::conjure({
                .allocator = ci.allocator,
                .size = handleSize * ci.raygenCount,
                .bufferUsage = vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                });

        vk::BufferDeviceAddressInfo raygenDeviceAI{};
        raygenDeviceAI.buffer = _raygen.buffer->raw();
        
        _raygen.address = ci.device->getBufferDeviceAddress(raygenDeviceAI);
        _raygen.count = ci.raygenCount;

        _raygen.region.deviceAddress = _raygen.address;
        _raygen.region.stride = ci.device->_rayTracingProperties.shaderGroupHandleSize;
        _raygen.region.size = ci.device->_rayTracingProperties.shaderGroupHandleSize * _raygen.count;

        _miss.buffer = hd::conjure({
                .allocator = ci.allocator,
                .size = handleSize * ci.missCount,
                .bufferUsage = vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                });

        vk::BufferDeviceAddressInfo missDeviceAI{};
        missDeviceAI.buffer = _miss.buffer->raw();
        
        _miss.address = ci.device->getBufferDeviceAddress(missDeviceAI);
        _miss.count = ci.missCount;

        _miss.region.deviceAddress = _miss.address;
        _miss.region.stride = ci.device->_rayTracingProperties.shaderGroupHandleSize;
        _miss.region.size = ci.device->_rayTracingProperties.shaderGroupHandleSize * _miss.count;

        _hit.buffer = hd::conjure({
                .allocator = ci.allocator,
                .size = handleSize * ci.hitCount,
                .bufferUsage = vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                });

        vk::BufferDeviceAddressInfo hitDeviceAI{};
        hitDeviceAI.buffer = _hit.buffer->raw();
        
        _hit.address = ci.device->getBufferDeviceAddress(hitDeviceAI);
        _hit.count = ci.hitCount;

        _hit.region.deviceAddress = _hit.address;
        _hit.region.stride = ci.device->_rayTracingProperties.shaderGroupHandleSize;
        _hit.region.size = ci.device->_rayTracingProperties.shaderGroupHandleSize * _hit.count;

        auto raygenData = static_cast<uint8_t*>(_raygen.buffer->map());
        memcpy(raygenData, shaderHandleStorage.data(), handleSize * ci.raygenCount);
        _raygen.buffer->unmap();

        auto missData = static_cast<uint8_t*>(_miss.buffer->map());
        memcpy(missData, shaderHandleStorage.data() + handleAlignment * ci.raygenCount, handleSize * ci.missCount);
        _miss.buffer->unmap();

        auto hitData = static_cast<uint8_t*>(_hit.buffer->map());
        memcpy(hitData, shaderHandleStorage.data() + handleAlignment * (ci.raygenCount + ci.missCount), handleSize * ci.hitCount);
        _hit.buffer->unmap();
    }
}

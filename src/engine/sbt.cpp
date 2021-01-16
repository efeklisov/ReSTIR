#include <sbt.hpp>

namespace hd {
    SBT_t::SBT_t(SBTCreateInfo ci) {
        auto device = ci.device->raw();

        const uint32_t handleSize = ci.device->_rayTracingProperties.shaderGroupHandleSize;
        const uint32_t handleAlignment = ci.device->_rayTracingProperties.shaderGroupHandleAlignment;
        const uint32_t groupCount = dynamic_cast<RaytraycingPipeline_t*>(ci.pipeline.get())->getGroupCount();
        const uint32_t sbtSize = handleSize * groupCount;

        std::vector<uint8_t> shaderHandleStorage(sbtSize);
        if (device.getRayTracingShaderGroupHandlesKHR(ci.pipeline->raw(), 0, groupCount, sbtSize, shaderHandleStorage.data()) != vk::Result::eSuccess)
            throw std::runtime_error("Couldn't retrieve shader groups' handles");

        _raygen.buffer = Buffer_t::conjure({
                .allocator = ci.allocator,
                .size = handleSize * ci.raygenCount,
                .bufferUsage = vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                });

        vk::BufferDeviceAddressInfo raygenDeviceAI{};
        raygenDeviceAI.buffer = _raygen.buffer->raw();
        
        _raygen.address = ci.device->getBufferDeviceAddress(raygenDeviceAI);
        _raygen.count = ci.raygenCount;

        _miss.buffer = Buffer_t::conjure({
                .allocator = ci.allocator,
                .size = handleSize * ci.missCount,
                .bufferUsage = vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                });

        vk::BufferDeviceAddressInfo missDeviceAI{};
        missDeviceAI.buffer = _miss.buffer->raw();
        
        _miss.address = ci.device->getBufferDeviceAddress(missDeviceAI);
        _miss.count = ci.missCount;

        _hit.buffer = Buffer_t::conjure({
                .allocator = ci.allocator,
                .size = handleSize * ci.hitCount,
                .bufferUsage = vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                });

        vk::BufferDeviceAddressInfo hitDeviceAI{};
        hitDeviceAI.buffer = _hit.buffer->raw();
        
        _hit.address = ci.device->getBufferDeviceAddress(hitDeviceAI);
        _hit.count = ci.hitCount;

        auto raygenData = static_cast<uint8_t*>(_raygen.buffer->map());
        memcpy(raygenData, shaderHandleStorage.data(), handleSize * ci.raygenCount);
        _raygen.buffer->unmap();

        auto missData = static_cast<uint8_t*>(_miss.buffer->map());
        memcpy(missData, shaderHandleStorage.data() + handleAlignment * ci.raygenCount, handleSize * ci.missCount);
        _miss.buffer->unmap();

        auto hitData = static_cast<uint8_t*>(_hit.buffer->map());
        memcpy(hitData, shaderHandleStorage.data() + handleAlignment * (ci.raygenCount + ci.missCount), handleSize * ci.hitCount);
        _hit.buffer->unmap();

        /* auto rayProps = ci.device->_rayTracingProperties; */
        
        /* _shaderGroups = dynamic_cast<RaytraycingPipeline_t*>(ci.pipeline.get())->getGroupCount(); */
        /* const uint32_t sbtSize = rayProps.shaderGroupBaseAlignment * _shaderGroups; */

        /* _buffer = Buffer_t::conjure({ */
        /*         .allocator = ci.allocator, */
        /*         .size = sbtSize, */
        /*         .bufferUsage = vk::BufferUsageFlagBits::eShaderBindingTableKHR, */
        /*         .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU, */
        /*         }); */

        /* std::vector<uint8_t> shaderHandleStorage(sbtSize); */

        /* if (device.getRayTracingShaderGroupHandlesKHR( */
        /*         ci.pipeline->raw(), 0, _shaderGroups, sbtSize, shaderHandleStorage.data()) != vk::Result::eSuccess) */
        /*     throw std::runtime_error("Couldn't retrieve shader groups' handles"); */

/*         auto data = static_cast<uint8_t*>(_buffer->map()); */
/*         for (uint32_t i = 0; i < _shaderGroups; i++) { */
/*             memcpy(data, shaderHandleStorage.data() + i * rayProps.shaderGroupHandleSize, rayProps.shaderGroupHandleSize); */
/*             data += rayProps.shaderGroupBaseAlignment; */
/*         } */
/*         _buffer->unmap(); */
    }

    SBTEntry SBT_t::raygen() {
        return _raygen;
    }

    SBTEntry SBT_t::miss() {
        return _miss;
    }

    SBTEntry SBT_t::hit() {
        return _hit;
    }
}

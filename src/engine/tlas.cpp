#include <tlas.hpp>

namespace hd {
    TLAS_t::TLAS_t(TLASCreateInfo ci) {
         _device = ci.device->raw();
         _allocator = ci.allocator;

        // TLAS
        vk::AccelerationStructureGeometryInstancesDataKHR instances{};
        instances.arrayOfPointers = false;
        instances.data = ci.instbuffer->address().deviceAddress;

        vk::AccelerationStructureGeometryKHR aStructGeometry{};
        aStructGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
        aStructGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
        aStructGeometry.geometry.instances = instances;

        vk::AccelerationStructureBuildGeometryInfoKHR aStructGeometryBI{};
        aStructGeometryBI.type = vk::AccelerationStructureTypeKHR::eTopLevel;
        aStructGeometryBI.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        aStructGeometryBI.setGeometries(aStructGeometry);

        vk::AccelerationStructureBuildSizesInfoKHR aStructSizesBI = _device.getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice, aStructGeometryBI, ci.instbuffer->count());

        vk::BufferCreateInfo bufferCI{};
        bufferCI.size = aStructSizesBI.accelerationStructureSize;
        bufferCI.usage = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;

        memory = _allocator->create(bufferCI, VMA_MEMORY_USAGE_GPU_ONLY);

        vk::AccelerationStructureCreateInfoKHR aStructCI{};
        aStructCI.buffer = memory.buffer;
        aStructCI.size = aStructSizesBI.accelerationStructureSize;
        aStructCI.type = vk::AccelerationStructureTypeKHR::eTopLevel;

        _aStruct = _device.createAccelerationStructureKHR(aStructCI, nullptr);

        // Scratch
        vk::BufferCreateInfo scratchCI{};
        scratchCI.size = aStructSizesBI.buildScratchSize;
        scratchCI.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;

        auto scratch = _allocator->create(scratchCI, VMA_MEMORY_USAGE_CPU_TO_GPU);

        vk::BufferDeviceAddressInfo scratchAI{};
        scratchAI.buffer = scratch.buffer;

        auto scratchAdress = _device.getBufferAddress(scratchAI);

        vk::AccelerationStructureBuildGeometryInfoKHR aStructGeometryBI2{};
        aStructGeometryBI2.type = vk::AccelerationStructureTypeKHR::eTopLevel;
        aStructGeometryBI2.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        aStructGeometryBI2.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
        aStructGeometryBI2.dstAccelerationStructure = _aStruct;
        aStructGeometryBI2.setGeometries(aStructGeometry);
        aStructGeometryBI2.scratchData.deviceAddress = scratchAdress;

        vk::AccelerationStructureBuildRangeInfoKHR aStructRangeBI{};
        aStructRangeBI.primitiveCount = ci.instbuffer->count();
        aStructRangeBI.primitiveOffset = 0;
        aStructRangeBI.firstVertex = 0;
        aStructRangeBI.transformOffset = 0;

        if (ci.device->_aStructFeatures.accelerationStructureHostCommands) {
            // Implementation supports building acceleration structure building on host
            if (_device.buildAccelerationStructuresKHR(nullptr, aStructGeometryBI2, &aStructRangeBI) != vk::Result::eSuccess)
                throw std::runtime_error("Unable to create TLAS");
        } else {
            // Acceleration structure needs to be build on the device
            CommandBuffer cmd = ci.commandPool->singleTimeBegin();
            cmd->raw().buildAccelerationStructuresKHR(aStructGeometryBI2, &aStructRangeBI);
            ci.commandPool->singleTimeEnd(cmd, ci.queue);
        }

        vk::AccelerationStructureDeviceAddressInfoKHR aDeviceAddressInfo{};
        aDeviceAddressInfo.accelerationStructure = _aStruct;

        _aAddress = _device.getAccelerationStructureAddressKHR(aDeviceAddressInfo);

        // Cleanup
        _allocator->destroy(scratch.buffer, scratch.allocation);
    }

    TLAS_t::~TLAS_t() {
        _device.destroyAccelerationStructureKHR(_aStruct);
        _allocator->destroy(memory.buffer, memory.allocation);
    }
}

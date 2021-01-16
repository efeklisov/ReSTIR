#include <blas.hpp>

namespace hd {
    BLAS_t::BLAS_t(BLASCreateInfo ci) {
         _device = ci.device->raw();
         _allocator = ci.allocator;

        // BLAS
        vk::AccelerationStructureGeometryTrianglesDataKHR triangles{};
        triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
        triangles.vertexData = ci.vertices->address().deviceAddress;
        triangles.maxVertex = ci.vertices->size();
        triangles.vertexStride = sizeof(Vertex);
        triangles.indexType = vk::IndexType::eUint32;
        triangles.indexData = ci.indices->address().deviceAddress;
        triangles.vertexStride = sizeof(Vertex);

        vk::AccelerationStructureGeometryKHR aStructGeometry{};
        aStructGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
        aStructGeometry.geometryType = vk::GeometryTypeKHR::eTriangles;
        aStructGeometry.geometry.triangles = triangles;

        vk::AccelerationStructureBuildGeometryInfoKHR aStructGeometryBI{};
        aStructGeometryBI.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
        aStructGeometryBI.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        aStructGeometryBI.setGeometries(aStructGeometry);

        vk::AccelerationStructureBuildSizesInfoKHR aStructSizesBI = _device.getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice, aStructGeometryBI, ci.indices->count() / 3);

        vk::BufferCreateInfo bufferCI{};
        bufferCI.size = aStructSizesBI.accelerationStructureSize;
        bufferCI.usage = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;

        memory = _allocator->create(bufferCI, VMA_MEMORY_USAGE_GPU_ONLY);

        vk::AccelerationStructureCreateInfoKHR aStructCI{};
        aStructCI.buffer = memory.buffer;
        aStructCI.size = aStructSizesBI.accelerationStructureSize;
        aStructCI.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

        _aStruct = _device.createAccelerationStructureKHR(aStructCI, nullptr);

        // Scratch
        vk::BufferCreateInfo scratchCI{};
        scratchCI.size = aStructSizesBI.buildScratchSize;
        scratchCI.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;

        auto scratch = _allocator->create(scratchCI, VMA_MEMORY_USAGE_CPU_TO_GPU);

        vk::BufferDeviceAddressInfo scratchAI{};
        scratchAI.buffer = scratch.buffer;

        auto scratchAdress = _device.getBufferAddress(scratchAI);

        // Build
        vk::AccelerationStructureBuildGeometryInfoKHR aStructGeometryBI2{};
        aStructGeometryBI2.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
        aStructGeometryBI2.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        aStructGeometryBI2.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
        aStructGeometryBI2.dstAccelerationStructure = _aStruct;
        aStructGeometryBI2.setGeometries(aStructGeometry);
        aStructGeometryBI2.scratchData.deviceAddress = scratchAdress;

        vk::AccelerationStructureBuildRangeInfoKHR aRangeBI{};
        aRangeBI.primitiveCount = ci.indices->count() / 3;
        aRangeBI.primitiveOffset = 0x0;
        aRangeBI.firstVertex = 0;
        aRangeBI.transformOffset = 0x0;

        if (ci.device->_aStructFeatures.accelerationStructureHostCommands) {
            // Implementation supports building acceleration structure building on host
            if (_device.buildAccelerationStructuresKHR(nullptr, aStructGeometryBI2, &aRangeBI) != vk::Result::eSuccess)
                throw std::runtime_error("Unable to create BLAS");
        } else {
            // Acceleration structure needs to be build on the device
            auto cmd = ci.commandPool->singleTimeBegin();
            cmd->raw().buildAccelerationStructuresKHR(aStructGeometryBI2, &aRangeBI);
            ci.commandPool->singleTimeEnd(cmd, ci.queue);
        }

        vk::AccelerationStructureDeviceAddressInfoKHR aDeviceAddressInfo{};
        aDeviceAddressInfo.accelerationStructure = _aStruct;

        _aAddress = _device.getAccelerationStructureAddressKHR(aDeviceAddressInfo);

        // Cleanup
        _allocator->destroy(scratch.buffer, scratch.allocation);
    }

    vk::AccelerationStructureKHR BLAS_t::raw() {
        return _aStruct;
    }

    vk::DeviceAddress BLAS_t::address() {
        return _aAddress;
    }

    BLAS_t::~BLAS_t() {
        _device.destroyAccelerationStructureKHR(_aStruct);
        _allocator->destroy(memory.buffer, memory.allocation);
    }
}

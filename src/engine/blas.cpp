#include <blas.hpp>

namespace hd {
    BLAS_t::BLAS_t(BLASCreateInfo ci) {
         _device = ci.device->raw();
         _allocator = ci.allocator;

        // BLAS
        vk::AccelerationStructureCreateGeometryTypeInfoKHR aGeometryCI{};
        aGeometryCI.geometryType = vk::GeometryTypeKHR::eTriangles;
        aGeometryCI.maxPrimitiveCount = ci.indices->count() / 3;
        aGeometryCI.indexType = vk::IndexType::eUint32;
        aGeometryCI.maxVertexCount = static_cast<uint32_t>(ci.vertices->size());
        aGeometryCI.vertexFormat = vk::Format::eR32G32B32Sfloat;
        aGeometryCI.allowsTransforms = false;

        vk::AccelerationStructureCreateInfoKHR aCI{};
        aCI.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
        aCI.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        aCI.setGeometryInfos(aGeometryCI);
        
        _aStruct = _device.createAccelerationStructureKHR(aCI, nullptr);

        // Object Memory
        vk::AccelerationStructureMemoryRequirementsInfoKHR aMemreq{};
        aMemreq.type = vk::AccelerationStructureMemoryRequirementsTypeKHR::eObject;
        aMemreq.buildType = vk::AccelerationStructureBuildTypeKHR::eDevice;
        aMemreq.accelerationStructure = _aStruct;

        auto memReq = _device.getAccelerationStructureMemoryRequirementsKHR(aMemreq).memoryRequirements;
        auto mem = _allocator->allocate(memReq);
        _objMem = mem.allocation;
        _objMemInfo = mem.allocationInfo;

        vk::BindAccelerationStructureMemoryInfoKHR aMemBI{};
        aMemBI.accelerationStructure = _aStruct;
        aMemBI.memory = _objMemInfo.deviceMemory;
        aMemBI.memoryOffset = _objMemInfo.offset;
        _device.bindAccelerationStructureMemoryKHR(aMemBI);

        // Geometry
        vk::AccelerationStructureGeometryTrianglesDataKHR triangles;
        triangles.vertexFormat = aGeometryCI.vertexFormat;
        triangles.vertexData.deviceAddress = ci.vertices->address().deviceAddress;
        triangles.vertexStride = sizeof(Vertex);
        triangles.indexType = aGeometryCI.indexType;
        triangles.indexData.deviceAddress = ci.indices->address().deviceAddress;
        triangles.transformData = {};

        vk::AccelerationStructureGeometryKHR aGeometry{};
        aGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
        aGeometry.geometryType = aGeometryCI.geometryType;
        aGeometry.geometry.triangles = triangles;

        const std::vector<vk::AccelerationStructureGeometryKHR> aGeometries = { aGeometry };
        auto aGeometries_p = aGeometries.data();

        // Scratch buffer
        vk::AccelerationStructureMemoryRequirementsInfoKHR sMemReqInfo{};
        sMemReqInfo.type = vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch;
        sMemReqInfo.buildType = vk::AccelerationStructureBuildTypeKHR::eDevice;
        sMemReqInfo.accelerationStructure = _aStruct;

        auto sMemReq = _device.getAccelerationStructureMemoryRequirementsKHR(sMemReqInfo);

        vk::BufferCreateInfo bufferCI{};
        bufferCI.size = sMemReq.memoryRequirements.size;
        bufferCI.usage = vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        bufferCI.sharingMode = vk::SharingMode::eExclusive;

        auto sBuffer = _allocator->create(bufferCI, VMA_MEMORY_USAGE_CPU_TO_GPU);

        vk::BufferDeviceAddressInfo sAddressInfo{};
        sAddressInfo.buffer = sBuffer.buffer;

        auto sAddress = _device.getBufferAddress(sAddressInfo);

        // Build
        vk::AccelerationStructureBuildGeometryInfoKHR aGeometryBI{};
        aGeometryBI.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
        aGeometryBI.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        aGeometryBI.update = false;
        aGeometryBI.dstAccelerationStructure = _aStruct;
        aGeometryBI.geometryArrayOfPointers = false;
        aGeometryBI.geometryCount = aGeometries.size();
        aGeometryBI.ppGeometries = &aGeometries_p;
        aGeometryBI.scratchData.deviceAddress = sAddress;

        vk::AccelerationStructureBuildOffsetInfoKHR aOffsetBI{};
        aOffsetBI.primitiveCount = aGeometryCI.maxPrimitiveCount;
        aOffsetBI.primitiveOffset = 0x0;
        aOffsetBI.firstVertex = 0;
        aOffsetBI.transformOffset = 0x0;

        const vk::AccelerationStructureBuildOffsetInfoKHR * const aOffsetBI_p_c = &aOffsetBI;

        if (ci.device->_rayTracingFeatures.rayTracingHostAccelerationStructureCommands) {
            // Implementation supports building acceleration structure building on host
            if (_device.buildAccelerationStructureKHR(aGeometryBI, aOffsetBI_p_c) != vk::Result::eSuccess)
                throw std::runtime_error("Unable to create BLAS");
        } else {
            // Acceleration structure needs to be build on the device
            auto cmd = ci.commandPool->singleTimeBegin();
            cmd->raw().buildAccelerationStructureKHR(aGeometryBI, aOffsetBI_p_c);
            ci.commandPool->singleTimeEnd(cmd, ci.queue);
        }

        vk::AccelerationStructureDeviceAddressInfoKHR aDeviceAddressInfo{};
        aDeviceAddressInfo.accelerationStructure = _aStruct;

        _aAddress = _device.getAccelerationStructureAddressKHR(aDeviceAddressInfo);

        // Cleanup
        _allocator->destroy(sBuffer.buffer, sBuffer.allocation);
    }

    vk::AccelerationStructureKHR BLAS_t::raw() {
        return _aStruct;
    }

    vk::DeviceAddress BLAS_t::address() {
        return _aAddress;
    }

    BLAS_t::~BLAS_t() {
        _device.destroyAccelerationStructureKHR(_aStruct);
        _allocator->free(_objMem);
    }
}

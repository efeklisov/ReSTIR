#include <tlas.hpp>

namespace hd {
    TLAS_t::TLAS_t(TLASCreateInfo ci) {
         _device = ci.device->raw();
         _allocator = ci.allocator;

        // TLAS
        vk::AccelerationStructureCreateGeometryTypeInfoKHR aGeometryCI{};
        aGeometryCI.geometryType = vk::GeometryTypeKHR::eInstances;
        aGeometryCI.maxPrimitiveCount = ci.instbuffer->count();
        aGeometryCI.allowsTransforms = true;

        vk::AccelerationStructureCreateInfoKHR aCI{};
        aCI.type = vk::AccelerationStructureTypeKHR::eTopLevel;
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
        vk::AccelerationStructureGeometryInstancesDataKHR instances{};
        instances.arrayOfPointers = false;
        instances.data.deviceAddress = ci.instbuffer->address().deviceAddress;

        vk::AccelerationStructureGeometryKHR aGeometry{};
        aGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
        aGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
        aGeometry.geometry.instances = instances;

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
        aGeometryBI.type = vk::AccelerationStructureTypeKHR::eTopLevel;
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
                throw std::runtime_error("Unable to create TLAS");
        } else {
            // Acceleration structure needs to be build on the device
            CommandBuffer cmd = ci.commandPool->singleTimeBegin();
            cmd->raw().buildAccelerationStructureKHR(aGeometryBI, aOffsetBI_p_c);
            ci.commandPool->singleTimeEnd(cmd, ci.queue);
        }

        vk::AccelerationStructureDeviceAddressInfoKHR aDeviceAddressInfo{};
        aDeviceAddressInfo.accelerationStructure = _aStruct;

        _aAddress = _device.getAccelerationStructureAddressKHR(aDeviceAddressInfo);

        // Cleanup
        _allocator->destroy(sBuffer.buffer, sBuffer.allocation);
    }

    vk::AccelerationStructureKHR TLAS_t::raw() {
        return _aStruct;
    }

    vk::DeviceAddress TLAS_t::address() {
        return _aAddress;
    }

    TLAS_t::~TLAS_t() {
        _device.destroyAccelerationStructureKHR(_aStruct);
        _allocator->free(_objMem);
    }
}

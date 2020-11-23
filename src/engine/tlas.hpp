#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/databuffer.hpp>
#include <hdvw/vertex.hpp>
#include <hdvw/device.hpp>
#include <hdvw/allocator.hpp>

#include <engine/blas.hpp>

#include <memory>

namespace hd {
    struct TLASCreateInfo {
        DataBuffer<vk::AccelerationStructureInstanceKHR> instbuffer;
        CommandPool commandPool;
        Queue queue;
        Device device;
        Allocator allocator;
    };

    class TLAS_t;
    typedef std::shared_ptr<TLAS_t> TLAS;

    class TLAS_t {
        private:
            vk::Device _device;
            Allocator _allocator;
            vk::AccelerationStructureKHR _aStruct;
            vk::DeviceAddress _aAddress;
            VmaAllocation _objMem;
            VmaAllocationInfo _objMemInfo;

        public:
            static TLAS conjure(TLASCreateInfo ci) {
                return std::make_shared<TLAS_t>(ci);
            }

            TLAS_t(TLASCreateInfo ci);

            vk::AccelerationStructureKHR raw();

            vk::DeviceAddress address();

            ~TLAS_t();
    };
}

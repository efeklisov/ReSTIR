#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/databuffer.hpp>
#include <hdvw/vertex.hpp>
#include <hdvw/device.hpp>
#include <hdvw/allocator.hpp>

#include <memory>

namespace hd {
    struct BLASCreateInfo {
        DataBuffer<Vertex> vertices;
        DataBuffer<uint32_t> indices;
        CommandPool commandPool;
        Queue queue;
        Device device;
        Allocator allocator;
    };

    class BLAS_t;
    typedef std::shared_ptr<BLAS_t> BLAS;

    class BLAS_t {
        private:
            vk::Device _device;
            Allocator _allocator;
            vk::AccelerationStructureKHR _aStruct;
            vk::DeviceAddress _aAddress;
            ReturnBuffer memory;

        public:
            static BLAS conjure(BLASCreateInfo ci) {
                return std::make_shared<BLAS_t>(ci);
            }

            BLAS_t(BLASCreateInfo ci);

            vk::AccelerationStructureKHR raw();

            vk::DeviceAddress address();

            ~BLAS_t();
    };
}

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
            ReturnBuffer memory;

        public:
            static TLAS conjure(TLASCreateInfo const & ci) {
                return std::make_shared<TLAS_t>(ci);
            }

            TLAS_t(TLASCreateInfo const & ci);

            inline auto raw() {
                return _aStruct;
            }

            inline auto address() {
                return _aAddress;
            }

            auto writeInfo(){
                vk::WriteDescriptorSetAccelerationStructureKHR info{};
                info.setAccelerationStructures(_aStruct);

                return info;
            }

            ~TLAS_t();
    };

    inline TLAS conjure(TLASCreateInfo const & ci) {
        return TLAS_t::conjure(ci);
    }
}

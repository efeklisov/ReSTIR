#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/buffer.hpp>
#include <hdvw/allocator.hpp>
#include <hdvw/pipeline.hpp>

#include <memory>
#include <vector>

namespace hd {
    struct SBTCreateInfo {
        Pipeline pipeline;
        Device device;
        Allocator allocator;
        uint32_t groupCount;
    };

    class SBT_t;
    typedef std::shared_ptr<SBT_t> SBT;

    class SBT_t {
        private:
            Buffer _buffer;
            vk::DeviceSize _shaderGroups;

        public:
            static SBT conjure(SBTCreateInfo ci) {
                return std::make_shared<SBT_t>(ci);
            }

            SBT_t(SBTCreateInfo ci);

            vk::Buffer raw();

            vk::DeviceSize size();
    };
}

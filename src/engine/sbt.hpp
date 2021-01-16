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
        uint32_t raygenCount;
        uint32_t missCount;
        uint32_t hitCount;
    };

    class SBT_t;
    typedef std::shared_ptr<SBT_t> SBT;

    struct SBTEntry {
        Buffer buffer;
        vk::DeviceAddress address;
        uint32_t count;
    };

    class SBT_t {
        private:
            SBTEntry _raygen;
            SBTEntry _miss;
            SBTEntry _hit;

        public:
            static SBT conjure(SBTCreateInfo ci) {
                return std::make_shared<SBT_t>(ci);
            }

            SBT_t(SBTCreateInfo ci);

            SBTEntry raygen();
            SBTEntry miss();
            SBTEntry hit();
    };
}

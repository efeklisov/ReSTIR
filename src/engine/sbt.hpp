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
        vk::StridedDeviceAddressRegionKHR region{};
    };

    class SBT_t {
        private:
            SBTEntry _raygen;
            SBTEntry _miss;
            SBTEntry _hit;

        public:
            static SBT conjure(SBTCreateInfo const & ci) {
                return std::make_shared<SBT_t>(ci);
            }

            SBT_t(SBTCreateInfo const & ci);

            inline auto raygen() {
                return _raygen;
            }

            inline auto miss() {
                return _miss;
            }

            inline auto hit() {
                return _hit;
            }
    };

    inline SBT conjure(SBTCreateInfo const & ci) {
        return SBT_t::conjure(ci);
    }
}

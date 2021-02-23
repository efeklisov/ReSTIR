#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>

#include <memory>

namespace hd {
    enum class FenceState {
        eSignaled,
        eIdle,
    };

    struct FenceCreateInfo {
        Device device;
        FenceState state = FenceState::eSignaled;
    };

    class Fence_t;
    typedef std::shared_ptr<Fence_t> Fence;

    class Fence_t {
        private:
            vk::Device _device;
            vk::Fence _fence;

        public:
            static Fence conjure(const FenceCreateInfo& ci) {
                return std::make_shared<Fence_t>(ci);
            }

            Fence_t(const FenceCreateInfo& ci);

            void wait();

            void reset();

            inline const auto raw() {
                return _fence;
            }

            ~Fence_t();
    };

    inline Fence conjure(const FenceCreateInfo& ci) {
        return Fence_t::conjure(ci);
    }
}

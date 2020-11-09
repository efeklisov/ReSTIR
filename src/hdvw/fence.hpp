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
            static Fence conjure(FenceCreateInfo ci) {
                return std::make_shared<Fence_t>(ci);
            }

            Fence_t(FenceCreateInfo ci);

            void wait();

            void reset();

            vk::Fence raw();

            ~Fence_t();
    };
}

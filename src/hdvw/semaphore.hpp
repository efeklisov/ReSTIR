#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>

#include <memory>

namespace hd {
    enum class SEMAPHORE {
    };

    struct SemaphoreCreateInfo {
        Device device;
        SEMAPHORE ambiguous;
    };

    class Semaphore_t;
    typedef std::shared_ptr<Semaphore_t> Semaphore;

    class Semaphore_t {
        private:
            vk::Device _device;
            vk::Semaphore _semaphore;

        public:
            static Semaphore conjure(const SemaphoreCreateInfo& ci) {
                return std::make_shared<Semaphore_t>(ci);
            }

            Semaphore_t(const SemaphoreCreateInfo& ci);

            inline const auto raw() {
                return _semaphore;
            }

            ~Semaphore_t();
    };

    inline Semaphore conjure(const SemaphoreCreateInfo& ci) {
        return Semaphore_t::conjure(ci);
    }
}

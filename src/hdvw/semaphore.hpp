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
            static Semaphore conjure(SemaphoreCreateInfo const & ci) {
                return std::make_shared<Semaphore_t>(ci);
            }

            Semaphore_t(SemaphoreCreateInfo const & ci);

            inline auto raw() {
                return _semaphore;
            }

            ~Semaphore_t();
    };

    inline Semaphore conjure(SemaphoreCreateInfo const & ci) {
        return Semaphore_t::conjure(ci);
    }
}

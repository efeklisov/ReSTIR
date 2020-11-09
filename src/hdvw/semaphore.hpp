#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>

#include <memory>

namespace hd {
    struct SemaphoreCreateInfo {
        Device device;
    };

    class Semaphore_t;
    typedef std::shared_ptr<Semaphore_t> Semaphore;

    class Semaphore_t {
        private:
            vk::Device _device;
            vk::Semaphore _semaphore;

        public:
            static Semaphore conjure(SemaphoreCreateInfo ci) {
                return std::make_shared<Semaphore_t>(ci);
            }

            Semaphore_t(SemaphoreCreateInfo ci);

            vk::Semaphore raw();

            ~Semaphore_t();
    };
}

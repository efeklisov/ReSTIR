#include <hdvw/semaphore.hpp>
using namespace hd;

Semaphore_t::Semaphore_t(SemaphoreCreateInfo ci) {
    _device = ci.device->raw();

    vk::SemaphoreCreateInfo sci = {};

    _semaphore = _device.createSemaphore(sci, nullptr);
}

vk::Semaphore Semaphore_t::raw() {
    return _semaphore;
}

Semaphore_t::~Semaphore_t() {
    _device.destroy(_semaphore);
}

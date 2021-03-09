#include <hdvw/semaphore.hpp>
using namespace hd;

Semaphore_t::Semaphore_t(SemaphoreCreateInfo const & ci) {
    _device = ci.device->raw();

    vk::SemaphoreCreateInfo sci = {};

    _semaphore = _device.createSemaphore(sci, nullptr);
}

Semaphore_t::~Semaphore_t() {
    _device.destroy(_semaphore);
}

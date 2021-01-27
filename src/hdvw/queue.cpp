#include <hdvw/queue.hpp>
using namespace hd;

#include <vector>
#include <stdexcept>

Queue_t::Queue_t(QueueCreateInfo ci) {
    type = ci.type;

    switch (ci.type) {
        case QueueType::eGraphics:
            _queue = ci.device->raw().getQueue(ci.device->indices().graphicsFamily.value(), graphicsIndex);

            if (ci.skipCheck || (graphicsIndex >= ci.device->indices().graphicsCount.value() - 1))
                return;

            graphicsIndex++;
            if (ci.device->indices().graphicsFamily.value() == ci.device->indices().presentFamily.value())
                presentIndex++;
            if (ci.device->indices().graphicsFamily.value() == ci.device->indices().transferFamily.value())
                transferIndex++;
            if (ci.device->indices().graphicsFamily.value() == ci.device->indices().computeFamily.value())
                computeIndex++;
            return;

        case QueueType::ePresent:
            _queue = ci.device->raw().getQueue(ci.device->indices().presentFamily.value(), presentIndex);

            if (ci.skipCheck || (presentIndex >= ci.device->indices().presentCount.value() - 1))
                return;

            presentIndex++;
            if (ci.device->indices().presentFamily.value() == ci.device->indices().graphicsFamily.value())
                graphicsIndex++;
            if (ci.device->indices().presentFamily.value() == ci.device->indices().transferFamily.value())
                transferIndex++;
            if (ci.device->indices().presentFamily.value() == ci.device->indices().computeFamily.value())
                computeIndex++;
            return;

        case QueueType::eTransfer:
            _queue = ci.device->raw().getQueue(ci.device->indices().transferFamily.value(), transferIndex);

            if (ci.skipCheck || (transferIndex >= ci.device->indices().transferCount.value() - 1))
                return;

            transferIndex++;
            if (ci.device->indices().transferFamily.value() == ci.device->indices().presentFamily.value())
                presentIndex++;
            if (ci.device->indices().transferFamily.value() == ci.device->indices().graphicsFamily.value())
                graphicsIndex++;
            if (ci.device->indices().transferFamily.value() == ci.device->indices().computeFamily.value())
                computeIndex++;
            return;

        case QueueType::eCompute:
            _queue = ci.device->raw().getQueue(ci.device->indices().computeFamily.value(), computeIndex);

            if (ci.skipCheck || (computeIndex >= ci.device->indices().computeCount.value() - 1))
                return;

            computeIndex++;
            if (ci.device->indices().computeFamily.value() == ci.device->indices().presentFamily.value())
                presentIndex++;
            if (ci.device->indices().computeFamily.value() == ci.device->indices().transferFamily.value())
                transferIndex++;
            if (ci.device->indices().computeFamily.value() == ci.device->indices().graphicsFamily.value())
                graphicsIndex++;
            return;
    }
}

void Queue_t::waitIdle() {
    _queue.waitIdle();
}

void Queue_t::submit(vk::SubmitInfo si, Fence fence) {
    if (fence == nullptr) {
        if (_queue.submit(1, &si, nullptr) != vk::Result::eSuccess)
            throw std::runtime_error("Couldn't submit to queue");
    } else {
        if (_queue.submit(1, &si, fence->raw()) != vk::Result::eSuccess)
            throw std::runtime_error("Couldn't submit to queue");
    }
}

void Queue_t::submit(std::vector<vk::SubmitInfo> si, Fence fence) {
    if (fence == nullptr) {
        if (_queue.submit(si.size(), si.data(), nullptr) != vk::Result::eSuccess)
            throw std::runtime_error("Couldn't submit to queue");;
    } else {
        if (_queue.submit(si.size(), si.data(), fence->raw()) != vk::Result::eSuccess)
            throw std::runtime_error("Couldn't submit to queue");;
    }
}

vk::Result Queue_t::present(vk::PresentInfoKHR& presentInfo) {
    if (type != QueueType::ePresent)
        throw std::runtime_error("This is not a 'present' queue");

    return _queue.presentKHR(&presentInfo);
}

uint32_t Queue_t::graphicsIndex{ 0 };
uint32_t Queue_t::presentIndex{ 0 };
uint32_t Queue_t::transferIndex{ 0 };
uint32_t Queue_t::computeIndex{ 0 };

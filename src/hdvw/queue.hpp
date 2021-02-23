#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>
#include <hdvw/fence.hpp>

#include <memory>

namespace hd {
    enum class QueueType {
        eGraphics,
        ePresent,
        eTransfer,
        eCompute,
    };

    struct QueueCreateInfo {
        Device device;
        QueueType type;
        bool skipCheck = false;
    };

    class Queue_t;
    typedef std::shared_ptr<Queue_t> Queue;

    class Queue_t {
        private:
            vk::Queue _queue;
            QueueType type;

            static uint32_t graphicsIndex;
            static uint32_t presentIndex;
            static uint32_t transferIndex;
            static uint32_t computeIndex;

        public:
            static Queue conjure(const QueueCreateInfo& ci) {
                return std::make_shared<Queue_t>(ci);
            }

            Queue_t(const QueueCreateInfo& ci);

            void waitIdle();

            void submit(vk::SubmitInfo si, Fence fence);

            void submit(std::vector<vk::SubmitInfo> si, Fence fence);

            vk::Result present(vk::PresentInfoKHR& presentInfo);

            inline const auto raw() {
                return _queue;
            }
    };

    inline Queue conjure(const QueueCreateInfo& ci) {
        return Queue_t::conjure(ci);
    }
}

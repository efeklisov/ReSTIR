#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>
#include <hdvw/queue.hpp>
#include <hdvw/commandbuffer.hpp>

#include <memory>

namespace hd {
    enum class PoolFamily {
        eGraphics,
        ePresent,
        eTransfer,
        eCompute,
    };

    struct CommandPoolCreateInfo {
        Device device;
        PoolFamily family;
        vk::CommandPoolCreateFlags flags{0};
    };

    class CommandPool_t;
    typedef std::shared_ptr<CommandPool_t> CommandPool;

    class CommandPool_t {
        private:
            vk::CommandPool _commandPool;
            vk::Device _device;

        public:
            static CommandPool conjure(CommandPoolCreateInfo ci) {
                return std::make_shared<CommandPool_t>(ci);
            }

            CommandPool_t(CommandPoolCreateInfo ci);
            
            std::vector<CommandBuffer> allocate(uint32_t count, vk::CommandBufferLevel level=vk::CommandBufferLevel::ePrimary);

            CommandBuffer singleTimeBegin();

            void singleTimeEnd(CommandBuffer buffer, Queue queue);

            vk::CommandPool raw();

            ~CommandPool_t();
    };
}

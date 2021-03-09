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
            static CommandPool conjure(CommandPoolCreateInfo const & ci) {
                return std::make_shared<CommandPool_t>(ci);
            }

            CommandPool_t(CommandPoolCreateInfo const & ci);
            
            std::vector<CommandBuffer> allocate(uint32_t count, vk::CommandBufferLevel level=vk::CommandBufferLevel::ePrimary);

            CommandBuffer singleTimeBegin();

            void singleTimeEnd(CommandBuffer buffer, Queue queue);

            inline auto raw() {
                return _commandPool;
            }

            ~CommandPool_t();
    };

    inline CommandPool conjure(CommandPoolCreateInfo const & ci) {
        return CommandPool_t::conjure(ci);
    }
}

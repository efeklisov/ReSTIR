#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/device.hpp>

#include <vector>
#include <memory>

namespace hd {
    struct ShaderCreateInfo {
        Device device;
        const char* filename;
        vk::ShaderStageFlagBits stage;
    };

    class Shader_t;
    typedef std::shared_ptr<Shader_t> Shader;

    class Shader_t {
        private:
            vk::Device _device;
            vk::ShaderModule _shaderModule;
            vk::PipelineShaderStageCreateInfo _shaderStageInfo = {};

            std::vector<char> read(const char* filename);

        public:
            static Shader conjure(ShaderCreateInfo const & ci) {
                return std::make_shared<Shader_t>(ci);
            }

            Shader_t(ShaderCreateInfo const & ci);

            inline auto info() {
                return _shaderStageInfo;
            }

            ~Shader_t();
    };

    inline Shader conjure(ShaderCreateInfo const & ci) {
        return Shader_t::conjure(ci);
    }
}

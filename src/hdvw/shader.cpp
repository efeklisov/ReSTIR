#include <hdvw/shader.hpp>
using namespace hd;

#include <fstream>

std::vector<char> Shader_t::read(const char* filename) {
    std::ifstream _file(filename, std::ios::ate | std::ios::binary);

    if (!_file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) _file.tellg();
    std::vector<char> buffer(fileSize);

    _file.seekg(0);
    _file.read(buffer.data(), fileSize);

    _file.close();

    return buffer;
}

Shader_t::Shader_t(ShaderCreateInfo ci) {
    _device = ci.device->raw();
    auto code = read(ci.filename);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    _shaderModule = _device.createShaderModule(createInfo);

    _shaderStageInfo.stage = ci.stage;
    _shaderStageInfo.module = _shaderModule;
    _shaderStageInfo.pName = "main";
}

vk::PipelineShaderStageCreateInfo Shader_t::info() {
    return _shaderStageInfo;
}

Shader_t::~Shader_t() {
    _device.destroy(_shaderModule);
}

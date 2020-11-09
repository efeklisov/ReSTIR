#include <hdvw/descriptorlayout.hpp>
using namespace hd;

DescriptorLayout_t::DescriptorLayout_t(DescriptorLayoutCreateInfo ci) {
    _device = ci.device->raw();

    for (auto & binding: ci.bindings) {
        if (_types.find(binding.descriptorType) == _types.end())
            _types[binding.descriptorType] = 1;
        else _types[binding.descriptorType]++;
    }

    vk::DescriptorSetLayoutCreateInfo dci;
    dci.bindingCount = ci.bindings.size();
    dci.pBindings = ci.bindings.data();
    dci.flags = ci.flags;

    _layout = _device.createDescriptorSetLayout(dci);
}

std::map<vk::DescriptorType, uint32_t> DescriptorLayout_t::types() {
    return _types;
}

vk::DescriptorSetLayout DescriptorLayout_t::raw() {
    return _layout;
}

DescriptorLayout_t::~DescriptorLayout_t() {
    _device.destroy(_layout);
}

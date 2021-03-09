#include <hdvw/descriptorlayout.hpp>
using namespace hd;

DescriptorLayout_t::DescriptorLayout_t(DescriptorLayoutCreateInfo const & ci) {
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

DescriptorLayout_t::~DescriptorLayout_t() {
    _device.destroy(_layout);
}

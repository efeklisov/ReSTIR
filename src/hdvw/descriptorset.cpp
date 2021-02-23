#include <hdvw/descriptorset.hpp>
using namespace hd;

DescriptorSet_t::DescriptorSet_t(const DescriptorSetCreateInfo& ci) {
    _device = ci.device;
    _set = ci.set;
}

vk::WriteDescriptorSet DescriptorSet_t::writeInfo(uint32_t binding, vk::DescriptorType type, uint32_t index, uint32_t count) {
    vk::WriteDescriptorSet writeSet{};
    writeSet.dstBinding = binding;
    writeSet.dstArrayElement = index;
    writeSet.descriptorType = type;
    writeSet.descriptorCount = count;
    writeSet.dstSet = _set;

    return writeSet;
};

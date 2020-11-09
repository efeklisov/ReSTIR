#include <hdvw/descriptorset.hpp>
using namespace hd;

DescriptorSet_t::DescriptorSet_t(DescriptorSetCreateInfo ci) {
    _device = ci.device;
    _set = ci.set;
}

void DescriptorSet_t::update(UpdateDescriptorBufferInfo ci) {
    auto writeSet = ci.writeSet;
    auto bufferInfo = ci.bufferInfo;

    writeSet.pBufferInfo = &bufferInfo;
    writeSet.dstSet = _set;

    _device.updateDescriptorSets(writeSet, nullptr);
}

void DescriptorSet_t::update(UpdateDescriptorImageInfo ci) {
    auto writeSet = ci.writeSet;
    auto imageInfo = ci.imageInfo;

    writeSet.pImageInfo = &imageInfo;
    writeSet.dstSet = _set;

    _device.updateDescriptorSets(writeSet, nullptr);
}

vk::DescriptorSet DescriptorSet_t::raw() {
    return _set;
}

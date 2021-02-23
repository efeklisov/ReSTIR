#include <hdvw/descriptorpool.hpp>
using namespace hd;

DescriptorPool_t::DescriptorPool_t(const DescriptorPoolCreateInfo& ci) {
    _device = ci.device->raw();

    std::map<vk::DescriptorType, uint32_t> types;

    uint32_t maxSets = 0;
    for (auto& mew : ci.layouts) {
        auto layoutTypes = mew.first->types();
        auto typeMultiplier = mew.second;

        for (auto& [key, val] : layoutTypes) {
            if (types.find(key) == types.end())
                types[key] = val * typeMultiplier;
            else types[key] += val * typeMultiplier;
        }
        maxSets += typeMultiplier;
    }

    std::vector<vk::DescriptorPoolSize> sizes = {};
    sizes.reserve(types.size());
    for (auto& [key, val] : types) {
        vk::DescriptorPoolSize size = {};
        size.descriptorCount = val;
        size.type = key;
        sizes.push_back(size);
    }

    vk::DescriptorPoolCreateInfo pci = {};
    pci.poolSizeCount = sizes.size();
    pci.pPoolSizes = sizes.data();
    pci.maxSets = maxSets;

    _pool = _device.createDescriptorPool(pci);
}

std::vector<DescriptorSet> DescriptorPool_t::allocate(uint32_t count, DescriptorLayout layout) {
    std::vector<vk::DescriptorSetLayout> layouts(count, layout->raw());

    vk::DescriptorSetAllocateInfo ai = {};
    ai.descriptorPool = _pool;
    ai.descriptorSetCount = layouts.size();
    ai.pSetLayouts = layouts.data();

    std::vector<DescriptorSet> sets;
    sets.reserve(layouts.size());

    for (auto& set: _device.allocateDescriptorSets(ai)) {
        sets.push_back(hd::conjure({
                    .set = set,
                    .device = _device,
                    }));
    }

    return sets;
}

DescriptorPool_t::~DescriptorPool_t() {
    _device.destroy(_pool);
}

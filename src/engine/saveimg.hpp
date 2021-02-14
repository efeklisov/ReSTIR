#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/allocator.hpp>
#include <hdvw/image.hpp>

#include <string_view>
#include <fstream>

namespace hd {
    void saveImg(hd::Image img, hd::Device device, hd::Allocator alloc, std::string method, uint32_t N, uint32_t tolerance, uint32_t frame);
};

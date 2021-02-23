#include <hdvw/surface.hpp>
using namespace hd;

#include <GLFW/glfw3.h>

#include <stdexcept>

Surface_t::Surface_t(const SurfaceCreateInfo& ci) {
    _instance = ci.instance->raw();
    auto psurface = static_cast<VkSurfaceKHR>(_surface);

    if (glfwCreateWindowSurface(_instance, ci.window->raw(), nullptr, &psurface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    _surface = static_cast<vk::SurfaceKHR>(psurface);
}

Surface_t::~Surface_t() {
    _instance.destroySurfaceKHR(_surface);
}

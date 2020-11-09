#pragma once

#include <vulkan/vulkan.hpp>

#include <hdvw/window.hpp>
#include <hdvw/instance.hpp>

#include <memory>

namespace hd {
    struct SurfaceCreateInfo {
        Window window;
        Instance instance;
    };

    class Surface_t;
    typedef std::shared_ptr<Surface_t> Surface;

    class Surface_t {
        private:
            vk::SurfaceKHR _surface;
            vk::Instance _instance;

        public:
            static Surface conjure(SurfaceCreateInfo ci) {
                return std::make_shared<Surface_t>(ci);
            }

            Surface_t(SurfaceCreateInfo ci);

            vk::SurfaceKHR raw();

            ~Surface_t();
    };
}

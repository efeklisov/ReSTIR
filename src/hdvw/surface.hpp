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
            static Surface conjure(const SurfaceCreateInfo& ci) {
                return std::make_shared<Surface_t>(ci);
            }

            Surface_t(const SurfaceCreateInfo& ci);

            inline const auto raw() {
                return _surface;
            }

            ~Surface_t();
    };

    inline Surface conjure(const SurfaceCreateInfo& ci) {
        return Surface_t::conjure(ci);
    }
}

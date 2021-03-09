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
            static Surface conjure(SurfaceCreateInfo const & ci) {
                return std::make_shared<Surface_t>(ci);
            }

            Surface_t(SurfaceCreateInfo const & ci);

            inline auto raw() {
                return _surface;
            }

            ~Surface_t();
    };

    inline Surface conjure(SurfaceCreateInfo const & ci) {
        return Surface_t::conjure(ci);
    }
}

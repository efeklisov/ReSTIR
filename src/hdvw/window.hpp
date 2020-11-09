#pragma once

#include <GLFW/glfw3.h>

#include <vector>
#include <memory>

namespace hd {
    struct WindowCreateInfo {
        uint32_t width = 800;
        uint32_t height = 600;
        const char* title;
        bool cursorVisible = false;
        void* windowUser = NULL;
        GLFWframebuffersizefun framebufferSizeCallback = NULL;
        GLFWcursorposfun cursorPosCallback = NULL;
        GLFWmousebuttonfun mouseButtonCallback = NULL;
    };

    class Window_t;
    typedef std::shared_ptr<Window_t> Window;

    class Window_t {
        private:
            GLFWwindow* _window;
            bool evl;

        public:
            static Window conjure(WindowCreateInfo ci) {
                return std::make_shared<Window_t>(ci);
            }

            Window_t(WindowCreateInfo ci);

            bool shouldClose();

            void pollEvents();

            void getFramebufferSize(uint32_t& width, uint32_t& height);

            std::vector<const char *> getRequiredExtensions();

            GLFWwindow* raw();

            ~Window_t();
    };
}

#include <hdvw/window.hpp>
using namespace hd;

Window_t::Window_t(WindowCreateInfo ci) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    _window = glfwCreateWindow(ci.width, ci.height, ci.title, nullptr, nullptr);
    glfwSetWindowUserPointer(_window, ci.windowUser);
    if (ci.cursorVisible)
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetFramebufferSizeCallback(_window, ci.framebufferSizeCallback);
    glfwSetCursorPosCallback(_window, ci.cursorPosCallback);
    glfwSetMouseButtonCallback(_window, ci.mouseButtonCallback);
}

bool Window_t::shouldClose() {
    return glfwWindowShouldClose(_window);
}

void Window_t::pollEvents() {
    glfwPollEvents();
}

void Window_t::getFramebufferSize(uint32_t& width, uint32_t& height) {
    int _width, _height;
    glfwGetFramebufferSize(_window, &_width, &_height);
    width = _width;
    height = _height;
}

std::vector<const char *> Window_t::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    return extensions;
}

GLFWwindow* Window_t::raw() {
    return _window;
}

Window_t::~Window_t() {
    glfwDestroyWindow(_window);
    glfwTerminate();
}

#include <easy2d/platform/window.h>
#include <easy2d/platform/input.h>
#include <easy2d/platform/glfw_user_pointer.h>
#include <easy2d/event/event_queue.h>
#include <easy2d/utils/logger.h>
#include <GLFW/glfw3.h>

namespace easy2d {

Window::Window()
    : window_(nullptr)
    , width_(0)
    , height_(0)
    , fullscreen_(false)
    , vsync_(true)
    , userData_(nullptr)
    , eventQueue_(nullptr) {
}

Window::~Window() {
    destroy();
}

bool Window::create(const WindowConfig& config) {
    if (window_) {
        E2D_LOG_WARN("Window already created");
        return false;
    }

    // 设置 GLFW 窗口提示
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    if (config.msaaSamples > 0) {
        glfwWindowHint(GLFW_SAMPLES, config.msaaSamples);
    }

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 创建窗口
    GLFWmonitor* monitor = config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    window_ = glfwCreateWindow(config.width, config.height, config.title.c_str(), monitor, nullptr);

    if (!window_) {
        E2D_LOG_ERROR("Failed to create GLFW window");
        return false;
    }

    width_ = config.width;
    height_ = config.height;
    fullscreen_ = config.fullscreen;
    vsync_ = config.vsync;

    // 设置当前上下文
    glfwMakeContextCurrent(window_);

    // 设置 VSync
    glfwSwapInterval(vsync_ ? 1 : 0);

    // 设置用户指针
    glfwUserPointer_ = makeUnique<GlfwUserPointer>();
    glfwUserPointer_->window = this;
    glfwSetWindowUserPointer(window_, glfwUserPointer_.get());

    // 设置回调
    glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);
    glfwSetWindowFocusCallback(window_, windowFocusCallback);
    glfwSetWindowCloseCallback(window_, windowCloseCallback);

    // 创建输入管理器
    input_ = makeUnique<Input>();
    input_->init(window_);
    glfwUserPointer_->input = input_.get();

    E2D_LOG_INFO("Window created: {}x{}", width_, height_);
    return true;
}

void Window::destroy() {
    if (window_) {
        input_.reset();
        glfwSetWindowUserPointer(window_, nullptr);
        glfwUserPointer_.reset();
        glfwDestroyWindow(window_);
        window_ = nullptr;
        E2D_LOG_INFO("Window destroyed");
    }
}

void Window::pollEvents() {
    glfwPollEvents();
    if (input_) {
        input_->update();
    }
}

void Window::swapBuffers() {
    if (window_) {
        glfwSwapBuffers(window_);
    }
}

bool Window::shouldClose() const {
    return window_ ? glfwWindowShouldClose(window_) : true;
}

void Window::setShouldClose(bool close) {
    if (window_) {
        glfwSetWindowShouldClose(window_, close);
    }
}

void Window::setTitle(const String& title) {
    if (window_) {
        glfwSetWindowTitle(window_, title.c_str());
    }
}

void Window::setSize(int width, int height) {
    if (window_) {
        glfwSetWindowSize(window_, width, height);
        width_ = width;
        height_ = height;
    }
}

void Window::setPosition(int x, int y) {
    if (window_) {
        glfwSetWindowPos(window_, x, y);
    }
}

Vec2 Window::getPosition() const {
    if (window_) {
        int x, y;
        glfwGetWindowPos(window_, &x, &y);
        return Vec2(static_cast<float>(x), static_cast<float>(y));
    }
    return Vec2::Zero();
}

void Window::setFullscreen(bool fullscreen) {
    if (window_ && fullscreen_ != fullscreen) {
        GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        
        if (fullscreen) {
            glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            width_ = mode->width;
            height_ = mode->height;
        } else {
            glfwSetWindowMonitor(window_, nullptr, 100, 100, 800, 600, 0);
            width_ = 800;
            height_ = 600;
        }
        fullscreen_ = fullscreen;
    }
}

void Window::setVSync(bool enabled) {
    vsync_ = enabled;
    glfwSwapInterval(enabled ? 1 : 0);
}

void Window::setResizable(bool resizable) {
    if (window_) {
        glfwSetWindowAttrib(window_, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
    }
}

bool Window::isFocused() const {
    return window_ ? glfwGetWindowAttrib(window_, GLFW_FOCUSED) : false;
}

bool Window::isMinimized() const {
    return window_ ? glfwGetWindowAttrib(window_, GLFW_ICONIFIED) : false;
}

bool Window::isMaximized() const {
    return window_ ? glfwGetWindowAttrib(window_, GLFW_MAXIMIZED) : false;
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* ctx = static_cast<GlfwUserPointer*>(glfwGetWindowUserPointer(window));
    Window* self = ctx ? ctx->window : nullptr;
    if (self) {
        self->width_ = width;
        self->height_ = height;
        if (self->resizeCallback_) {
            self->resizeCallback_(width, height);
        }
    }
}

void Window::windowFocusCallback(GLFWwindow* window, int focused) {
    auto* ctx = static_cast<GlfwUserPointer*>(glfwGetWindowUserPointer(window));
    Window* self = ctx ? ctx->window : nullptr;
    if (self && self->focusCallback_) {
        self->focusCallback_(focused == GLFW_TRUE);
    }
}

void Window::windowCloseCallback(GLFWwindow* window) {
    auto* ctx = static_cast<GlfwUserPointer*>(glfwGetWindowUserPointer(window));
    Window* self = ctx ? ctx->window : nullptr;
    if (self && self->closeCallback_) {
        self->closeCallback_();
    }
}

} // namespace easy2d

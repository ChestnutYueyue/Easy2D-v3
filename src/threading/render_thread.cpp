#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <easy2d/threading/render_thread.h>
#include <easy2d/platform/window.h>
#include <easy2d/utils/logger.h>
#include <easy2d/graphics/render_backend.h>

namespace easy2d {

RenderThread::RenderThread()
    : renderContext_(nullptr), window_(nullptr) {
}

RenderThread::~RenderThread() {
    stop();
}

bool RenderThread::start(GLFWwindow* primaryWindow, Window* window) {
    if (!primaryWindow || !window) {
        E2D_LOG_ERROR("RenderThread: Invalid window parameters");
        return false;
    }

    if (running_.load(std::memory_order_acquire)) {
        E2D_LOG_WARN("RenderThread: Already running");
        return true;
    }

    window_ = window;

    // 创建共享上下文窗口
    // 先将主线程的上下文分离
    glfwMakeContextCurrent(nullptr);

    // 创建不可见的小窗口来共享 OpenGL 上下文
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    renderContext_ = glfwCreateWindow(1, 1, "", nullptr, primaryWindow);

    // 恢复主线程上下文
    glfwMakeContextCurrent(primaryWindow);

    if (!renderContext_) {
        E2D_LOG_ERROR("RenderThread: Failed to create shared OpenGL context");
        return false;
    }

    // 标记为运行状态
    running_.store(true, std::memory_order_release);
    frameComplete_.store(true, std::memory_order_release);

    // 启动渲染线程
    thread_ = makeUnique<std::thread>([this] { renderLoop(); });

    E2D_LOG_INFO("RenderThread: Started successfully");
    return true;
}

void RenderThread::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    E2D_LOG_INFO("RenderThread: Stopping...");

    // 通知线程停止
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        running_.store(false, std::memory_order_release);
        frameReady_.store(true, std::memory_order_release);
    }
    frameReadyCV_.notify_one();

    // 等待线程结束
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    thread_.reset();

    // 清理 OpenGL 上下文
    if (renderContext_) {
        glfwDestroyWindow(renderContext_);
        renderContext_ = nullptr;
    }

    E2D_LOG_INFO("RenderThread: Stopped");
}

void RenderThread::submitFrame(const FrameData& frameData) {
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        if (!currentFrame_) {
            currentFrame_ = makeUnique<FrameData>();
        }
        *currentFrame_ = frameData;
        frameReady_.store(true, std::memory_order_release);
        frameComplete_.store(false, std::memory_order_release);
    }
    frameReadyCV_.notify_one();
}

void RenderThread::waitForFrameComplete() {
    std::unique_lock<std::mutex> lock(frameMutex_);
    frameCompleteCV_.wait(lock, [this] {
        return frameComplete_.load(std::memory_order_acquire) ||
               !running_.load(std::memory_order_acquire);
    });
}

void RenderThread::renderLoop() {
    // 将渲染线程的上下文设置为当前
    glfwMakeContextCurrent(renderContext_);

    // 初始化 GLEW
    if (glewInit() != GLEW_OK) {
        E2D_LOG_ERROR("RenderThread: Failed to initialize GLEW");
        running_.store(false, std::memory_order_release);
        return;
    }

    E2D_LOG_INFO("RenderThread: Render loop started");

    while (running_.load(std::memory_order_acquire)) {
        // 等待帧数据
        {
            std::unique_lock<std::mutex> lock(frameMutex_);
            frameReadyCV_.wait(lock, [this] {
                return frameReady_.load(std::memory_order_acquire) ||
                       !running_.load(std::memory_order_acquire);
            });

            if (!running_.load(std::memory_order_acquire)) {
                break;
            }

            if (!frameReady_.load(std::memory_order_acquire) || !currentFrame_) {
                continue;
            }

            // 这里不释放锁，因为需要访问 currentFrame_
            // 但是需要注意不要让主线程等待过久
        }

        // 执行渲染（在锁外，允许主线程继续）
        if (currentFrame_) {
            // 清屏
            glClearColor(currentFrame_->clearColor.r / 255.0f,
                        currentFrame_->clearColor.g / 255.0f,
                        currentFrame_->clearColor.b / 255.0f,
                        currentFrame_->clearColor.a / 255.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // 设置视口
            glViewport(currentFrame_->viewportX,
                      currentFrame_->viewportY,
                      currentFrame_->viewportWidth,
                      currentFrame_->viewportHeight);

            // 这里应该使用 RenderBackend 来执行实际的渲染命令
            // 但由于线程限制，我们延迟渲染逻辑集成
        }

        // 交换缓冲区
        glfwSwapBuffers(renderContext_);

        // 标记帧完成
        {
            std::lock_guard<std::mutex> lock(frameMutex_);
            frameReady_.store(false, std::memory_order_release);
            frameComplete_.store(true, std::memory_order_release);
        }
        frameCompleteCV_.notify_one();
    }

    E2D_LOG_INFO("RenderThread: Render loop exited");

    // 分离当前上下文
    glfwMakeContextCurrent(nullptr);
}

} // namespace easy2d

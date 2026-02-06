#pragma once

#include <easy2d/core/types.h>
#include <easy2d/threading/frame_data.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

struct GLFWwindow;

namespace easy2d {

// 前向声明
class Window;

// ============================================================================
// 渲染线程管理器
// ============================================================================
class RenderThread {
public:
    RenderThread();
    ~RenderThread();

    // 禁止拷贝
    RenderThread(const RenderThread&) = delete;
    RenderThread& operator=(const RenderThread&) = delete;

    // 生命周期
    bool start(GLFWwindow* primaryWindow, Window* window);
    void stop();

    // 帧提交
    void submitFrame(const FrameData& frameData);

    // 状态查询
    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    // 等待前一帧完成（可选）
    void waitForFrameComplete();

private:
    // 渲染线程入口点
    void renderLoop();

    // 成员变量
    GLFWwindow* renderContext_;
    Window* window_;
    UniquePtr<std::thread> thread_;

    // 同步原语
    std::mutex frameMutex_;
    std::condition_variable frameReadyCV_;
    std::condition_variable frameCompleteCV_;

    // 帧数据
    UniquePtr<FrameData> currentFrame_;

    // 状态
    std::atomic<bool> running_{false};
    std::atomic<bool> frameReady_{false};
    std::atomic<bool> frameComplete_{true};
};

} // namespace easy2d

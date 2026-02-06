#pragma once

#include <easy2d/core/types.h>
#include <easy2d/core/color.h>
#include <easy2d/core/math_types.h>
#include <easy2d/graphics/render_command.h>
#include <glm/mat4x4.hpp>
#include <vector>
#include <atomic>
#include <mutex>

namespace easy2d {

// ============================================================================
// 帧数据 - 包含一帧所有渲染信息
// ============================================================================
struct FrameData {
    // 摄像机和视口
    glm::mat4 viewProjectionMatrix;
    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 800;
    int viewportHeight = 600;

    // 清空颜色
    Color clearColor = Colors::Black;

    // 渲染命令列表
    std::vector<RenderCommand> renderCommands;

    // 帧编号（用于同步）
    uint64_t frameNumber = 0;

    void clear() {
        renderCommands.clear();
    }
};

// ============================================================================
// 帧数据缓冲 - 双缓冲结构
// ============================================================================
class FrameDataBuffer {
public:
    FrameDataBuffer();
    ~FrameDataBuffer() = default;

    // 禁止拷贝
    FrameDataBuffer(const FrameDataBuffer&) = delete;
    FrameDataBuffer& operator=(const FrameDataBuffer&) = delete;

    // 获取主线程用于写入的缓冲
    FrameData* getWriteBuffer();

    // 获取渲染线程用于读取的缓冲
    const FrameData* getReadBuffer() const;

    // 原子交换缓冲（主线程调用）
    void swap();

    // 获取当前写入索引
    int getWriteIndex() const { return writeIndex_.load(std::memory_order_acquire); }

    // 获取当前读取索引
    int getReadIndex() const { return readIndex_.load(std::memory_order_acquire); }

private:
    // 双缓冲
    FrameData buffers_[2];

    // 原子索引
    std::atomic<int> writeIndex_{0};
    mutable std::atomic<int> readIndex_{1};
};

} // namespace easy2d

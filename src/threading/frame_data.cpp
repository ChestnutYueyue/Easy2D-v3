#include <easy2d/threading/frame_data.h>

namespace easy2d {

FrameDataBuffer::FrameDataBuffer()
    : writeIndex_(0), readIndex_(1) {
}

FrameData* FrameDataBuffer::getWriteBuffer() {
    int idx = writeIndex_.load(std::memory_order_acquire);
    return &buffers_[idx];
}

const FrameData* FrameDataBuffer::getReadBuffer() const {
    int idx = readIndex_.load(std::memory_order_acquire);
    return &buffers_[idx];
}

void FrameDataBuffer::swap() {
    // 主线程将读取索引更新为当前写入索引
    int oldWrite = writeIndex_.load(std::memory_order_acquire);
    readIndex_.store(oldWrite, std::memory_order_release);

    // 切换写入索引到另一个缓冲
    int newWrite = 1 - oldWrite;
    writeIndex_.store(newWrite, std::memory_order_release);
}

} // namespace easy2d

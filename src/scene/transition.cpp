#include <easy2d/scene/transition.h>
#include <easy2d/graphics/render_backend.h>
#include <easy2d/graphics/camera.h>
#include <easy2d/core/math_types.h>
#include <algorithm>

namespace easy2d {

// ============================================================================
// 缓动函数
// ============================================================================
static float easeInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

static float easeOutQuad(float t) {
    return t * (2.0f - t);
}

// ============================================================================
// Transition 基类
// ============================================================================
Transition::Transition(float duration)
    : duration_(duration)
    , elapsed_(0.0f)
    , progress_(0.0f)
    , isFinished_(false)
    , isStarted_(false) {
}

void Transition::start(Ptr<Scene> from, Ptr<Scene> to) {
    outgoingScene_ = from;
    incomingScene_ = to;
    elapsed_ = 0.0f;
    progress_ = 0.0f;
    isFinished_ = false;
    isStarted_ = true;
}

void Transition::update(float dt) {
    if (!isStarted_ || isFinished_) {
        return;
    }
    
    elapsed_ += dt;
    progress_ = duration_ > 0.0f 
        ? std::min(1.0f, elapsed_ / duration_)
        : 1.0f;
    
    if (progress_ >= 1.0f) {
        onFinish();
    }
}

void Transition::render(RenderBackend& renderer) {
    if (!isStarted_ || isFinished_) {
        return;
    }
    
    onRenderTransition(renderer, easeInOutQuad(progress_));
}

float Transition::getFadeInAlpha() const {
    return easeOutQuad(progress_);
}

float Transition::getFadeOutAlpha() const {
    return 1.0f - easeOutQuad(progress_);
}

void Transition::onFinish() {
    isFinished_ = true;
    if (finishCallback_) {
        finishCallback_();
    }
}

// ============================================================================
// FadeTransition - 淡入淡出
// ============================================================================
FadeTransition::FadeTransition(float duration)
    : Transition(duration) {
}

void FadeTransition::onRenderTransition(RenderBackend& renderer, float progress) {
    // 渲染源场景（淡出）
    if (outgoingScene_) {
        outgoingScene_->renderScene(renderer);
    }
    
    // 渲染目标场景（淡入）
    if (incomingScene_) {
        incomingScene_->renderScene(renderer);
    }
}

// ============================================================================
// SlideTransition - 滑动
// ============================================================================
SlideTransition::SlideTransition(float duration, TransitionDirection direction)
    : Transition(duration)
    , direction_(direction) {
}

void SlideTransition::onRenderTransition(RenderBackend& renderer, float progress) {
    // 获取视口尺寸
    float screenWidth = 800.0f;
    float screenHeight = 600.0f;
    
    if (outgoingScene_) {
        Size viewportSize = outgoingScene_->getViewportSize();
        if (viewportSize.width > 0 && viewportSize.height > 0) {
            screenWidth = viewportSize.width;
            screenHeight = viewportSize.height;
        }
    } else if (incomingScene_) {
        Size viewportSize = incomingScene_->getViewportSize();
        if (viewportSize.width > 0 && viewportSize.height > 0) {
            screenWidth = viewportSize.width;
            screenHeight = viewportSize.height;
        }
    }
    
    // 渲染源场景（滑出）
    if (outgoingScene_) {
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        
        switch (direction_) {
            case TransitionDirection::Left:
                offsetX = -screenWidth * progress;
                break;
            case TransitionDirection::Right:
                offsetX = screenWidth * progress;
                break;
            case TransitionDirection::Up:
                offsetY = -screenHeight * progress;
                break;
            case TransitionDirection::Down:
                offsetY = screenHeight * progress;
                break;
        }
        
        // 保存原始相机位置
        Camera* camera = outgoingScene_->getActiveCamera();
        Vec2 originalPos = camera->getPosition();
        
        // 应用偏移
        camera->setPosition(originalPos.x + offsetX, originalPos.y + offsetY);
        
        // 渲染场景
        outgoingScene_->renderScene(renderer);
        
        // 恢复相机位置
        camera->setPosition(originalPos);
    }
    
    // 渲染目标场景（滑入）
    if (incomingScene_) {
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        
        switch (direction_) {
            case TransitionDirection::Left:
                offsetX = screenWidth * (1.0f - progress);
                break;
            case TransitionDirection::Right:
                offsetX = -screenWidth * (1.0f - progress);
                break;
            case TransitionDirection::Up:
                offsetY = screenHeight * (1.0f - progress);
                break;
            case TransitionDirection::Down:
                offsetY = -screenHeight * (1.0f - progress);
                break;
        }
        
        // 保存原始相机位置
        Camera* camera = incomingScene_->getActiveCamera();
        Vec2 originalPos = camera->getPosition();
        
        // 应用偏移
        camera->setPosition(originalPos.x + offsetX, originalPos.y + offsetY);
        
        // 渲染场景
        incomingScene_->renderScene(renderer);
        
        // 恢复相机位置
        camera->setPosition(originalPos);
    }
}

// ============================================================================
// ScaleTransition - 缩放
// ============================================================================
ScaleTransition::ScaleTransition(float duration)
    : Transition(duration) {
}

void ScaleTransition::onRenderTransition(RenderBackend& renderer, float progress) {
    // 源场景：缩小消失
    if (outgoingScene_) {
        float scale = 1.0f - progress;
        
        // 保存原始相机状态
        Camera* camera = outgoingScene_->getActiveCamera();
        float originalZoom = camera->getZoom();
        Vec2 originalPos = camera->getPosition();
        
        // 应用缩放（通过调整相机 zoom 实现）
        camera->setZoom(originalZoom * scale);
        
        // 渲染场景
        outgoingScene_->renderScene(renderer);
        
        // 恢复相机状态
        camera->setZoom(originalZoom);
        camera->setPosition(originalPos);
    }
    
    // 目标场景：放大出现
    if (incomingScene_) {
        float scale = progress;
        
        // 保存原始相机状态
        Camera* camera = incomingScene_->getActiveCamera();
        float originalZoom = camera->getZoom();
        Vec2 originalPos = camera->getPosition();
        
        // 应用缩放
        camera->setZoom(originalZoom * scale);
        
        // 渲染场景
        incomingScene_->renderScene(renderer);
        
        // 恢复相机状态
        camera->setZoom(originalZoom);
        camera->setPosition(originalPos);
    }
}

// ============================================================================
// FlipTransition - 翻页
// ============================================================================
FlipTransition::FlipTransition(float duration, Axis axis)
    : Transition(duration)
    , axis_(axis) {
}

void FlipTransition::onRenderTransition(RenderBackend& renderer, float progress) {
    float angle = progress * PI_F; // 180度翻转
    
    if (progress < 0.5f) {
        // 前半段：翻转源场景
        if (outgoingScene_) {
            float currentAngle = angle;
            
            // 保存原始相机状态
            Camera* camera = outgoingScene_->getActiveCamera();
            float originalRotation = camera->getRotation();
            
            // 应用旋转（水平翻转绕Y轴，垂直翻转绕X轴）
            if (axis_ == Axis::Horizontal) {
                // 水平轴翻转 - 模拟绕X轴旋转
                camera->setRotation(originalRotation + currentAngle * RAD_TO_DEG);
            } else {
                // 垂直轴翻转 - 模拟绕Y轴旋转
                camera->setRotation(originalRotation - currentAngle * RAD_TO_DEG);
            }
            
            // 渲染场景
            outgoingScene_->renderScene(renderer);
            
            // 恢复相机状态
            camera->setRotation(originalRotation);
        }
    } else {
        // 后半段：翻转目标场景
        if (incomingScene_) {
            float currentAngle = angle - PI_F;
            
            // 保存原始相机状态
            Camera* camera = incomingScene_->getActiveCamera();
            float originalRotation = camera->getRotation();
            
            // 应用旋转
            if (axis_ == Axis::Horizontal) {
                camera->setRotation(originalRotation + currentAngle * RAD_TO_DEG);
            } else {
                camera->setRotation(originalRotation - currentAngle * RAD_TO_DEG);
            }
            
            // 渲染场景
            incomingScene_->renderScene(renderer);
            
            // 恢复相机状态
            camera->setRotation(originalRotation);
        }
    }
}

// ============================================================================
// BoxTransition - 方块过渡
// ============================================================================
BoxTransition::BoxTransition(float duration, int divisions)
    : Transition(duration)
    , divisions_(divisions) {
}

void BoxTransition::onRenderTransition(RenderBackend& renderer, float progress) {
    // 简化的方块效果
    // 实际实现可能需要渲染到纹理然后裁剪显示
    
    if (progress < 0.5f) {
        // 源场景逐渐消失
        if (outgoingScene_) {
            outgoingScene_->renderScene(renderer);
        }
    } else {
        // 目标场景逐渐出现
        if (incomingScene_) {
            incomingScene_->renderScene(renderer);
        }
    }
}

} // namespace easy2d

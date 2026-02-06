#include <easy2d/graphics/camera.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace easy2d {

Camera::Camera()
    : left_(-1.0f), right_(1.0f), bottom_(-1.0f), top_(1.0f) {
}

Camera::Camera(float left, float right, float bottom, float top)
    : left_(left), right_(right), bottom_(bottom), top_(top) {
}

Camera::Camera(const Size& viewport)
    : left_(0.0f), right_(viewport.width), bottom_(viewport.height), top_(0.0f) {
}

void Camera::setPosition(const Vec2& position) {
    position_ = position;
    viewDirty_ = true;
}

void Camera::setPosition(float x, float y) {
    position_.x = x;
    position_.y = y;
    viewDirty_ = true;
}

void Camera::setRotation(float degrees) {
    rotation_ = degrees;
    viewDirty_ = true;
}

void Camera::setZoom(float zoom) {
    zoom_ = zoom;
    viewDirty_ = true;
    projDirty_ = true;
}

void Camera::setViewport(float left, float right, float bottom, float top) {
    left_ = left;
    right_ = right;
    bottom_ = bottom;
    top_ = top;
    projDirty_ = true;
}

void Camera::setViewport(const Rect& rect) {
    left_ = rect.left();
    right_ = rect.right();
    bottom_ = rect.bottom();
    top_ = rect.top();
    projDirty_ = true;
}

Rect Camera::getViewport() const {
    return Rect(left_, top_, right_ - left_, bottom_ - top_);
}

glm::mat4 Camera::getViewMatrix() const {
    if (viewDirty_) {
        viewMatrix_ = glm::mat4(1.0f);
        // 对于2D相机，我们只需要平移（注意Y轴方向）
        viewMatrix_ = glm::translate(viewMatrix_, glm::vec3(-position_.x, -position_.y, 0.0f));
        viewDirty_ = false;
    }
    return viewMatrix_;
}

glm::mat4 Camera::getProjectionMatrix() const {
    if (projDirty_) {
        // 对于2D游戏，Y轴向下增长（屏幕坐标系）
        // OpenGL默认Y轴向上，所以需要反转Y轴
        // left, right, bottom, top
        projMatrix_ = glm::ortho(
            left_, right_,     // X轴：从左到右
            top_, bottom_,     // Y轴：从上到下（反转）
            -1.0f, 1.0f
        );
        projDirty_ = false;
    }
    return projMatrix_;
}

glm::mat4 Camera::getViewProjectionMatrix() const {
    // 对于2D相机，我们主要依赖投影矩阵
    // 视口变换已经处理了坐标系转换
    return getProjectionMatrix();
}

Vec2 Camera::screenToWorld(const Vec2& screenPos) const {
    glm::vec4 clipPos(
        (screenPos.x - left_) / (right_ - left_) * 2.0f - 1.0f,
        1.0f - (screenPos.y - top_) / (bottom_ - top_) * 2.0f,
        0.0f, 1.0f
    );
    
    glm::mat4 invVP = glm::inverse(getViewProjectionMatrix());
    glm::vec4 worldPos = invVP * clipPos;
    
    return Vec2(worldPos.x, worldPos.y);
}

Vec2 Camera::worldToScreen(const Vec2& worldPos) const {
    glm::vec4 world(worldPos.x, worldPos.y, 0.0f, 1.0f);
    glm::vec4 clipPos = getViewProjectionMatrix() * world;
    
    return Vec2(
        (clipPos.x + 1.0f) * 0.5f * (right_ - left_) + left_,
        (1.0f - clipPos.y) * 0.5f * (bottom_ - top_) + top_
    );
}

Vec2 Camera::screenToWorld(float x, float y) const {
    return screenToWorld(Vec2(x, y));
}

Vec2 Camera::worldToScreen(float x, float y) const {
    return worldToScreen(Vec2(x, y));
}

void Camera::move(const Vec2& offset) {
    position_ += offset;
    viewDirty_ = true;
}

void Camera::move(float x, float y) {
    position_.x += x;
    position_.y += y;
    viewDirty_ = true;
}

void Camera::setBounds(const Rect& bounds) {
    bounds_ = bounds;
    hasBounds_ = true;
}

void Camera::clearBounds() {
    hasBounds_ = false;
}

void Camera::clampToBounds() {
    if (!hasBounds_) return;
    
    float viewportWidth = (right_ - left_) / zoom_;
    float viewportHeight = (bottom_ - top_) / zoom_;
    
    float minX = bounds_.left() + viewportWidth * 0.5f;
    float maxX = bounds_.right() - viewportWidth * 0.5f;
    float minY = bounds_.top() + viewportHeight * 0.5f;
    float maxY = bounds_.bottom() - viewportHeight * 0.5f;
    
    if (minX > maxX) {
        position_.x = bounds_.center().x;
    } else {
        position_.x = std::clamp(position_.x, minX, maxX);
    }
    
    if (minY > maxY) {
        position_.y = bounds_.center().y;
    } else {
        position_.y = std::clamp(position_.y, minY, maxY);
    }
    
    viewDirty_ = true;
}

void Camera::lookAt(const Vec2& target) {
    position_ = target;
    viewDirty_ = true;
}

} // namespace easy2d

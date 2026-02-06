#include <easy2d/ui/button.h>
#include <easy2d/graphics/render_backend.h>
#include <algorithm>

namespace easy2d {

Button::Button() {
    setSpatialIndexed(false);
    auto& dispatcher = getEventDispatcher();
    dispatcher.addListener(EventType::UIHoverEnter, [this](Event&) { hovered_ = true; });
    dispatcher.addListener(EventType::UIHoverExit, [this](Event&) { hovered_ = false; pressed_ = false; });
    dispatcher.addListener(EventType::UIPressed, [this](Event&) { pressed_ = true; });
    dispatcher.addListener(EventType::UIReleased, [this](Event&) { pressed_ = false; });
    dispatcher.addListener(EventType::UIClicked, [this](Event&) { if (onClick_) onClick_(); });
}

Ptr<Button> Button::create() {
    return makePtr<Button>();
}

void Button::setText(const String& text) {
    text_ = text;
    if (font_ && getSize().empty()) {
        Vec2 textSize = font_->measureText(text_);
        setSize(textSize.x + padding_.x * 2.0f, textSize.y + padding_.y * 2.0f);
    }
}

void Button::setFont(Ptr<FontAtlas> font) {
    font_ = font;
    if (font_ && getSize().empty() && !text_.empty()) {
        Vec2 textSize = font_->measureText(text_);
        setSize(textSize.x + padding_.x * 2.0f, textSize.y + padding_.y * 2.0f);
    }
}

void Button::setPadding(const Vec2& padding) {
    padding_ = padding;
    if (font_ && getSize().empty() && !text_.empty()) {
        Vec2 textSize = font_->measureText(text_);
        setSize(textSize.x + padding_.x * 2.0f, textSize.y + padding_.y * 2.0f);
    }
}

void Button::setTextColor(const Color& color) {
    textColor_ = color;
}

void Button::setBackgroundColor(const Color& normal, const Color& hover, const Color& pressed) {
    bgNormal_ = normal;
    bgHover_ = hover;
    bgPressed_ = pressed;
}

void Button::setBorder(const Color& color, float width) {
    borderColor_ = color;
    borderWidth_ = width;
}

void Button::setOnClick(Function<void()> callback) {
    onClick_ = std::move(callback);
}

void Button::setBackgroundImage(Ptr<Texture> normal, Ptr<Texture> hover, Ptr<Texture> pressed) {
    imgNormal_ = normal;
    imgHover_ = hover ? hover : normal;
    imgPressed_ = pressed ? pressed : (hover ? hover : normal);
    useImageBackground_ = (normal != nullptr);

    // 如果使用原图大小模式，设置按钮大小为图片大小
    if (useImageBackground_ && scaleMode_ == ImageScaleMode::Original && normal) {
        setSize(static_cast<float>(normal->getWidth()), static_cast<float>(normal->getHeight()));
    }
}

void Button::setBackgroundImageScaleMode(ImageScaleMode mode) {
    scaleMode_ = mode;
    // 如果切换到原图大小模式，更新按钮大小
    if (useImageBackground_ && scaleMode_ == ImageScaleMode::Original && imgNormal_) {
        setSize(static_cast<float>(imgNormal_->getWidth()), static_cast<float>(imgNormal_->getHeight()));
    }
}

void Button::setCustomSize(const Vec2& size) {
    setSize(size.x, size.y);
}

void Button::setCustomSize(float width, float height) {
    setSize(width, height);
}

Vec2 Button::calculateImageSize(const Vec2& buttonSize, const Vec2& imageSize) {
    switch (scaleMode_) {
        case ImageScaleMode::Original:
            return imageSize;

        case ImageScaleMode::Stretch:
            return buttonSize;

        case ImageScaleMode::ScaleFit: {
            float scaleX = buttonSize.x / imageSize.x;
            float scaleY = buttonSize.y / imageSize.y;
            float scale = std::min(scaleX, scaleY);
            return Vec2(imageSize.x * scale, imageSize.y * scale);
        }

        case ImageScaleMode::ScaleFill: {
            float scaleX = buttonSize.x / imageSize.x;
            float scaleY = buttonSize.y / imageSize.y;
            float scale = std::max(scaleX, scaleY);
            return Vec2(imageSize.x * scale, imageSize.y * scale);
        }
    }
    return imageSize;
}

void Button::drawBackgroundImage(RenderBackend& renderer, const Rect& rect) {
    Texture* texture = nullptr;
    if (pressed_ && imgPressed_) {
        texture = imgPressed_.get();
    } else if (hovered_ && imgHover_) {
        texture = imgHover_.get();
    } else if (imgNormal_) {
        texture = imgNormal_.get();
    }

    if (!texture) return;

    Vec2 imageSize(static_cast<float>(texture->getWidth()), static_cast<float>(texture->getHeight()));
    Vec2 buttonSize(rect.size.width, rect.size.height);
    Vec2 drawSize = calculateImageSize(buttonSize, imageSize);

    // 计算绘制位置（居中）
    Vec2 drawPos(
        rect.origin.x + (rect.size.width - drawSize.x) * 0.5f,
        rect.origin.y + (rect.size.height - drawSize.y) * 0.5f
    );

    Rect destRect(drawPos.x, drawPos.y, drawSize.x, drawSize.y);
    renderer.drawSprite(*texture, destRect, Rect(0, 0, imageSize.x, imageSize.y), Colors::White, 0.0f, Vec2::Zero());
}

void Button::onDraw(RenderBackend& renderer) {
    Rect rect = getBoundingBox();
    if (rect.empty()) {
        return;
    }

    // 绘制背景（图片或纯色）
    if (useImageBackground_) {
        drawBackgroundImage(renderer, rect);
    } else {
        Color bg = bgNormal_;
        if (pressed_) {
            bg = bgPressed_;
        } else if (hovered_) {
            bg = bgHover_;
        }
        renderer.fillRect(rect, bg);
    }

    // 绘制边框
    if (borderWidth_ > 0.0f) {
        renderer.drawRect(rect, borderColor_, borderWidth_);
    }

    // 绘制文字
    if (font_ && !text_.empty()) {
        Vec2 textSize = font_->measureText(text_);
        Vec2 textPos(rect.center().x - textSize.x * 0.5f, rect.center().y - textSize.y * 0.5f);

        float minX = rect.left() + padding_.x;
        float minY = rect.top() + padding_.y;
        if (textPos.x < minX) textPos.x = minX;
        if (textPos.y < minY) textPos.y = minY;

        renderer.drawText(*font_, text_, textPos, textColor_);
    }
}

} // namespace easy2d

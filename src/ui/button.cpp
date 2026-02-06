#include <easy2d/ui/button.h>

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

void Button::onDraw(RenderBackend& renderer) {
    Rect rect = getBoundingBox();
    if (rect.empty()) {
        return;
    }

    Color bg = bgNormal_;
    if (pressed_) {
        bg = bgPressed_;
    } else if (hovered_) {
        bg = bgHover_;
    }

    renderer.fillRect(rect, bg);
    if (borderWidth_ > 0.0f) {
        renderer.drawRect(rect, borderColor_, borderWidth_);
    }

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

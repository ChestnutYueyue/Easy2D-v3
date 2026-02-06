#pragma once

#include <easy2d/ui/widget.h>
#include <easy2d/graphics/font.h>

namespace easy2d {

class Button : public Widget {
public:
    Button();
    ~Button() override = default;

    static Ptr<Button> create();

    void setText(const String& text);
    const String& getText() const { return text_; }

    void setFont(Ptr<FontAtlas> font);
    Ptr<FontAtlas> getFont() const { return font_; }

    void setPadding(const Vec2& padding);
    Vec2 getPadding() const { return padding_; }

    void setTextColor(const Color& color);
    Color getTextColor() const { return textColor_; }

    void setBackgroundColor(const Color& normal, const Color& hover, const Color& pressed);
    void setBorder(const Color& color, float width);

    void setOnClick(Function<void()> callback);

protected:
    void onDraw(RenderBackend& renderer) override;

private:
    String text_;
    Ptr<FontAtlas> font_;
    Vec2 padding_ = Vec2(10.0f, 6.0f);

    Color textColor_ = Colors::White;
    Color bgNormal_ = Color(0.2f, 0.2f, 0.2f, 1.0f);
    Color bgHover_ = Color(0.28f, 0.28f, 0.28f, 1.0f);
    Color bgPressed_ = Color(0.15f, 0.15f, 0.15f, 1.0f);

    Color borderColor_ = Color(0.6f, 0.6f, 0.6f, 1.0f);
    float borderWidth_ = 1.0f;

    bool hovered_ = false;
    bool pressed_ = false;

    Function<void()> onClick_;
};

} // namespace easy2d

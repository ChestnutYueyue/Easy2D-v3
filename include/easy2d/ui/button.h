#pragma once

#include <easy2d/ui/widget.h>
#include <easy2d/graphics/font.h>
#include <easy2d/graphics/texture.h>

namespace easy2d {

// 图片缩放模式
enum class ImageScaleMode {
    Original,       // 使用原图大小
    Stretch,        // 拉伸填充
    ScaleFit,       // 等比缩放，保持完整显示
    ScaleFill       // 等比缩放，填充整个区域（可能裁剪）
};

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

    // 纯色背景设置
    void setBackgroundColor(const Color& normal, const Color& hover, const Color& pressed);
    void setBorder(const Color& color, float width);

    // 图片背景设置
    void setBackgroundImage(Ptr<Texture> normal, Ptr<Texture> hover = nullptr, Ptr<Texture> pressed = nullptr);
    void setBackgroundImageScaleMode(ImageScaleMode mode);
    void setCustomSize(const Vec2& size);
    void setCustomSize(float width, float height);

    void setOnClick(Function<void()> callback);

protected:
    void onDraw(RenderBackend& renderer) override;
    void drawBackgroundImage(RenderBackend& renderer, const Rect& rect);
    Vec2 calculateImageSize(const Vec2& buttonSize, const Vec2& imageSize);

private:
    String text_;
    Ptr<FontAtlas> font_;
    Vec2 padding_ = Vec2(10.0f, 6.0f);

    // 文字颜色
    Color textColor_ = Colors::White;

    // 纯色背景
    Color bgNormal_ = Color(0.2f, 0.2f, 0.2f, 1.0f);
    Color bgHover_ = Color(0.28f, 0.28f, 0.28f, 1.0f);
    Color bgPressed_ = Color(0.15f, 0.15f, 0.15f, 1.0f);

    // 图片背景
    Ptr<Texture> imgNormal_;
    Ptr<Texture> imgHover_;
    Ptr<Texture> imgPressed_;
    ImageScaleMode scaleMode_ = ImageScaleMode::Original;
    bool useImageBackground_ = false;

    // 边框
    Color borderColor_ = Color(0.6f, 0.6f, 0.6f, 1.0f);
    float borderWidth_ = 1.0f;

    bool hovered_ = false;
    bool pressed_ = false;

    Function<void()> onClick_;
};

} // namespace easy2d

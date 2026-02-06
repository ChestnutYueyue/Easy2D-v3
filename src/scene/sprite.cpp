#include <easy2d/scene/sprite.h>
#include <easy2d/graphics/render_backend.h>
#include <easy2d/graphics/texture.h>
#include <easy2d/graphics/render_command.h>

namespace easy2d {

Sprite::Sprite() = default;

Sprite::Sprite(Ptr<Texture> texture) {
    setTexture(texture);
}

void Sprite::setTexture(Ptr<Texture> texture) {
    texture_ = texture;
    if (texture_) {
        textureRect_ = Rect(0, 0, static_cast<float>(texture_->getWidth()), static_cast<float>(texture_->getHeight()));
    }
}

void Sprite::setTextureRect(const Rect& rect) {
    textureRect_ = rect;
}

void Sprite::setColor(const Color& color) {
    color_ = color;
}

void Sprite::setFlipX(bool flip) {
    flipX_ = flip;
}

void Sprite::setFlipY(bool flip) {
    flipY_ = flip;
}

Ptr<Sprite> Sprite::create() {
    return makePtr<Sprite>();
}

Ptr<Sprite> Sprite::create(Ptr<Texture> texture) {
    return makePtr<Sprite>(texture);
}

Ptr<Sprite> Sprite::create(Ptr<Texture> texture, const Rect& rect) {
    auto sprite = makePtr<Sprite>(texture);
    sprite->setTextureRect(rect);
    return sprite;
}

void Sprite::onDraw(RenderBackend& renderer) {
    if (!texture_ || !texture_->isValid()) {
        return;
    }
    
    // Calculate destination rectangle based on texture rect
    float width = textureRect_.width();
    float height = textureRect_.height();
    
    auto pos = getPosition();
    auto anchor = getAnchor();
    auto scale = getScale();
    Rect destRect(pos.x - width * anchor.x * scale.x, 
                  pos.y - height * anchor.y * scale.y,
                  width * scale.x, 
                  height * scale.y);
    
    // Adjust source rect for flipping
    Rect srcRect = textureRect_;
    if (flipX_) {
        srcRect.origin.x = srcRect.right();
        srcRect.size.width = -srcRect.size.width;
    }
    if (flipY_) {
        srcRect.origin.y = srcRect.bottom();
        srcRect.size.height = -srcRect.size.height;
    }
    
    renderer.drawSprite(*texture_, destRect, srcRect, color_, getRotation(), getAnchor());
}

void Sprite::generateRenderCommand(std::vector<RenderCommand>& commands, int zOrder) {
    if (!texture_ || !texture_->isValid()) {
        return;
    }

    // 计算目标矩形（与 onDraw 一致）
    float width = textureRect_.width();
    float height = textureRect_.height();

    auto pos = getPosition();
    auto anchor = getAnchor();
    auto scale = getScale();
    Rect destRect(pos.x - width * anchor.x * scale.x,
                  pos.y - height * anchor.y * scale.y,
                  width * scale.x,
                  height * scale.y);

    // 调整源矩形（翻转）
    Rect srcRect = textureRect_;
    if (flipX_) {
        srcRect.origin.x = srcRect.right();
        srcRect.size.width = -srcRect.size.width;
    }
    if (flipY_) {
        srcRect.origin.y = srcRect.bottom();
        srcRect.size.height = -srcRect.size.height;
    }

    // 创建渲染命令
    RenderCommand cmd;
    cmd.type = RenderCommandType::Sprite;
    cmd.zOrder = zOrder;
    cmd.data = SpriteData{
        texture_,
        destRect,
        srcRect,
        color_,
        getRotation(),
        anchor
    };

    commands.push_back(std::move(cmd));
}

} // namespace easy2d

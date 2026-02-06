#pragma once

#include <easy2d/graphics/texture.h>
#include <GL/glew.h>

namespace easy2d {

// ============================================================================
// OpenGL 纹理实现
// ============================================================================
class GLTexture : public Texture {
public:
    GLTexture(int width, int height, const uint8_t* pixels, int channels);
    GLTexture(const std::string& filepath);
    ~GLTexture();

    // Texture 接口实现
    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }
    Size getSize() const override { return Size(static_cast<float>(width_), static_cast<float>(height_)); }
    int getChannels() const override { return channels_; }
    void* getNativeHandle() const override { return reinterpret_cast<void*>(static_cast<uintptr_t>(textureID_)); }
    bool isValid() const override { return textureID_ != 0; }
    void setFilter(bool linear) override;
    void setWrap(bool repeat) override;

    // OpenGL 特定
    GLuint getTextureID() const { return textureID_; }
    void bind(unsigned int slot = 0) const;
    void unbind() const;

private:
    GLuint textureID_;
    int width_;
    int height_;
    int channels_;

    void createTexture(const uint8_t* pixels);
};

} // namespace easy2d

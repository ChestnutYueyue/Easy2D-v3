#include <easy2d/graphics/opengl/gl_texture.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <easy2d/utils/logger.h>

namespace easy2d {

GLTexture::GLTexture(int width, int height, const uint8_t* pixels, int channels)
    : textureID_(0), width_(width), height_(height), channels_(channels) {
    createTexture(pixels);
}

GLTexture::GLTexture(const std::string& filepath)
    : textureID_(0), width_(0), height_(0), channels_(0) {
    stbi_set_flip_vertically_on_load(true);
    uint8_t* data = stbi_load(filepath.c_str(), &width_, &height_, &channels_, 0);
    if (data) {
        createTexture(data);
        stbi_image_free(data);
    } else {
        E2D_LOG_ERROR("Failed to load texture: {}", filepath);
    }
}

GLTexture::~GLTexture() {
    if (textureID_ != 0) {
        glDeleteTextures(1, &textureID_);
    }
}

void GLTexture::setFilter(bool linear) {
    bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
}

void GLTexture::setWrap(bool repeat) {
    bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
}

void GLTexture::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, textureID_);
}

void GLTexture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLTexture::createTexture(const uint8_t* pixels) {
    GLenum format = GL_RGBA;
    GLenum internalFormat = GL_RGBA8;
    int unpackAlignment = 4;
    if (channels_ == 1) {
        format = GL_RED;
        internalFormat = GL_R8;
        unpackAlignment = 1;
    } else if (channels_ == 3) {
        format = GL_RGB;
        internalFormat = GL_RGB8;
        unpackAlignment = 1;
    } else if (channels_ == 4) {
        format = GL_RGBA;
        internalFormat = GL_RGBA8;
        unpackAlignment = 4;
    }

    glGenTextures(1, &textureID_);
    bind();
    
    GLint prevUnpackAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevUnpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width_, height_, 0, format, GL_UNSIGNED_BYTE, pixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, prevUnpackAlignment);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glGenerateMipmap(GL_TEXTURE_2D);
}

} // namespace easy2d

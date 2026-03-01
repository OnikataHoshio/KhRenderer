#include "KH_Framebuffer.h"

KH_Framebuffer::KH_Framebuffer()
{

}

KH_Framebuffer::KH_Framebuffer(uint32_t width, uint32_t height) {
    Create(width, height);
}

KH_Framebuffer::~KH_Framebuffer() {
    glDeleteFramebuffers(1, &FBO);
    glDeleteTextures(1, &TextureID);
    glDeleteRenderbuffers(1, &RBO);
}

void KH_Framebuffer::Create(uint32_t width, uint32_t height)
{
    if (FBO != 0)
        return;

    this->Width = width;
    this->Height = height;

    glGenFramebuffers(1, &FBO);
    Bind();

    // 1. 创建颜色附件纹理
    glGenTextures(1, &TextureID);
    glBindTexture(GL_TEXTURE_2D, TextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TextureID, 0);

    // 2. 创建深度和模板附件 (RBO)
    glGenRenderbuffers(1, &RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Error: Framebuffer is not complete!" << std::endl;

    Unbind();
}

void KH_Framebuffer::Bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
}

void KH_Framebuffer::Unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void KH_Framebuffer::Rescale(uint32_t width, uint32_t height) {

    this->Width = width;
    this->Height = height;
    glBindTexture(GL_TEXTURE_2D, TextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

uint32_t KH_Framebuffer::GetWidth()
{
    return Width;
}

uint32_t KH_Framebuffer::GetHeight()
{
    return Height;
}

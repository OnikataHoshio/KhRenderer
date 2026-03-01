#pragma once

#include "KH_Common.h"

class KH_Framebuffer {
public:
    KH_Framebuffer();
    KH_Framebuffer(uint32_t width, uint32_t height);
    ~KH_Framebuffer();

    void Create(uint32_t width, uint32_t height);
    void Bind() const;
    void Unbind() const;
    void Rescale(uint32_t width, uint32_t height);

    uint32_t GetWidth();
    uint32_t GetHeight();

    uint32_t GetTextureID() const { return TextureID; }

private:  
    uint32_t FBO = 0;
    uint32_t TextureID = 0;
    uint32_t RBO = 0;

    uint32_t Width;
    uint32_t Height;
};
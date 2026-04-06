#pragma once

#include "KH_Common.h"

class KH_Shader;

enum class KH_FramebufferTextureFormat
{
    None = 0,

    // Color
    RGBA8,
    RGBA16F,
    RGBA32F,
    RG16F,
    R8,
    R32I,

    // Depth / Stencil
    DEPTH24STENCIL8,
    DEPTH32F
};

struct KH_FramebufferTextureDescription
{
    KH_FramebufferTextureFormat Format = KH_FramebufferTextureFormat::None;

    KH_FramebufferTextureDescription() = default;
    KH_FramebufferTextureDescription(KH_FramebufferTextureFormat format)
        : Format(format) {
    }
};

struct KH_FramebufferAttachmentDescription
{
    std::vector<KH_FramebufferTextureDescription> Attachments;

    KH_FramebufferAttachmentDescription() = default;
    KH_FramebufferAttachmentDescription(std::initializer_list<KH_FramebufferTextureDescription> attachments)
        : Attachments(attachments) {
    }
};

struct KH_FramebufferDescription
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    KH_FramebufferAttachmentDescription Attachments;
};


class KH_Framebuffer
{
public:
    KH_Framebuffer() = default;
    explicit KH_Framebuffer(const KH_FramebufferDescription& desc);
    ~KH_Framebuffer();

    KH_Framebuffer(const KH_Framebuffer&) = delete;
    KH_Framebuffer& operator=(const KH_Framebuffer&) = delete;

    KH_Framebuffer(KH_Framebuffer&& other) noexcept;
    KH_Framebuffer& operator=(KH_Framebuffer&& other) noexcept;

    void Create(const KH_FramebufferDescription& desc);
    void Invalidate();
    void Release();

    void Bind() const;
    void Unbind() const;
    void Resize(uint32_t width, uint32_t height);
    void Clear(const glm::vec4& color) const;
    void ClearColorAttachment(uint32_t attachmentIndex, const glm::vec4& value) const;
    void ClearDepthAttachment(float value) const;
    void ClearDepthStencilAttachment(float depth, int stencil) const;

    void BindColorAttachment(uint32_t attachmentIndex = 0, uint32_t unit = 0) const;
    void BindDepthAttachment(uint32_t unit = 0) const;
    void UnbindTexture(uint32_t unit = 0) const;

    uint32_t GetRendererID() const { return FBO; }
    uint32_t GetWidth() const { return Desc.Width; }
    uint32_t GetHeight() const { return Desc.Height; }

    uint32_t GetColorAttachmentID(uint32_t index = 0) const;
    uint32_t GetDepthAttachmentID() const { return DepthAttachment; }

    const KH_FramebufferDescription& GetDescription() const { return Desc; }

private:
    static bool IsDepthFormat(KH_FramebufferTextureFormat format);
    static GLenum ToGLInternalFormat(KH_FramebufferTextureFormat format);
    static GLenum ToGLDataFormat(KH_FramebufferTextureFormat format);

private:
    KH_FramebufferDescription Desc;

    uint32_t FBO = 0;
    std::vector<uint32_t> ColorAttachments;
    uint32_t DepthAttachment = 0;

    std::vector<KH_FramebufferTextureDescription> ColorAttachmentDescs;
    KH_FramebufferTextureDescription DepthAttachmentDesc = KH_FramebufferTextureFormat::None;
};
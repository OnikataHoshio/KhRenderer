#include "KH_Framebuffer.h"
#include "KH_Shader.h"
#include "Utils/KH_DebugUtils.h"

namespace
{
    static void CreateTextures(bool multisample, uint32_t* outID, uint32_t count)
    {
        glCreateTextures(multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, count, outID);
    }

    static void BindTexture(bool multisample, uint32_t id)
    {
        glBindTexture(multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, id);
    }

    static void AttachColorTexture(uint32_t id,
        GLenum internalFormat,
        GLenum format,
        uint32_t width,
        uint32_t height,
        uint32_t index)
    {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, id, 0);
    }

    static void AttachIntegerColorTexture(uint32_t id,
        GLenum internalFormat,
        GLenum format,
        uint32_t width,
        uint32_t height,
        uint32_t index)
    {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_INT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, id, 0);
    }

    static void AttachDepthTexture(uint32_t id,
        GLenum internalFormat,
        GLenum attachmentType,
        uint32_t width,
        uint32_t height)
    {
        glBindTexture(GL_TEXTURE_2D, id);

        if (attachmentType == GL_DEPTH_ATTACHMENT)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, id, 0);
    }
}

KH_Framebuffer::KH_Framebuffer(const KH_FramebufferDescription& desc)
{
    Create(desc);
}

KH_Framebuffer::~KH_Framebuffer()
{
    Release();
}

KH_Framebuffer::KH_Framebuffer(KH_Framebuffer&& other) noexcept
    : Desc(std::move(other.Desc)),
    FBO(other.FBO),
    ColorAttachments(std::move(other.ColorAttachments)),
    DepthAttachment(other.DepthAttachment),
    ColorAttachmentDescs(std::move(other.ColorAttachmentDescs)),
    DepthAttachmentDesc(other.DepthAttachmentDesc)
{
    other.FBO = 0;
    other.DepthAttachment = 0;
}

KH_Framebuffer& KH_Framebuffer::operator=(KH_Framebuffer&& other) noexcept
{
    if (this != &other)
    {
        Release();

        Desc = std::move(other.Desc);
        FBO = other.FBO;
        ColorAttachments = std::move(other.ColorAttachments);
        DepthAttachment = other.DepthAttachment;
        ColorAttachmentDescs = std::move(other.ColorAttachmentDescs);
        DepthAttachmentDesc = other.DepthAttachmentDesc;

        other.FBO = 0;
        other.DepthAttachment = 0;
    }
    return *this;
}

void KH_Framebuffer::Create(const KH_FramebufferDescription& desc)
{
    Desc = desc;

    ColorAttachmentDescs.clear();
    DepthAttachmentDesc = KH_FramebufferTextureFormat::None;

    for (const auto& attachment : Desc.Attachments.Attachments)
    {
        if (IsDepthFormat(attachment.Format))
            DepthAttachmentDesc = attachment;
        else
            ColorAttachmentDescs.push_back(attachment);
    }

    Invalidate();
}

void KH_Framebuffer::Invalidate()
{
    Release();

    glCreateFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    if (!ColorAttachmentDescs.empty())
    {
        ColorAttachments.resize(ColorAttachmentDescs.size());
        glCreateTextures(GL_TEXTURE_2D, static_cast<GLsizei>(ColorAttachments.size()), ColorAttachments.data());

        for (size_t i = 0; i < ColorAttachments.size(); ++i)
        {
            const auto format = ColorAttachmentDescs[i].Format;

            switch (format)
            {
            case KH_FramebufferTextureFormat::RGBA8:
                AttachColorTexture(ColorAttachments[i], GL_RGBA8, GL_RGBA, Desc.Width, Desc.Height, static_cast<uint32_t>(i));
                break;
            case KH_FramebufferTextureFormat::RGBA16F:
                AttachColorTexture(ColorAttachments[i], GL_RGBA16F, GL_RGBA, Desc.Width, Desc.Height, static_cast<uint32_t>(i));
                break;
            case KH_FramebufferTextureFormat::RGBA32F:
                AttachColorTexture(ColorAttachments[i], GL_RGBA32F, GL_RGBA, Desc.Width, Desc.Height, static_cast<uint32_t>(i));
                break;
            case KH_FramebufferTextureFormat::RG16F:
                glBindTexture(GL_TEXTURE_2D, ColorAttachments[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, Desc.Width, Desc.Height, 0, GL_RG, GL_FLOAT, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i), GL_TEXTURE_2D, ColorAttachments[i], 0);
                break;
            case KH_FramebufferTextureFormat::R8:
                glBindTexture(GL_TEXTURE_2D, ColorAttachments[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, Desc.Width, Desc.Height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i), GL_TEXTURE_2D, ColorAttachments[i], 0);
                break;
            case KH_FramebufferTextureFormat::R32I:
                AttachIntegerColorTexture(ColorAttachments[i], GL_R32I, GL_RED_INTEGER, Desc.Width, Desc.Height, static_cast<uint32_t>(i));
                break;
            default:
                assert(false && "Unsupported color attachment format");
                break;
            }
        }
    }

    if (DepthAttachmentDesc.Format != KH_FramebufferTextureFormat::None)
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &DepthAttachment);

        switch (DepthAttachmentDesc.Format)
        {
        case KH_FramebufferTextureFormat::DEPTH24STENCIL8:
            AttachDepthTexture(DepthAttachment, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, Desc.Width, Desc.Height);
            break;
        case KH_FramebufferTextureFormat::DEPTH32F:
            AttachDepthTexture(DepthAttachment, GL_DEPTH_COMPONENT32F, GL_DEPTH_ATTACHMENT, Desc.Width, Desc.Height);
            break;
        default:
            assert(false && "Unsupported depth attachment format");
            break;
        }
    }

    if (ColorAttachments.size() > 1)
    {
        assert(ColorAttachments.size() <= 8);
        GLenum buffers[8] = {
            GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
            GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7
        };
        glDrawBuffers(static_cast<GLsizei>(ColorAttachments.size()), buffers);
    }
    else if (ColorAttachments.empty())
    {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_E("Framebuffer is not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void KH_Framebuffer::Release()
{
    if (FBO)
    {
        glDeleteFramebuffers(1, &FBO);
        FBO = 0;
    }

    if (!ColorAttachments.empty())
    {
        glDeleteTextures(static_cast<GLsizei>(ColorAttachments.size()), ColorAttachments.data());
        ColorAttachments.clear();
    }

    if (DepthAttachment)
    {
        glDeleteTextures(1, &DepthAttachment);
        DepthAttachment = 0;
    }
}

void KH_Framebuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glViewport(0, 0, static_cast<GLsizei>(Desc.Width), static_cast<GLsizei>(Desc.Height));
}

void KH_Framebuffer::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void KH_Framebuffer::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    if (Desc.Width == width && Desc.Height == height)
        return;

    Desc.Width = width;
    Desc.Height = height;
    Invalidate();
}

void KH_Framebuffer::Clear(const glm::vec4& color) const
{
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void KH_Framebuffer::ClearColorAttachment(uint32_t attachmentIndex, const glm::vec4& value) const
{
    assert(attachmentIndex < ColorAttachments.size());

    const auto format = ColorAttachmentDescs[attachmentIndex].Format;

    switch (format)
    {
    case KH_FramebufferTextureFormat::RGBA8:
    {
        const float clearValue[4] = { value.r, value.g, value.b, value.a };
        glClearTexImage(ColorAttachments[attachmentIndex], 0, GL_RGBA, GL_FLOAT, clearValue);
        break;
    }
    case KH_FramebufferTextureFormat::RGBA16F:
    {
        const float clearValue[4] = { value.r, value.g, value.b, value.a };
        glClearTexImage(ColorAttachments[attachmentIndex], 0, GL_RGBA, GL_FLOAT, clearValue);
        break;
    }
    case KH_FramebufferTextureFormat::RGBA32F:
    {
        const float clearValue[4] = { value.r, value.g, value.b, value.a };
        glClearTexImage(ColorAttachments[attachmentIndex], 0, GL_RGBA, GL_FLOAT, clearValue);
        break;
    }
    case KH_FramebufferTextureFormat::RG16F:
    {
        const float clearValue[2] = { value.r, value.g };
        glClearTexImage(ColorAttachments[attachmentIndex], 0, GL_RG, GL_FLOAT, clearValue);
        break;
    }
    case KH_FramebufferTextureFormat::R8:
    {
        const float clearValue[1] = { value.r };
        glClearTexImage(ColorAttachments[attachmentIndex], 0, GL_RED, GL_FLOAT, clearValue);
        break;
    }
    case KH_FramebufferTextureFormat::R32I:
    {
        const int clearValue[1] = { static_cast<int>(value.r) };
        glClearTexImage(ColorAttachments[attachmentIndex], 0, GL_RED_INTEGER, GL_INT, clearValue);
        break;
    }
    default:
        assert(false && "Unsupported color attachment format for ClearColorAttachment");
        break;
    }
}

void KH_Framebuffer::ClearDepthAttachment(float value) const
{
    assert(DepthAttachment != 0);

    switch (DepthAttachmentDesc.Format)
    {
    case KH_FramebufferTextureFormat::DEPTH32F:
        glClearTexImage(DepthAttachment, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &value);
        break;
    default:
        assert(false && "Unsupported depth attachment format for ClearDepthAttachment");
        break;
    }
}

void KH_Framebuffer::ClearDepthStencilAttachment(float depth, int stencil) const
{
    assert(DepthAttachment != 0);

    if (DepthAttachmentDesc.Format == KH_FramebufferTextureFormat::DEPTH32F)
    {
        glClearTexImage(DepthAttachment, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    }
    else if (DepthAttachmentDesc.Format == KH_FramebufferTextureFormat::DEPTH24STENCIL8)
    {
        glClearNamedFramebufferfi(FBO, GL_DEPTH_STENCIL, 0, depth, stencil);
    }
}

//void KH_Framebuffer::BindColorAttachment(KH_Shader& shader, const std::string& name, uint32_t attachmentIndex, uint32_t unit) const
//{
//    BindColorAttachment(attachmentIndex, unit);
//    shader.SetInt(name, static_cast<int>(unit));
//}
//
//void KH_Framebuffer::BindDepthAttachment(KH_Shader& shader, const std::string& name, uint32_t unit) const
//{
//    BindDepthAttachment(unit);
//    shader.SetInt(name, static_cast<int>(unit));
//}

void KH_Framebuffer::BindColorAttachment(uint32_t attachmentIndex, uint32_t unit) const
{
    assert(attachmentIndex < ColorAttachments.size());
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ColorAttachments[attachmentIndex]);
}

void KH_Framebuffer::BindDepthAttachment(uint32_t unit) const
{
    assert(DepthAttachment != 0);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, DepthAttachment);
}

void KH_Framebuffer::UnbindTexture(uint32_t unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

uint32_t KH_Framebuffer::GetColorAttachmentID(uint32_t index) const
{
    assert(index < ColorAttachments.size());
    return ColorAttachments[index];
}

bool KH_Framebuffer::IsDepthFormat(KH_FramebufferTextureFormat format)
{
    return format == KH_FramebufferTextureFormat::DEPTH24STENCIL8 ||
        format == KH_FramebufferTextureFormat::DEPTH32F;
}

GLenum KH_Framebuffer::ToGLInternalFormat(KH_FramebufferTextureFormat format)
{
    return 0;
}

GLenum KH_Framebuffer::ToGLDataFormat(KH_FramebufferTextureFormat format)
{
    return 0;
}

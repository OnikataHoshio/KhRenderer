#include "KH_Texture.h"

#include "Scene/KH_Scene.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#include "Utils/KH_DebugUtils.h"

KH_TextureResource::~KH_TextureResource()
{
    if (ID != 0)
    {
        glDeleteTextures(1, &ID);
        ID = 0;
    }
}

KH_Texture::KH_Texture(std::shared_ptr<KH_TextureResource> resource)
    : Resource(std::move(resource))
{
}

bool KH_Texture::IsValid() const
{
    return Resource != nullptr && Resource->ID != 0;
}

unsigned int KH_Texture::GetID() const
{
    return Resource ? Resource->ID : 0;
}

KH_TEXTURE_TYPE KH_Texture::GetType() const
{
    return Resource ? Resource->Type : KH_TEXTURE_TYPE::DEFAULT;
}

const std::string& KH_Texture::GetFileName() const
{
    static std::string Empty;
    return Resource ? Resource->FileName : Empty;
}

//void KH_Texture::Bind(KH_Shader& Shader, const std::string& Name, uint32_t Unit) const
//{
//    if (!IsValid())
//        return;
//
//    glActiveTexture(GL_TEXTURE0 + Unit);
//    glBindTexture(GL_TEXTURE_2D, Resource->ID);
//    Shader.SetInt(Name, static_cast<int>(Unit));
//}

void KH_Texture::Bind(uint32_t Unit) const
{
    if (!IsValid())
        return;

    glActiveTexture(GL_TEXTURE0 + Unit);
    glBindTexture(GL_TEXTURE_2D, Resource->ID);
}

void KH_Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

KH_Texture KH_TextureManager::LoadTexture(const std::string& filePath,
    bool generateMipmap,
    KH_TEXTURE_TYPE type,
    bool flipY)
{
    const std::string key = NormalizePath(filePath);

    auto it = TextureCache.find(key);
    if (it != TextureCache.end())
    {
        if (auto shared = it->second.lock())
        {
            return KH_Texture(shared);
        }
    }

    auto resource = CreateTextureResource(key, generateMipmap, type, flipY);
    if (!resource || resource->ID == 0)
    {
        return KH_Texture();
    }

    TextureCache[key] = resource;
    return KH_Texture(resource);
}

void KH_TextureManager::GarbageCollect()
{
    for (auto it = TextureCache.begin(); it != TextureCache.end();)
    {
        if (it->second.expired())
            it = TextureCache.erase(it);
        else
            ++it;
    }
}

void KH_TextureManager::Clear()
{
    TextureCache.clear();
}

std::string KH_TextureManager::NormalizePath(const std::string& path) const
{
    try
    {
        std::filesystem::path p(path);
        p = std::filesystem::weakly_canonical(p);
        return p.generic_string();
    }
    catch (...)
    {
        return std::filesystem::path(path).generic_string();
    }
}

std::shared_ptr<KH_TextureResource> KH_TextureManager::CreateTextureResource(const std::string& normalizedPath,
    bool generateMipmap,
    KH_TEXTURE_TYPE type,
    bool flipY)
{
    auto resource = std::make_shared<KH_TextureResource>();
    resource->FileName = normalizedPath;

    glGenTextures(1, &resource->ID);

    if (resource->ID == 0)
    {
        LOG_E(std::format("glGenTextures failed for path: {}", normalizedPath));
        return nullptr;
    }

    std::string extension;
    auto pos = normalizedPath.find_last_of('.');
    if (pos != std::string::npos)
        extension = normalizedPath.substr(pos + 1);

    if (extension == "hdr" || extension == "HDR")
    {
        resource->Type = KH_TEXTURE_TYPE::HDR;
        LoadHDRTexture(*resource, normalizedPath.c_str(), generateMipmap, flipY);
    }
    else
    {
        resource->Type = type;
        LoadStandardTexture(*resource, normalizedPath.c_str(), generateMipmap, flipY);
    }

    if (resource->ID == 0)
        return nullptr;

    return resource;
}

void KH_TextureManager::LoadStandardTexture(KH_TextureResource& resource,
    const char* path,
    bool generateMipmap,
    bool flipY) const
{
    stbi_set_flip_vertically_on_load(flipY);

    int width = 0;
    int height = 0;
    int nrComponents = 0;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (!data)
    {
        LOG_E(std::format("Texture failed to load at path: {}", path));
        if (resource.ID != 0)
        {
            glDeleteTextures(1, &resource.ID);
            resource.ID = 0;
        }
        return;
    }

    GLenum dataFormat = GL_RGB;
    GLenum internalFormat = GL_RGB8;

    if (nrComponents == 1)
    {
        dataFormat = GL_RED;
        internalFormat = GL_R8;
    }
    else if (nrComponents == 3)
    {
        dataFormat = GL_RGB;
        internalFormat = GL_RGB8;
    }
    else if (nrComponents == 4)
    {
        dataFormat = GL_RGBA;
        internalFormat = GL_RGBA8;
    }

    resource.Width = width;
    resource.Height = height;
    resource.Channels = nrComponents;

    glBindTexture(GL_TEXTURE_2D, resource.ID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (generateMipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}

void KH_TextureManager::LoadHDRTexture(KH_TextureResource& resource,
    const char* path,
    bool generateMipmap,
    bool flipY) const
{
    stbi_set_flip_vertically_on_load(flipY);

    int width = 0;
    int height = 0;
    int nrComponents = 0;
    float* data = stbi_loadf(path, &width, &height, &nrComponents, 0);

    if (!data)
    {
        LOG_E(std::format("HDR Texture failed to load at path: {}", path));
        if (resource.ID != 0)
        {
            glDeleteTextures(1, &resource.ID);
            resource.ID = 0;
        }
        return;
    }

    resource.Width = width;
    resource.Height = height;
    resource.Channels = nrComponents;

    glBindTexture(GL_TEXTURE_2D, resource.ID);

    GLenum dataFormat = (nrComponents == 4) ? GL_RGBA : GL_RGB;
    GLenum internalFormat = (nrComponents == 4) ? GL_RGBA16F : GL_RGB16F;

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_FLOAT, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (generateMipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}

KH_ExampleTextures::KH_ExampleTextures()
{
    InitTextures();
}

void KH_ExampleTextures::InitTextures()
{
    FirePlaceHDR = KH_TextureManager::Instance().LoadTexture(
        "Assert/Images/HDR/fireplace_4k.hdr",
        false,
        KH_TEXTURE_TYPE::HDR,
        true
    );
}

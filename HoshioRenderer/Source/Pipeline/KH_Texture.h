#pragma once

#include "KH_Common.h"

class KH_Shader;

enum class KH_TEXTURE_TYPE
{
    DEFAULT = 0,
    DIFFUSE,
    SPECULAR,
    NORMAL,
    HEIGHT,
    HDR
};

struct KH_TextureResource
{
    unsigned int ID = 0;
    KH_TEXTURE_TYPE Type = KH_TEXTURE_TYPE::DEFAULT;
    std::string FileName;

    int Width = 0;
    int Height = 0;
    int Channels = 0;

    ~KH_TextureResource();
};

class KH_Texture
{
public:
    KH_Texture() = default;
    explicit KH_Texture(std::shared_ptr<KH_TextureResource> resource);

    KH_Texture(const KH_Texture&) = default;
    KH_Texture& operator=(const KH_Texture&) = default;
    KH_Texture(KH_Texture&&) noexcept = default;
    KH_Texture& operator=(KH_Texture&&) noexcept = default;
    ~KH_Texture() = default;

    bool IsValid() const;
    unsigned int GetID() const;
    KH_TEXTURE_TYPE GetType() const;
    const std::string& GetFileName() const;

    void Bind(KH_Shader& Shader, const std::string& Name, uint32_t Unit = 0) const;
    void Bind(uint32_t Unit = 0) const;
    void Unbind() const;

private:
    std::shared_ptr<KH_TextureResource> Resource;
};

class KH_TextureManager : public KH_Singleton<KH_TextureManager>
{
    friend class KH_Singleton<KH_TextureManager>;
public:
    KH_Texture LoadTexture(const std::string& filePath,
        bool generateMipmap = true,
        KH_TEXTURE_TYPE type = KH_TEXTURE_TYPE::DEFAULT,
        bool flipY = true);

    void GarbageCollect();
    void Clear();

private:
    KH_TextureManager() = default;
    ~KH_TextureManager() override = default;

    KH_TextureManager(const KH_TextureManager&) = delete;
    KH_TextureManager& operator=(const KH_TextureManager&) = delete;

private:
    std::unordered_map<std::string, std::weak_ptr<KH_TextureResource>> TextureCache;

private:
    std::string NormalizePath(const std::string& path) const;

    std::shared_ptr<KH_TextureResource> CreateTextureResource(const std::string& normalizedPath,
        bool generateMipmap,
        KH_TEXTURE_TYPE type,
        bool flipY);

    void LoadStandardTexture(KH_TextureResource& resource,
        const char* path,
        bool generateMipmap,
        bool flipY) const;

    void LoadHDRTexture(KH_TextureResource& resource,
        const char* path,
        bool generateMipmap,
        bool flipY) const;
};

class KH_ExampleTextures : public KH_Singleton<KH_ExampleTextures>
{
    friend class KH_Singleton<KH_ExampleTextures>;
private:
    KH_ExampleTextures();
    ~KH_ExampleTextures() override = default;

    void InitTextures();
   
public:
    KH_Texture FirePlaceHDR;

};

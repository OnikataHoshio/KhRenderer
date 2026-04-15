#pragma once
#include "KH_Shader.h"

enum class KH_ShaderFeatureType : uint8_t
{
    DisneyBRDF = 0,
    BSSRDF,
    Count
};

constexpr size_t KH_ShaderFeatureTypeCount =
static_cast<size_t>(KH_ShaderFeatureType::Count);

inline size_t KH_ShaderFeatureTypeToIndex(KH_ShaderFeatureType type)
{
    return static_cast<size_t>(type);
}

class KH_ShaderFeatureBase
{
public:
    explicit KH_ShaderFeatureBase(
        KH_ShaderFeatureType type = KH_ShaderFeatureType::DisneyBRDF);

    KH_ShaderFeatureBase(
        KH_ShaderFeatureType type,
        const KH_Shader& shader);

    virtual ~KH_ShaderFeatureBase() = default;

    virtual void DrawControlPanel() = 0;
    virtual void ApplyUniforms() = 0;

    // 每个 ShaderFeature 自己负责自己的材质缓存
    virtual void UploadMaterialBuffer() = 0;
    virtual void BindBuffers() = 0;
    virtual void ClearMaterials() = 0;
    virtual int GetMaterialCount() const = 0;
    virtual bool DeleteMaterial(int materialID) = 0;

    virtual void Use();

    KH_ShaderFeatureType GetType() const;

    bool IsEnabled() const;
    void SetEnabled(bool enabled);

    void SetShader(const KH_Shader& shader);

    KH_Shader& GetShader() { return Shader; }
    const KH_Shader& GetShader() const { return Shader; }

protected:
    KH_ShaderFeatureType Type;
    KH_Shader Shader;
    bool bEnabled = true;
};
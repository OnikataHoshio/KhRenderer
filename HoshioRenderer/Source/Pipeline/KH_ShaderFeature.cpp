#include "KH_ShaderFeature.h"

#include "KH_ShaderFeature.h"

KH_ShaderFeatureBase::KH_ShaderFeatureBase(KH_ShaderFeatureType type)
    : Type(type)
{
}

KH_ShaderFeatureBase::KH_ShaderFeatureBase(
    KH_ShaderFeatureType type,
    const KH_Shader& shader)
    : Type(type), Shader(shader)
{
}

void KH_ShaderFeatureBase::Use()
{
    if (!bEnabled || !Shader.IsValid())
        return;

    Shader.Use();
    BindBuffers();
    ApplyUniforms();
}

KH_ShaderFeatureType KH_ShaderFeatureBase::GetType() const
{
    return Type;
}

bool KH_ShaderFeatureBase::IsEnabled() const
{
    return bEnabled;
}

void KH_ShaderFeatureBase::SetEnabled(bool enabled)
{
    bEnabled = enabled;
}

void KH_ShaderFeatureBase::SetShader(const KH_Shader& shader)
{
    Shader = shader;
}
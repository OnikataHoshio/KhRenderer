#include "KH_ShaderFeature.h"

KH_ShaderFeatureBase::KH_ShaderFeatureBase(const KH_Shader& shader)
	:Shader(shader)
{
}

void KH_ShaderFeatureBase::Use()
{
    if (!Shader.IsValid())
        return;

    Shader.Use();
    ApplyUniforms();
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

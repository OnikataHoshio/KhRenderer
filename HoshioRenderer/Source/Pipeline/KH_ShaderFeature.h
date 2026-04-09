#pragma once
#include "KH_Shader.h"

class KH_ShaderFeatureBase
{
public:
    KH_ShaderFeatureBase() = default;
    KH_ShaderFeatureBase(const KH_Shader& shader);

    virtual ~KH_ShaderFeatureBase() = default;

    virtual void DrawControlPanel() = 0;
    virtual void ApplyUniforms() = 0;

    virtual void Use();

    bool IsEnabled() const;
    void SetEnabled(bool enabled);

    void SetShader(const KH_Shader& shader);

    KH_Shader& GetShader() { return Shader; }
    const KH_Shader& GetShader() const { return Shader; }

protected:
    KH_Shader Shader;
    bool bEnabled = true;
};
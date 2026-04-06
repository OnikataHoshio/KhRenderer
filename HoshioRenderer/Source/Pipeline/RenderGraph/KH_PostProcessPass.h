#pragma once

#include "KH_RenderPass.h"
#include "Pipeline/KH_Shader.h"

class KH_Shader;
class KH_Framebuffer;

class KH_PostProcessPass : public KH_RenderPass
{
public:
    KH_PostProcessPass(const std::string& name, KH_Shader shader);
    virtual ~KH_PostProcessPass() override = default;

    const std::string& GetName() const override;
    bool IsEnabled() const override;
    void SetEnabled(bool enabled);

    virtual void Execute(KH_Framebuffer& Input, KH_Framebuffer& Output);

protected:
    virtual void RenderFullscreenQuad();

protected:
    std::string Name;
    KH_Shader Shader;
    bool Enabled = true;
};
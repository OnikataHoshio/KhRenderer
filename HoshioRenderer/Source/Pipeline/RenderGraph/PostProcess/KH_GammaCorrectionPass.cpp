#include "KH_GammaCorrectionPass.h"
#include "Pipeline/KH_Framebuffer.h"


KH_GammaCorrectionPass::KH_GammaCorrectionPass(const std::string& name, KH_Shader shader, float gamma)
	:KH_PostProcessPass(name, shader), Gamma(gamma)
{
}

void KH_GammaCorrectionPass::Execute(KH_Framebuffer& Input, KH_Framebuffer& Output)
{
    Output.Bind();

    Shader.Use();
    Shader.SetInt("uTexture", 0);
    Shader.SetFloat("uGamma", Gamma);

    Input.BindColorAttachment(0, 0);

    RenderFullscreenQuad();

    Output.Unbind();
}

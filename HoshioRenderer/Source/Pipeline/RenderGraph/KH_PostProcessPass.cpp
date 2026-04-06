#include "KH_PostProcessPass.h"
#include "Pipeline/KH_Shader.h"
#include "Pipeline/KH_Framebuffer.h"

#include "Scene/KH_Model.h"

KH_PostProcessPass::KH_PostProcessPass(const std::string& name, KH_Shader shader)
	:Name(name), Shader(shader)
{
}

const std::string& KH_PostProcessPass::GetName() const
{
	return Name;
}

bool KH_PostProcessPass::IsEnabled() const
{
	return Enabled;
}

void KH_PostProcessPass::SetEnabled(bool enabled)
{
	Enabled = enabled;
}

void KH_PostProcessPass::Execute(KH_Framebuffer& Input, KH_Framebuffer& Output)
{
    Output.Bind();
    glClear(GL_COLOR_BUFFER_BIT);

    Shader.Use();
    Shader.SetInt("uTexture", 0);

    Input.BindColorAttachment(0, 0);

    RenderFullscreenQuad();

    Output.Unbind();
}

void KH_PostProcessPass::RenderFullscreenQuad()
{
    glBindVertexArray(KH_DefaultModels::Instance().FullscreenQuad.GetVAO());
    glDrawElements(GL_TRIANGLES, KH_DefaultModels::Instance().FullscreenQuad.GetNumIndices(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

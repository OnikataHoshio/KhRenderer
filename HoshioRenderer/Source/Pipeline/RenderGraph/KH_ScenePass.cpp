#include "KH_ScenePass.h"
#include "Pipeline/KH_Shader.h"
#include "Pipeline/KH_Framebuffer.h"
#include "Scene/KH_Model.h"

KH_ScenePass::KH_ScenePass(const std::string& name, KH_Shader shader)
	:Name(name), Shader(shader)
{
}

const std::string& KH_ScenePass::GetName() const
{
	return Name;
}

bool KH_ScenePass::IsEnabled() const
{
	return Enabled;
}

void KH_ScenePass::SetEnabled(bool enabled)
{
	Enabled = enabled;
}

void KH_ScenePass::RenderFullscreenQuad()
{
	glBindVertexArray(KH_DefaultModels::Instance().FullscreenQuad.GetVAO());
	glDrawElements(GL_TRIANGLES, KH_DefaultModels::Instance().FullscreenQuad.GetNumIndices(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

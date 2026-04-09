#include "KH_DrawSobolPass.h"
#include "Pipeline/KH_Shader.h"
#include "Pipeline/KH_Framebuffer.h"
#include "Scene/KH_Model.h"

#include "Editor/KH_Editor.h"

KH_DrawSobolPass::KH_DrawSobolPass(const std::string& name, KH_Shader shader)
	: KH_ScenePass(name, shader)
{
}

void KH_DrawSobolPass::Execute()
{
	KH_Editor& Editor = KH_Editor::Instance();

	Editor.BindCanvasFramebuffer();
	Shader.Use();

	Shader.SetUint("uSamplesPerFrame", 4u);
	Shader.SetUint("uFrameCounter", KH_Editor::Instance().GetFrameCounter());
	Shader.SetUvec2("uResolution", glm::uvec2(KH_Editor::GetCanvasWidth(), KH_Editor::GetCanvasHeight()));

	KH_Editor::Instance().GetLastFramebuffer().BindColorAttachment(0, 0);
	Shader.SetInt("uLastFrame", 0);

	RenderFullscreenQuad();

	Editor.UnbindCanvasFramebuffer();
}

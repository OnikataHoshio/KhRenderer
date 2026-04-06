#include "Editor/KH_Editor.h"
#include "Pipeline/KH_Shader.h"
#include "Scene/KH_Scene.h"
#include "Pipeline/RenderGraph/KH_PostProcessGraph.h"

int main()
{
	KH_Editor::SetEditorWidth(1280);
	KH_Editor::SetEditorHeight(920);
	KH_Editor::SetTitle("KH_Renderer");
	KH_Editor::Instance();

	KH_Model& Bunny = KH_DefaultModels::Instance().Bunny;
	KH_GpuLBVHScene& GpuLBVHBunnyScene = KH_GpuLBVHExampleScenes::Instance().Bunny;
	KH_GpuLBVHScene& GpuLBVHDebugBoxScene = KH_GpuLBVHExampleScenes::Instance().DebugBox;
	KH_Shader& TestShader = KH_ExampleShaders::Instance().TestShader;
	KH_Shader& AABBShader = KH_ExampleShaders::Instance().AABBShader;

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	while (!glfwWindowShouldClose(KH_Editor::Instance().GLFWwindow()))
	{
		KH_Editor::Instance().BeginRender();

		KH_Editor::Instance().BindCanvasFramebuffer();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Bunny.Render(TestShader);

		//GpuLBVHDebugBoxScene.Render();
		GpuLBVHBunnyScene.Render();

		KH_Editor::Instance().UnbindCanvasFramebuffer();

		KH_PostProcessHelper::Instance().SingleGammaCorrectionGraph.Execute();

		KH_Editor::Instance().EndRender();
	}

	return 0;
}

#include "Editor/KH_Editor.h"
#include "Pipeline/KH_Shader.h"
#include "Hit/KH_LBVH.h"
#include "Renderer/KH_Renderer.h"
#include "Scene/KH_Scene.h"
#include "Utils/KH_DebugUtils.h"

int main()
{
	KH_Editor::SetEditorWidth(1280);
	KH_Editor::SetEditorHeight(920);
	KH_Editor::SetTitle("KH_Renderer");
	KH_Editor::Instance();

	KH_Model& Bunny = KH_DefaultModels::Instance().Bunny;
	KH_FlatBVHScene& FlatBVHBunnyScene = KH_FlatBVHExampleScenes::Instance().Bunny;
	KH_FlatBVHScene& FlatBVHSingleTriangleScene = KH_FlatBVHExampleScenes::Instance().SingleTriangle;
	KH_FlatBVHScene& FlatBVHDebugBoxScene = KH_FlatBVHExampleScenes::Instance().DebugBox;
	KH_LBVHScene& LBVHBunnyScene = KH_LBVHExampleScenes::Instance().Bunny;
	KH_LBVHScene& LBVHSingleTriangleScene = KH_LBVHExampleScenes::Instance().SingleTriangle;
	KH_LBVHScene& LBVHDebugBoxScene = KH_LBVHExampleScenes::Instance().DebugBox;
	KH_GpuLBVHScene& GpuLBVHBunnyScene = KH_GpuLBVHExampleScenes::Instance().Bunny;
	KH_GpuLBVHScene& GpuLBVHDebugBoxScene = KH_GpuLBVHExampleScenes::Instance().DebugBox;
	KH_Shader& TestShader = KH_ExampleShaders::Instance().TestShader;
	KH_Shader& AABBShader = KH_ExampleShaders::Instance().AABBShader;

	//KH_RendererBase BaseRenderer;
	//BaseRenderer.TraversalMode = KH_PRIMITIVE_TRAVERSAL_MODE::BASE_BVH;
	//BaseRenderer.Render(KH_ExampleScenes::Instance().ExampleScene1);

	//GpuLBVHBunnyScene.BVH.CheckAllData(LBVHBunnyScene.BVH);
	//GpuLBVHDebugBoxScene.BVH.CheckAllData(LBVHDebugBoxScene.BVH);

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	while (!glfwWindowShouldClose(KH_Editor::Instance().GLFWwindow()))
	{
		KH_Editor::Instance().BeginRender();

		KH_Editor::Instance().BindCanvasFramebuffer();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//KH_ShaderHelper::SetupTestShader(TestShader, glm::vec3(1.0f, 1.0f, 1.0f));
		//Bunny.Render(TestShader);

		//FlatBVHBunnyScene.BVH.RenderAABB(AABBShader, glm::vec3(1.0f, 1.0f, 1.0f));

		//FlatBVHDebugBoxScene.BVH.RenderAABB(AABBShader, glm::vec3(1.0f, 1.0f, 1.0f));
		//LBVHBunnyScene.BVH.RenderAABB(AABBShader, glm::vec3(1.0f, 1.0f, 1.0f));
		//GpuLBVHBunnyScene.BVH.RenderAABB(AABBShader, glm::vec3(1.0f, 1.0f, 1.0f));
		//GpuLBVHDebugBoxScene.BVH.RenderAABB(AABBShader, glm::vec3(1.0f, 1.0f, 1.0f));

		//FlatBVHSingleTriangleScene.Render();
		//FlatBVHBunnyScene.Render();
		//FlatBVHDebugBoxScene.Render();
		//LBVHSingleTriangleScene.Render();
		//LBVHBunnyScene.Render();
		//LBVHDebugBoxScene.Render();

		//GpuLBVHDebugBoxScene.Render();
		GpuLBVHBunnyScene.Render();

		KH_Editor::Instance().UnbindCanvasFramebuffer();
		KH_Editor::Instance().EndRender();
	}

	return 0;
}

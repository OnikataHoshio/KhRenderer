#include "Editor/KH_Editor.h"
#include "Pipeline/KH_Shader.h"
#include "Scene/KH_Object.h"
#include "Hit/KH_BVH.h"
#include "Renderer/KH_Renderer.h"
#include "Scene/KH_Scene.h"
#include "Utils/KH_DebugUtils.h"

#include "Scene/KH_Scene.h"

int main()
{
	KH_Editor::EditorWidth = 1920;
	KH_Editor::EditorHeight = 1080;
	KH_Editor::Title = "KH_Renderer";
	KH_Editor::Instance();

	//KH_RendererBase BaseRenderer;
	//BaseRenderer.TraversalMode = KH_PRIMITIVE_TRAVERSAL_MODE::BASE_BVH;
	//BaseRenderer.Render(KH_ExampleScenes::Instance().ExampleScene1);

	while (!glfwWindowShouldClose(KH_Editor::Instance().GLFWwindow()))
	{
		KH_Editor::Instance().BeginRender();

		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


		//KH_DefaultModels::Get().Cube.Render(TestShader);
		//KH_DefaultModels::Get().EmptyCube.Render(TestShader);
		//KH_DefaultModels::Get().Plane.Render(TestShader);

		//Bunny.Render(TestShader);
		//KH_ExampleScene::Instance().ExampleScene1.BVH.RenderAABB(AABBShader, glm::vec3(1.0f, 1.0f, 1.0f));

		KH_ExampleScenes::Instance().ExampleScene1.Render();

		{
			ImGui::Begin("Global Info");
			{
				ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
				ImGui::Separator();
			}
			ImGui::End();
		}


		KH_Editor::Instance().EndRender();
	}

	return 0;
}
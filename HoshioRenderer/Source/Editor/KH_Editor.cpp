#include "KH_Editor.h"
uint32_t KH_Editor::EditorWidth = 1920;
uint32_t KH_Editor::EditorHeight = 1080;
uint32_t KH_Editor::CanvasWidth = 800;
uint32_t KH_Editor::CanvasHeight = 800;
std::string KH_Editor::Title = "KH_Renderer";

KH_Editor& KH_Editor::Instance()
{
	static KH_Editor instance; 
	return instance;
}

GLFWwindow* KH_Editor::GLFWwindow()
{
	return Window.Window;
}

void KH_Editor::BeginRender()
{
	Window.BeginRender();
    BeginImgui();
    RenderDockSpace();
}

void KH_Editor::EndRender()
{
    RenderView.Render();
    Console.Render();
    Setting.Render();
    GlobalInfo.Render();
    EndImgui();
	Window.EndRender();
}

void KH_Editor::UpdateCanvasExtent(uint32_t Width, uint32_t Height)
{
    CanvasWidth = Width;
    CanvasHeight = Height;

    Camera.Width = Width;
    Camera.Height = Height;
    Camera.UpdateAspect();
}

KH_Framebuffer& KH_Editor::GetCanvasFramebuffer()
{
    return RenderView.Framebuffer;
}

KH_Editor::KH_Editor()
	:Camera(CanvasWidth, CanvasHeight), Window(EditorWidth, EditorHeight, Title)
{
	Window.SetCamera(&Camera);

	Initialize();
}

KH_Editor::~KH_Editor()
{
	DeInitialize();
}

void KH_Editor::Initialize()
{

}

void KH_Editor::DeInitialize()
{
	//TODO
}

void KH_Editor::RenderDockSpace()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("MyRootDockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    ImGui::End();
}

void KH_Editor::BeginImgui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void KH_Editor::EndImgui()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}



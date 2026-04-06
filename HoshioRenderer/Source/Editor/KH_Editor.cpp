#include "KH_Editor.h"
#include "Hit/KH_Ray.h"

#include "Scene/KH_Scene.h"

uint32_t KH_Editor::EditorWidth = 1920;
uint32_t KH_Editor::EditorHeight = 1080;
uint32_t KH_Editor::CanvasWidth = 800;
uint32_t KH_Editor::CanvasHeight = 800;
std::string KH_Editor::Title = "KH_Renderer";

namespace
{
    ImGuizmo::OPERATION ToImGuizmoOperation(KH_GizmoOperation op)
    {
        switch (op)
        {
        case KH_GizmoOperation::Translate: return ImGuizmo::TRANSLATE;
        case KH_GizmoOperation::Rotate:    return ImGuizmo::ROTATE;
        case KH_GizmoOperation::Scale:     return ImGuizmo::SCALE;
        default:                           return ImGuizmo::TRANSLATE;
        }
    }

    ImGuizmo::MODE ToImGuizmoMode(KH_GizmoMode mode)
    {
        switch (mode)
        {
        case KH_GizmoMode::Local: return ImGuizmo::LOCAL;
        case KH_GizmoMode::World: return ImGuizmo::WORLD;
        default:                  return ImGuizmo::LOCAL;
        }
    }

    bool ExtractTRS(const glm::mat4& m, glm::vec3& outPosition, glm::quat& outRotation, glm::vec3& outScale)
    {
        outPosition = glm::vec3(m[3]);

        const glm::vec3 col0 = glm::vec3(m[0]);
        const glm::vec3 col1 = glm::vec3(m[1]);
        const glm::vec3 col2 = glm::vec3(m[2]);

        outScale.x = glm::length(col0);
        outScale.y = glm::length(col1);
        outScale.z = glm::length(col2);

        if (outScale.x <= 1e-6f || outScale.y <= 1e-6f || outScale.z <= 1e-6f)
            return false;

        glm::mat3 rot;
        rot[0] = col0 / outScale.x;
        rot[1] = col1 / outScale.y;
        rot[2] = col2 / outScale.z;

        outRotation = glm::normalize(glm::quat_cast(rot));
        return true;
    }
}

KH_Editor& KH_Editor::Instance()
{
    static KH_Editor instance;
    return instance;
}

uint32_t KH_Editor::GetFrameCounter() const
{
    return FrameCounter;
}

void KH_Editor::RequestSceneRebuild()
{
    bSceneRebuildRequested = true;
}

KH_Canvas& KH_Editor::GetCanvas()
{
    return Canvas;
}

void KH_Editor::SetEditorWidth(uint32_t Width)
{
    EditorWidth = Width;
}

void KH_Editor::SetEditorHeight(uint32_t Height)
{
    EditorHeight = Height;
}

void KH_Editor::SetCanvasWidth(uint32_t Width)
{
    CanvasWidth = Width;
}

void KH_Editor::SetCanvasHeight(uint32_t Height)
{
    CanvasHeight = Height;
}

void KH_Editor::SetTitle(std::string Title)
{
    KH_Editor::Title = Title;
}

uint32_t KH_Editor::GetEditorWidth()
{
    return EditorWidth;
}

uint32_t KH_Editor::GetEdtiorHeight()
{
    return EditorHeight;
}

uint32_t KH_Editor::GetCanvasWidth()
{
    return CanvasWidth;
}

uint32_t KH_Editor::GetCanvasHeight()
{
    return CanvasHeight;
}

const std::string& KH_Editor::GetTitle()
{
    return Title;
}

GLFWwindow* KH_Editor::GLFWwindow() const
{
    return Window.Window;
}

void KH_Editor::BeginRender()
{
    bSceneRebuildRequested = false;
    Window.BeginRender();
    BeginImgui();
    RenderDockSpace();
}

void KH_Editor::EndRender()
{
    Canvas.Render();
    UpdateSelectedObjectIndex();

    Console.Render();
    Inspector.Render();
    GlobalInfo.Render();
    Canvas.SwapFramebuffer();
    EndImgui();
    Window.EndRender();

    FrameCounter += 1;

    if (bSceneRebuildRequested)
    {
        Scene->BindAndBuild();
        ResetFrameCounter();
    }

    if (bViewManipulatorUsing)
    {
        ResetFrameCounter();
    }
}

void KH_Editor::UpdateCanvasExtent(uint32_t Width, uint32_t Height)
{
    CanvasWidth = Width;
    CanvasHeight = Height;

    Camera.Width = Width;
    Camera.Height = Height;
    Camera.UpdateAspect();

    ResetFrameCounter();
}

void KH_Editor::BindCanvasFramebuffer()
{
    Canvas.BindSceneFramebuffer();
}

void KH_Editor::UnbindCanvasFramebuffer()
{
    Canvas.UnbindSceneFramebuffer();
}

const KH_Framebuffer& KH_Editor::GetLastFramebuffer()
{
    return Canvas.GetLastFramebuffer();
}

int32_t KH_Editor::GetSelectedObjectIndex() const
{
    return SelectedObjectIndex;
}

void KH_Editor::ResetFrameCounter()
{
    FrameCounter = 0;
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

    ImGui::Begin("RootDockSpace", nullptr, window_flags);
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
    ImGuizmo::BeginFrame();
}

void KH_Editor::EndImgui()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void KH_Editor::UpdateSelectedObjectIndex()
{
    if (!Scene)
        return;

    if (ImGuizmo::IsUsingAny() || ImGuizmo::IsViewManipulateHovered())
        return;

    KH_Ray pickRay;
    if (Canvas.TryBuildPickRay(Camera, pickRay, 0))
    {
        KH_PickResult result = Scene->Pick(pickRay);
        SelectedObjectIndex = result.bIsHit ? result.ObjectIndex : -1;
    }
}

void KH_Editor::DrawCanvasGizmos()
{
    if (!Scene)
        return;

    if (!Canvas.IsHovered() && !Canvas.IsFocused())
        return;

    const glm::vec2& min = Canvas.GetCanvasMin();
    const glm::vec2& size = Canvas.GetCanvasSize();
    if (size.x <= 0.0f || size.y <= 0.0f)
        return;

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(min.x, min.y, size.x, size.y);

    DrawViewManipulator();
    DrawObjectGizmo();
}

glm::vec3 KH_Editor::GetViewManipulatorPivot() const
{
    if (!Scene)
        return glm::vec3(0.0f);

    glm::vec3 pivot = Scene->AABB.GetCenter();

    const auto& objects = Scene->GetObjects();
    if (SelectedObjectIndex >= 0 &&
        SelectedObjectIndex < static_cast<int32_t>(objects.size()) &&
        objects[SelectedObjectIndex].Object)
    {
        pivot = objects[SelectedObjectIndex].Object->GetAABB().GetCenter();
    }

    return pivot;
}

void KH_Editor::DrawViewManipulator()
{
    glm::mat4 view = Camera.GetViewMatrix();
    glm::mat4 oldView = view;

    const glm::vec2& min = Canvas.GetCanvasMin();
    const glm::vec2& size = Canvas.GetCanvasSize();

    ImVec2 widgetPos(
        min.x + size.x - ViewManipulatorSize - ViewManipulatorMargin,
        min.y + ViewManipulatorMargin
    );
    ImVec2 widgetSize(ViewManipulatorSize, ViewManipulatorSize);

    glm::vec3 pivot = GetViewManipulatorPivot();
    float distance = glm::length(Camera.Position - pivot);
    distance = std::max(distance, 0.001f);

    ImGuizmo::PushID("ViewManipulator");
    ImGuizmo::ViewManipulate(
        glm::value_ptr(view),
        distance,
        widgetPos,
        widgetSize,
        ViewManipulatorBgColor
    );
    bViewManipulatorHovered = ImGuizmo::IsViewManipulateHovered();
    bViewManipulatorUsing = ImGuizmo::IsUsingViewManipulate();
    ImGuizmo::PopID();

    bool viewChanged = std::memcmp(glm::value_ptr(oldView), glm::value_ptr(view), sizeof(float) * 16) != 0;
    if (viewChanged)
    {
        ApplyViewManipulatorToCamera(view, pivot, distance);
        ResetFrameCounter();
    }
}

void KH_Editor::ApplyViewManipulatorToCamera(const glm::mat4& manipulatedView, const glm::vec3& pivot, float distance)
{
    glm::mat4 invView = glm::inverse(manipulatedView);

    glm::vec3 right = glm::normalize(glm::vec3(invView[0]));
    glm::vec3 up = glm::normalize(glm::vec3(invView[1]));
    glm::vec3 back = glm::normalize(glm::vec3(invView[2]));
    glm::vec3 front = -back;

    Camera.Right = right;
    Camera.Up = up;
    Camera.Front = glm::normalize(front);

    Camera.Position = pivot - Camera.Front * distance;

    Camera.Pitch = glm::degrees(std::asin(glm::clamp(Camera.Front.y, -1.0f, 1.0f)));
    Camera.Yaw = glm::degrees(std::atan2(Camera.Front.z, Camera.Front.x));
}

void KH_Editor::DrawObjectGizmo()
{
    bGizmoOver = false;
    bGizmoUsing = false;

    const auto& objects = Scene->GetObjects();
    if (SelectedObjectIndex < 0 || SelectedObjectIndex >= static_cast<int32_t>(objects.size()))
        return;

    KH_Object* object = objects[SelectedObjectIndex].Object.get();
    if (!object)
        return;

    if (ImGui::IsKeyPressed(ImGuiKey_G)) GizmoOperation = KH_GizmoOperation::Translate;
    if (ImGui::IsKeyPressed(ImGuiKey_R)) GizmoOperation = KH_GizmoOperation::Rotate;
    if (ImGui::IsKeyPressed(ImGuiKey_Y)) GizmoOperation = KH_GizmoOperation::Scale;
    if (ImGui::IsKeyPressed(ImGuiKey_L)) GizmoMode = KH_GizmoMode::Local;
    if (ImGui::IsKeyPressed(ImGuiKey_W)) GizmoMode = KH_GizmoMode::World;
    if (ImGui::IsKeyPressed(ImGuiKey_S)) bUseGizmoSnap = !bUseGizmoSnap;

    glm::mat4 view = Camera.GetViewMatrix();
    glm::mat4 proj = Camera.GetProjMatrix();
    glm::mat4 gizmoMatrix = object->GetGizmoMatrix();

    float translateSnap[3] = { TranslateSnap.x, TranslateSnap.y, TranslateSnap.z };
    float rotateSnap = RotateSnap;
    float scaleSnap = ScaleSnap;

    const float* snap = nullptr;
    switch (GizmoOperation)
    {
    case KH_GizmoOperation::Translate:
        snap = bUseGizmoSnap ? translateSnap : nullptr;
        break;
    case KH_GizmoOperation::Rotate:
        snap = bUseGizmoSnap ? &rotateSnap : nullptr;
        break;
    case KH_GizmoOperation::Scale:
        snap = bUseGizmoSnap ? &scaleSnap : nullptr;
        break;
    }

    ImGuizmo::PushID("ObjectGizmo");
    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        ToImGuizmoOperation(GizmoOperation),
        ToImGuizmoMode(GizmoMode),
        glm::value_ptr(gizmoMatrix),
        nullptr,
        snap
    );
    bGizmoOver = ImGuizmo::IsOver();
    bGizmoUsing = ImGuizmo::IsUsing();
    ImGuizmo::PopID();

    if (bGizmoUsing)
    {
        const glm::vec3& pivotLocal = object->GetGizmoPivotLocal();

        glm::mat4 newModel =
            gizmoMatrix *
            glm::inverse(glm::translate(glm::mat4(1.0f), pivotLocal));

        glm::vec3 position;
        glm::vec3 scale;
        glm::quat rotationQuat;

        if (ExtractTRS(newModel, position, rotationQuat, scale))
        {
            object->SetTransform(position, rotationQuat, scale);
            RequestSceneRebuild();
            ResetFrameCounter();
        }
    }
}
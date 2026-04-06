#pragma once
#include "KH_Camera.h"
#include "KH_Window.h"
#include "KH_Canvas.h"

class KH_SceneBase;

enum class KH_GizmoOperation
{
    Translate,
    Rotate,
    Scale
};

enum class KH_GizmoMode
{
    Local,
    World
};

class KH_Editor
{
public:
    static KH_Editor& Instance();

    KH_Editor(const KH_Editor&) = delete;
    KH_Editor& operator=(const KH_Editor&) = delete;

    void BeginRender();
    void EndRender();

    void UpdateCanvasExtent(uint32_t Width, uint32_t Height);
    void BindCanvasFramebuffer();
    void UnbindCanvasFramebuffer();

    const KH_Framebuffer& GetLastFramebuffer();

    int32_t GetSelectedObjectIndex() const;

    void ResetFrameCounter();
    uint32_t GetFrameCounter() const;

    void RequestSceneRebuild();

    KH_Canvas& GetCanvas();

    static void SetEditorWidth(uint32_t Width);
    static void SetEditorHeight(uint32_t Height);
    static void SetCanvasWidth(uint32_t Width);
    static void SetCanvasHeight(uint32_t Height);
    static void SetTitle(std::string Title);

    static uint32_t GetEditorWidth();
    static uint32_t GetEdtiorHeight();
    static uint32_t GetCanvasWidth();
    static uint32_t GetCanvasHeight();
    static const std::string& GetTitle();

    GLFWwindow* GLFWwindow() const;

    // 在 Canvas 窗口内部调用
    void DrawCanvasGizmos();

    KH_Camera Camera;
    KH_Window Window;
    KH_SceneBase* Scene = nullptr;

private:
    static uint32_t EditorWidth;
    static uint32_t EditorHeight;
    static uint32_t CanvasWidth;
    static uint32_t CanvasHeight;
    static std::string Title;

    uint32_t FrameCounter = 0;
    int32_t SelectedObjectIndex = -1;

    bool bSceneRebuildRequested = false;

    bool bGizmoOver = false;
    bool bGizmoUsing = false;

    bool bViewManipulatorHovered = false;
    bool bViewManipulatorUsing = false;

    KH_GizmoOperation GizmoOperation = KH_GizmoOperation::Translate;
    KH_GizmoMode GizmoMode = KH_GizmoMode::Local;

    bool bUseGizmoSnap = false;
    glm::vec3 TranslateSnap = glm::vec3(0.1f);
    float RotateSnap = 15.0f;
    float ScaleSnap = 0.1f;

    float ViewManipulatorSize = 96.0f;
    float ViewManipulatorMargin = 12.0f;
    ImU32 ViewManipulatorBgColor = 0x10101010;

    KH_Canvas Canvas;
    KH_Console Console;
    KH_Insepctor Inspector;
    KH_GlobalInfo GlobalInfo;

    KH_Editor();
    ~KH_Editor();

    void Initialize();
    void DeInitialize();

    void RenderDockSpace();

    void BeginImgui();
    void EndImgui();

    void UpdateSelectedObjectIndex();

    void DrawObjectGizmo();
    void DrawViewManipulator();
    void ApplyViewManipulatorToCamera(const glm::mat4& manipulatedView, const glm::vec3& pivot, float distance);
    glm::vec3 GetViewManipulatorPivot() const;
};




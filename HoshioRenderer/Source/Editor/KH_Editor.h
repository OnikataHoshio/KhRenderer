#pragma once
#include "KH_Camera.h"
#include "KH_Window.h"
#include "KH_Panel.h"

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

    void ResetFrameCounter();
    uint32_t GetFrameCounter() const;

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

    KH_Camera Camera;
    KH_Window Window;

private:
    static uint32_t EditorWidth;
    static uint32_t EditorHeight;
    static uint32_t CanvasWidth;
    static uint32_t CanvasHeight;
    static std::string Title;

    uint32_t FrameCounter = 0;

    KH_Canvas RenderView;
    KH_Console Console;
    KH_Setting Setting;
    KH_GlobalInfo GlobalInfo;

    KH_Editor();  
    ~KH_Editor();

    void Initialize();
    void DeInitialize();

    void RenderDockSpace();

    void BeginImgui();
    void EndImgui();


};




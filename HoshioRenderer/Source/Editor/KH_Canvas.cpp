#include "KH_Canvas.h"
#include "Utils/KH_DebugUtils.h"
#include "KH_Editor.h"

#include "Hit/KH_Ray.h"

KH_Canvas::KH_Canvas()
    :Timer(3.0f)
{
    KH_FramebufferDescription sceneDesc;
    sceneDesc.Width = 64;
    sceneDesc.Height = 64;
    sceneDesc.Attachments = {
        KH_FramebufferTextureFormat::RGBA16F,      // Scene Color
        KH_FramebufferTextureFormat::DEPTH32F      // Depth
    };

    SceneFramebuffer[0].Create(sceneDesc);
    SceneFramebuffer[1].Create(sceneDesc);


    sceneDesc.Attachments = {
        KH_FramebufferTextureFormat::RGBA8,      // Final Color
    };

    PostProcessFramebuffers[0].Create(sceneDesc);
    PostProcessFramebuffers[1].Create(sceneDesc);
}

void KH_Canvas::Render()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGui::Begin("Canvas");
    {
        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        uint32_t viewportWidth = static_cast<uint32_t>(viewportPanelSize.x);
        uint32_t viewportHeight = static_cast<uint32_t>(viewportPanelSize.y);

        KH_Framebuffer& Framebuffer = GetLastPostProcessFramebuffer();

        if (viewportWidth != Framebuffer.GetWidth() || viewportHeight != Framebuffer.GetHeight())
        {
            if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
            {
                ResizeAllFramebuffers(viewportWidth, viewportHeight);

                KH_Editor::Instance().UpdateCanvasExtent(viewportWidth, viewportHeight);
                glViewport(0, 0, KH_Editor::GetCanvasWidth(), KH_Editor::GetCanvasHeight());

                Timer.Reset();
            }
        }

        Timer.Tick(ImGui::GetIO().DeltaTime);

        if (Timer.HasFinished())
        {
            std::string DebugMessage = std::format("Canvas size has been changed to [{},{}]", viewportWidth, viewportHeight);
            LOG_D(DebugMessage);
            Timer.InActive();
        }

        uint32_t textureID = Framebuffer.GetColorAttachmentID(0);

        ImVec2 imagePos = ImGui::GetCursorScreenPos();
        CanvasMin = glm::vec2(imagePos.x, imagePos.y);
        CanvasSize = glm::vec2(viewportPanelSize.x, viewportPanelSize.y);
        CanvasMax = CanvasMin + CanvasSize;

        ImGui::Image((void*)(intptr_t)textureID, viewportPanelSize, ImVec2(0, 1), ImVec2(1, 0));

        // 关键：必须在 Canvas 窗口内部画
        KH_Editor::Instance().DrawCanvasGizmos();

        bIsFocused = ImGui::IsWindowFocused();
        bIsHovered = ImGui::IsWindowHovered();
    }
    ImGui::End();

    ImGui::PopStyleVar();
}
KH_Framebuffer& KH_Canvas::GetSceneFramebuffer()
{
    return SceneFramebuffer[FrameBufferHandle];
}

KH_Framebuffer& KH_Canvas::GetCurrentPostProcessFramebuffer()
{
	return PostProcessFramebuffers[CurrentPostProcessFBO];
}

KH_Framebuffer& KH_Canvas::GetLastPostProcessFramebuffer()
{
    return PostProcessFramebuffers[(CurrentPostProcessFBO + 1) % 2];
}

KH_Framebuffer& KH_Canvas::GetLastFramebuffer()
{
    return SceneFramebuffer[(FrameBufferHandle + 1) % 2];
}

void KH_Canvas::BindSceneFramebuffer()
{
    GetSceneFramebuffer().Bind();
}

void KH_Canvas::UnbindSceneFramebuffer()
{
    GetSceneFramebuffer().Unbind();
}

void KH_Canvas::BindPostProcessFramebuffer()
{
    GetCurrentPostProcessFramebuffer().Bind();
}

void KH_Canvas::UnbindPostProcessFramebuffer()
{
    GetLastPostProcessFramebuffer().Unbind();
}

void KH_Canvas::SwapPostProcessFramebuffers()
{
    CurrentPostProcessFBO = (CurrentPostProcessFBO + 1) % 2;
}

void KH_Canvas::SwapFramebuffer()
{
    FrameBufferHandle = (FrameBufferHandle + 1) % 2;
}

void KH_Canvas::ResizeAllFramebuffers(int width, int height)
{
    SceneFramebuffer[0].Resize(width, height);
    SceneFramebuffer[1].Resize(width, height);

    PostProcessFramebuffers[0].Resize(width, height);
    PostProcessFramebuffers[1].Resize(width, height);
}


bool KH_Canvas::IsMouseInsideCanvas() const
{
    if (!bIsHovered || CanvasSize.x <= 0.0f || CanvasSize.y <= 0.0f)
        return false;

    const ImVec2 mousePos = ImGui::GetIO().MousePos;

    return mousePos.x >= CanvasMin.x && mousePos.x <= CanvasMax.x &&
        mousePos.y >= CanvasMin.y && mousePos.y <= CanvasMax.y;
}

bool KH_Canvas::GetMouseCanvasUV(glm::vec2& outUV) const
{
    if (!IsMouseInsideCanvas())
        return false;

    const ImVec2 mousePos = ImGui::GetIO().MousePos;
    glm::vec2 mouse(mousePos.x, mousePos.y);

    outUV = (mouse - CanvasMin) / CanvasSize;
    outUV = glm::clamp(outUV, glm::vec2(0.0f), glm::vec2(1.0f));
    return true;
}

bool KH_Canvas::GetMouseCanvasNDC(glm::vec2& outNDC) const
{
    glm::vec2 uv(0.0f);
    if (!GetMouseCanvasUV(uv))
        return false;

    outNDC.x = uv.x * 2.0f - 1.0f;
    outNDC.y = 1.0f - uv.y * 2.0f;
    return true;
}

bool KH_Canvas::BuildPickRay(const KH_Camera& Camera, KH_Ray& outRay) const
{
    glm::vec2 ndc(0.0f);
    if (!GetMouseCanvasNDC(ndc))
        return false;

    outRay = Camera.GetRay(ndc.x, ndc.y);

    return true;
}

bool KH_Canvas::TryBuildPickRay(const KH_Camera& Camera, KH_Ray& outRay, int mouseButton) const
{
    if (!IsMouseInsideCanvas())
        return false;

    if (!ImGui::IsMouseClicked(mouseButton))
        return false;

    return BuildPickRay(Camera, outRay);
}


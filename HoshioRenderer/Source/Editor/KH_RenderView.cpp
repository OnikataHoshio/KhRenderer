#include "KH_RenderView.h"
#include "KH_Editor.h"
#include "Utils/KH_DebugUtils.h"

KH_RenderView::KH_RenderView()
{
    Framebuffer.Create(64, 64);
}

void KH_RenderView::Render()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGui::Begin("RenderView"); 
    {
        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        uint32_t viewportWidth = static_cast<uint32_t>(viewportPanelSize.x);
        uint32_t viewportHeight = static_cast<uint32_t>(viewportPanelSize.y);


        if (viewportWidth != Framebuffer.GetWidth() || viewportHeight != Framebuffer.GetHeight())
        {
            if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
                Framebuffer.Rescale(viewportWidth, viewportHeight);
                KH_Editor::Instance().UpdateCanvasExtent(viewportWidth, viewportHeight);
                glViewport(0, 0, KH_Editor::CanvasWidth, KH_Editor::CanvasHeight);

                std::string DebugMessage = std::format("Canvas size has been changed to [{},{}]", viewportWidth, viewportHeight);
                LOG_D(DebugMessage);
            }
        }

        uint32_t textureID = Framebuffer.GetTextureID();
        ImGui::Image((void*)(intptr_t)textureID, viewportPanelSize, ImVec2(0, 1), ImVec2(1, 0));

        bIsFocused = ImGui::IsWindowFocused();
        bIsHovered = ImGui::IsWindowHovered();
    }
    ImGui::End();

    ImGui::PopStyleVar(); 
}

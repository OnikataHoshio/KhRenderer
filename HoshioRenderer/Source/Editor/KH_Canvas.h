#pragma once
#include "KH_Panel.h"

class KH_Camera;
class KH_Ray;

class KH_Canvas : public KH_Panel
{
public:
	KH_Canvas();
	~KH_Canvas() override = default;

	void Render() override;

	KH_Framebuffer& GetSceneFramebuffer();
	KH_Framebuffer& GetCurrentPostProcessFramebuffer();
	KH_Framebuffer& GetLastPostProcessFramebuffer();
	KH_Framebuffer& GetLastFramebuffer();

	void BindSceneFramebuffer();
	void UnbindSceneFramebuffer();

	void BindPostProcessFramebuffer();
	void UnbindPostProcessFramebuffer();

	void SwapPostProcessFramebuffers();

	void SwapFramebuffer();

	bool IsMouseInsideCanvas() const;
	bool GetMouseCanvasUV(glm::vec2& outUV) const;     // [0,1]
	bool GetMouseCanvasNDC(glm::vec2& outNDC) const;   // [-1,1]
	bool BuildPickRay(const KH_Camera& Camera, KH_Ray& outRay) const;
	bool TryBuildPickRay(const KH_Camera& Camera, KH_Ray& outRay, int mouseButton = 0) const;


	const glm::vec2& GetCanvasMin() const { return CanvasMin; }
	const glm::vec2& GetCanvasMax() const { return CanvasMax; }
	const glm::vec2& GetCanvasSize() const { return CanvasSize; }

	bool IsFocused() const { return bIsFocused; }
	bool IsHovered() const { return bIsHovered; }

private:

	void ResizeAllFramebuffers(int width, int height);


	bool bHistoryValid = false;

	uint32_t FrameBufferHandle = 0;
	uint32_t CurrentPostProcessFBO = 0;
	KH_Framebuffer SceneFramebuffer[2];
	//KH_Framebuffer TAAFramebuffers[2];
	KH_Framebuffer PostProcessFramebuffers[2];
	KH_Timer Timer;

	glm::vec2 CanvasMin = glm::vec2(0.0f);
	glm::vec2 CanvasMax = glm::vec2(0.0f);
	glm::vec2 CanvasSize = glm::vec2(0.0f);
};
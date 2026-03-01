#pragma once
#include "Pipeline/KH_Framebuffer.h"

class KH_RenderView
{
public:
	KH_RenderView();
	~KH_RenderView() = default;

	void Render();

	KH_Framebuffer Framebuffer;

private:
	bool bIsFocused = false;
	bool bIsHovered = false;

};

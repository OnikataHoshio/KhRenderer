#pragma once

#include "Pipeline/KH_Framebuffer.h"
#include "Utils/KH_Timer.h"

struct KH_LOG_MESSAGE;

class KH_Panel
{
public:
	KH_Panel() = default;
	virtual ~KH_Panel() = default;

	virtual void Render() = 0;
protected:
	bool bIsFocused = false;
	bool bIsHovered = false;
};


class KH_Canvas : public KH_Panel
{
public:
	KH_Canvas();
	~KH_Canvas() override = default;

	void Render() override;

	KH_Framebuffer& GetCurrentFramebuffer();
	KH_Framebuffer& GetLastFramebuffer();

	void BindFramebuffer();
	void UnbindFramebuffer();

private:
	void SwapFramebuffer();

	uint32_t FrameBufferHandle = 0;
	KH_Framebuffer Framebuffers[2];
	KH_Framebuffer PostProcessFrameBuffer;
	KH_Timer Timer;
};

class KH_Console : public KH_Panel
{
public:
	KH_Console() = default;
	~KH_Console() override = default;

	void Render() override;

	static std::vector<KH_LOG_MESSAGE> LogMessages;
};

class KH_Setting : public KH_Panel
{
public:
	KH_Setting() = default;
	~KH_Setting() override = default;

	void Render() override;
};


class KH_GlobalInfo : public KH_Panel
{
public:
	KH_GlobalInfo() = default;
	 ~KH_GlobalInfo() override = default;

	 void Render() override;
};

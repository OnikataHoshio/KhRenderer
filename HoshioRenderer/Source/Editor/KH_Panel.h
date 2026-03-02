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


class KH_RenderView : public KH_Panel
{
public:
	KH_RenderView();
	virtual ~KH_RenderView() override = default;

	virtual void Render() override;

	KH_Framebuffer Framebuffer;

private:
	KH_Timer Timer;
};


class KH_Console : public KH_Panel
{
public:
	KH_Console() = default;
	virtual ~KH_Console() override = default;

	virtual void Render() override;

	static std::vector<KH_LOG_MESSAGE> LogMessages;

};

class KH_Setting : public KH_Panel
{
public:
	KH_Setting() = default;
	virtual ~KH_Setting() override = default;

	virtual void Render() override;
};


class KH_GlobalInfo : public KH_Panel
{
public:
	KH_GlobalInfo() = default;
	virtual ~KH_GlobalInfo() override = default;

	virtual void Render() override;
};

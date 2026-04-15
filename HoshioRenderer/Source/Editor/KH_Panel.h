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

class KH_Console : public KH_Panel
{
public:
    KH_Console() = default;
    ~KH_Console() override = default;

    void Render() override;

    static std::vector<KH_LOG_MESSAGE> LogMessages;
};

class KH_Insepctor : public KH_Panel
{
public:
    KH_Insepctor() = default;
    ~KH_Insepctor() override = default;

    void Render() override;
};

class KH_GlobalInfo : public KH_Panel
{
public:
    KH_GlobalInfo() = default;
    ~KH_GlobalInfo() override = default;

    void Render() override;
};

class KH_SceneTree : public KH_Panel
{
public:
    KH_SceneTree() = default;
    ~KH_SceneTree() override = default;

    void Render() override;
};

// 保留类名兼容现有调用；UI 窗口标题改为更清晰的 “Render Pipeline”
class KH_RenderPipeline : public KH_Panel
{
public:
    KH_RenderPipeline() = default;
    ~KH_RenderPipeline() override = default;

    void Render() override;
};

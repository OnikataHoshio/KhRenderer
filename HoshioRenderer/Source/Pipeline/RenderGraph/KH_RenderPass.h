#pragma once
#include "KH_Common.h"

class KH_RenderPass
{
public:
    virtual ~KH_RenderPass() = default;

    virtual const std::string& GetName() const = 0;

    virtual bool IsEnabled() const { return true; }
};
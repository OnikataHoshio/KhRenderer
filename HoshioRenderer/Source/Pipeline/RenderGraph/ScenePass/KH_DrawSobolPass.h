#pragma once

#include "Pipeline/RenderGraph/KH_ScenePass.h"

class KH_DrawSobolPass :  public KH_ScenePass
{
public:
	KH_DrawSobolPass(const std::string& name, KH_Shader shader);
	virtual ~KH_DrawSobolPass() override = default;

	void Execute() override;
};

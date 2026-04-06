#pragma once

#include "Pipeline/RenderGraph/KH_PostProcessPass.h"

class KH_GammaCorrectionPass : public KH_PostProcessPass
{
public:
	KH_GammaCorrectionPass(const std::string& name, KH_Shader shader, float gamma = 2.2);


	~KH_GammaCorrectionPass() override = default;

	void Execute(KH_Framebuffer& Input, KH_Framebuffer& Output) override;

	float Gamma = 2.2;
};
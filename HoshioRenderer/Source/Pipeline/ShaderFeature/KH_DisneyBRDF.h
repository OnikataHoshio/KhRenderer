#pragma once

#include "Pipeline/KH_ShaderFeature.h"

class KH_DisneyBRDF : public KH_ShaderFeatureBase
{
public:
	KH_DisneyBRDF() = default;

	KH_DisneyBRDF(const KH_Shader& shader);

	~KH_DisneyBRDF() override = default;

	void DrawControlPanel() override;

	void ApplyUniforms() override;

	void SetEnableSobol(bool bEnable);

	void SetEnableImportantSampling(bool bEnable);

	void SetEnableDiffuseIS(bool bEnable);

	void SetEnableSpecularIS(bool bEnable);

	void SetEnableClearcoatIS(bool bEnable);

private:
	int uEnableSobol = 0;
	int uEnableImportantSampling = 0;
	int uAllowSingleIS = 0;
	int uEnableDiffuseIS = 0;
	int uEnableSpecularIS = 0;
	int uEnableClearcoatIS = 0;
};
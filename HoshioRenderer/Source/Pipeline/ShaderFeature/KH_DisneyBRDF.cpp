#include "KH_DisneyBRDF.h"
#include "Editor/KH_Editor.h"

KH_DisneyBRDF::KH_DisneyBRDF(const KH_Shader& shader)
	: KH_ShaderFeatureBase(Shader)
{

}

void KH_DisneyBRDF::DrawControlPanel()
{
	KH_Editor& Editor = KH_Editor::Instance();

	static bool bEnableSobol = false;
	static bool bEnableImportantSampling = false;
	static int SelectedISMode = 0;   // 0: Diffuse, 1: Specular, 2: Clearcoat
	static int PrevAllowSingleIS = uAllowSingleIS;

	bool bEnableDiffuseIS = false;
	bool bEnableSpecularIS = false;
	bool bEnableClearcoatIS = false;

	ImGui::SeparatorText("DisneyBRDF");
	ImGui::Indent(20.0f);
	ImGui::Text("FrameCounter: %d", std::min(Editor.GetFrameCounter(), 2048u));

	bool bNeedReset = false;

	if (ImGui::Checkbox("EnableSobol", &bEnableSobol))
	{
		bNeedReset = true;
	}

	if (ImGui::Checkbox("EnableImportantSampling", &bEnableImportantSampling))
	{
		bNeedReset = true;
	}


	if (bEnableImportantSampling)
	{
		bool bSingleIS = (uAllowSingleIS == 1);
		if (ImGui::Checkbox("Allow Single IS", &bSingleIS))
		{
			uAllowSingleIS = bSingleIS ? 1 : 0;
			bNeedReset = true;
		}

		if (PrevAllowSingleIS != uAllowSingleIS)
		{
			PrevAllowSingleIS = uAllowSingleIS;
			bNeedReset = true;
		}

		if (uAllowSingleIS == 1)
		{
			ImGui::Text("Importance Sampling Mode");

			if (ImGui::RadioButton("Diffuse IS", SelectedISMode == 0))
			{
				SelectedISMode = 0;
				bNeedReset = true;
			}
			if (ImGui::RadioButton("Specular IS", SelectedISMode == 1))
			{
				SelectedISMode = 1;
				bNeedReset = true;
			}
			if (ImGui::RadioButton("Clearcoat IS", SelectedISMode == 2))
			{
				SelectedISMode = 2;
				bNeedReset = true;
			}

			bEnableDiffuseIS = (SelectedISMode == 0);
			bEnableSpecularIS = (SelectedISMode == 1);
			bEnableClearcoatIS = (SelectedISMode == 2);
		}
		else
		{
			bEnableDiffuseIS = true;
			bEnableSpecularIS = true;
			bEnableClearcoatIS = true;

			ImGui::TextDisabled("Importance Sampling: All Enabled");
		}

	}
	
	ImGui::Unindent(20.0f);

	if (bNeedReset)
	{
		Editor.RequestFrameReset();
	}

	SetEnableSobol(bEnableSobol);
	SetEnableImportantSampling(bEnableImportantSampling);
	SetEnableDiffuseIS(bEnableDiffuseIS);
	SetEnableSpecularIS(bEnableSpecularIS);
	SetEnableClearcoatIS(bEnableClearcoatIS);
}

void KH_DisneyBRDF::ApplyUniforms()
{
	Shader.SetInt("uEnableSobol", uEnableSobol);
	Shader.SetInt("uEnableImportantSampling", uEnableImportantSampling);
	Shader.SetInt("uAllowSingleIS", uAllowSingleIS);
	Shader.SetInt("uEnableDiffuseIS", uEnableDiffuseIS);
	Shader.SetInt("uEnableSpecularIS", uEnableSpecularIS);
	Shader.SetInt("uEnableClearcoatIS", uEnableClearcoatIS);
}

void KH_DisneyBRDF::SetEnableSobol(bool bEnable)
{
	uEnableSobol = bEnable ? 1 : 0;
}

void KH_DisneyBRDF::SetEnableImportantSampling(bool bEnable)
{
	uEnableImportantSampling = bEnable ? 1 : 0;
}

void KH_DisneyBRDF::SetEnableDiffuseIS(bool bEnable)
{
	uEnableDiffuseIS = bEnable ? 1 : 0;
}

void KH_DisneyBRDF::SetEnableSpecularIS(bool bEnable)
{
	uEnableSpecularIS = bEnable ? 1 : 0;
}

void KH_DisneyBRDF::SetEnableClearcoatIS(bool bEnable)
{
	uEnableClearcoatIS = bEnable ? 1 : 0;
}


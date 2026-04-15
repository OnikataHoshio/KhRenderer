#include "KH_DisneyBRDF.h"
#include "Editor/KH_Editor.h"

KH_DisneyBRDF::KH_DisneyBRDF()
    : KH_ShaderFeatureBase(KH_ShaderFeatureType::DisneyBRDF)
{
    Material_SSBO.SetBindPoint(4);
}

KH_DisneyBRDF::KH_DisneyBRDF(const KH_Shader& shader)
    : KH_ShaderFeatureBase(KH_ShaderFeatureType::DisneyBRDF, shader)
{
    Material_SSBO.SetBindPoint(4);
}

int KH_DisneyBRDF::AddMaterial(const KH_BRDFMaterial& material)
{
    Materials.push_back(material);
    return static_cast<int>(Materials.size()) - 1;
}

std::vector<KH_BRDFMaterial>& KH_DisneyBRDF::GetMaterials()
{
    return Materials;
}

const std::vector<KH_BRDFMaterial>& KH_DisneyBRDF::GetMaterials() const
{
    return Materials;
}

std::vector<KH_BRDFMaterialEncoded> KH_DisneyBRDF::EncodeMaterials() const
{
    std::vector<KH_BRDFMaterialEncoded> encoded(Materials.size());

    for (size_t i = 0; i < Materials.size(); ++i)
    {
        const KH_BRDFMaterial& mat = Materials[i];

        encoded[i].Emissive = glm::vec4(mat.Emissive, 1.0f);
        encoded[i].BaseColor = glm::vec4(mat.BaseColor, 1.0f);
        encoded[i].Param1 = glm::vec4(
            mat.Subsurface,
            mat.Metallic,
            mat.Specular,
            mat.SpecularTint);

        encoded[i].Param2 = glm::vec4(
            mat.Roughness,
            mat.Anisotropic,
            mat.Sheen,
            mat.SheenTint);

        encoded[i].Param3 = glm::vec4(
            mat.Clearcoat,
            mat.ClearcoatGloss,
            mat.IOR,
            mat.Transmission);
    }

    return encoded;
}

void KH_DisneyBRDF::UploadMaterialBuffer()
{
    std::vector<KH_BRDFMaterialEncoded> encoded = EncodeMaterials();

    if (encoded.empty())
    {
        encoded.resize(1);
    }

    Material_SSBO.SetData(encoded);
}

void KH_DisneyBRDF::BindBuffers()
{
    Material_SSBO.Bind();
}

void KH_DisneyBRDF::ClearMaterials()
{
    Materials.clear();
    UploadMaterialBuffer();
}

int KH_DisneyBRDF::GetMaterialCount() const
{
    return static_cast<int>(Materials.size());
}

bool KH_DisneyBRDF::DeleteMaterial(int materialID)
{
    if (materialID < 0 || materialID >= static_cast<int>(Materials.size()))
        return false;

    Materials.erase(Materials.begin() + materialID);
    return true;
}

void KH_DisneyBRDF::DrawControlPanel()
{
    KH_Editor& Editor = KH_Editor::Instance();

    static bool bEnableSobol = false;
    static bool bEnableImportanceSampling = false;
    static bool bEnableMIS = false;
    static bool bEnableSkybox = false;
    static int SelectedISMode = 0;
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

    if (ImGui::Checkbox("EnableImportanceSampling", &bEnableImportanceSampling))
    {
        bNeedReset = true;
    }

    if (ImGui::Checkbox("EnableSkybox", &bEnableSkybox))
    {
        bNeedReset = true;
    }

    if (bEnableImportanceSampling)
    {
        if (bEnableSkybox)
        {
            if (ImGui::Checkbox("EnableMIS", &bEnableMIS))
            {
                bNeedReset = true;
            }
        }

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
    SetEnableSkybox(bEnableSkybox);
    SetEnableImportanceSampling(bEnableImportanceSampling);
    SetEnableMIS(bEnableMIS);
    SetEnableDiffuseIS(bEnableDiffuseIS);
    SetEnableSpecularIS(bEnableSpecularIS);
    SetEnableClearcoatIS(bEnableClearcoatIS);
}

void KH_DisneyBRDF::ApplyUniforms()
{
    Shader.SetInt("uEnableSobol", uEnableSobol);
    Shader.SetInt("uEnableSkybox", uEnableSkybox);
    Shader.SetInt("uEnableImportanceSampling", uEnableImportanceSampling);
    Shader.SetInt("uEnableMIS", uEnableMIS);
    Shader.SetInt("uAllowSingleIS", uAllowSingleIS);
    Shader.SetInt("uEnableDiffuseIS", uEnableDiffuseIS);
    Shader.SetInt("uEnableSpecularIS", uEnableSpecularIS);
    Shader.SetInt("uEnableClearcoatIS", uEnableClearcoatIS);
}

void KH_DisneyBRDF::SetEnableSobol(bool bEnable)
{
    uEnableSobol = bEnable ? 1 : 0;
}

void KH_DisneyBRDF::SetEnableSkybox(bool bEnable)
{
    uEnableSkybox = bEnable ? 1 : 0;
}

void KH_DisneyBRDF::SetEnableImportanceSampling(bool bEnable)
{
    uEnableImportanceSampling = bEnable ? 1 : 0;
}

void KH_DisneyBRDF::SetEnableMIS(bool bEnable)
{
    uEnableMIS = bEnable ? 1 : 0;
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

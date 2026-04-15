#include "KH_BSSRDF.h"
#include "Editor/KH_Editor.h"

KH_BSSRDF::KH_BSSRDF()
    : KH_ShaderFeatureBase(KH_ShaderFeatureType::BSSRDF)
{
    Material_SSBO.SetBindPoint(4);
    InvertCDF_SSBO.SetBindPoint(6);
    PrecomputeInvertCDF();
}

KH_BSSRDF::KH_BSSRDF(const KH_Shader& shader)
    : KH_ShaderFeatureBase(KH_ShaderFeatureType::BSSRDF, shader)
{
    Material_SSBO.SetBindPoint(4);
    InvertCDF_SSBO.SetBindPoint(6);
    PrecomputeInvertCDF();
}

int KH_BSSRDF::AddMaterial(const KH_BSSRDFMaterial& material)
{
    Materials.push_back(material);
    return static_cast<int>(Materials.size()) - 1;
}

std::vector<KH_BSSRDFMaterial>& KH_BSSRDF::GetMaterials()
{
    return Materials;
}

const std::vector<KH_BSSRDFMaterial>& KH_BSSRDF::GetMaterials() const
{
    return Materials;
}

std::vector<KH_BSSRDFMaterialEncoded> KH_BSSRDF::EncodeMaterials() const
{
    std::vector<KH_BSSRDFMaterialEncoded> encoded(Materials.size());

    for (size_t i = 0; i < Materials.size(); ++i)
    {
        const KH_BSSRDFMaterial& mat = Materials[i];

        encoded[i].Emissive = glm::vec4(mat.Emissive, 1.0f);
        encoded[i].BaseColor = glm::vec4(mat.BaseColor, 1.0f);
        encoded[i].Radius = glm::vec4(mat.Radius, 1.0f);
        encoded[i].Eta = glm::vec2(mat.Eta, 0.0f);
        encoded[i].Scale = glm::vec2(mat.Scale, 0.0f);
    }

    return encoded;
}

void KH_BSSRDF::UploadMaterialBuffer()
{
    std::vector<KH_BSSRDFMaterialEncoded> encoded = EncodeMaterials();

    if (encoded.empty())
    {
        encoded.resize(1);
    }

    Material_SSBO.SetData(encoded);
}

void KH_BSSRDF::BindBuffers()
{
    Material_SSBO.Bind();
    InvertCDF_SSBO.Bind();
}

void KH_BSSRDF::ClearMaterials()
{
    Materials.clear();
    UploadMaterialBuffer();
}

int KH_BSSRDF::GetMaterialCount() const
{
    return static_cast<int>(Materials.size());
}

bool KH_BSSRDF::DeleteMaterial(int materialID)
{
    if (materialID < 0 || materialID >= static_cast<int>(Materials.size()))
        return false;

    Materials.erase(Materials.begin() + materialID);
    return true;
}

void KH_BSSRDF::DrawControlPanel()
{
    KH_Editor& Editor = KH_Editor::Instance();

    static bool bEnableSkybox = false;

    ImGui::SeparatorText("BSSRDF");

    ImGui::Indent(20.0f);
    ImGui::Text("FrameCounter: %d", std::min(Editor.GetFrameCounter(), 2048u));

    bool bNeedReset = false;

    if (ImGui::DragFloat("Rmax", &Rmax, 0.001f, 0.001f, 1000.0f))
    {
        bNeedReset = true;
    }
 
    if (ImGui::Checkbox("EnableSkybox", &bEnableSkybox))
    {
        bNeedReset = true;
    }

    ImGui::Unindent(20.0f);

    if (bNeedReset)
    {
        Editor.RequestFrameReset();
    }

    SetEnableSkybox(bEnableSkybox);
}

void KH_BSSRDF::ApplyUniforms()
{
    Shader.SetInt("uInvertCDFResolution", InvertCDF_Resolution);
    Shader.SetFloat("uRmax", Rmax);
    Shader.SetInt("uEnableSkybox", EnableSkybox);
}

float KH_BSSRDF::InvertCDF_Newton(float init_value, float xi)
{
    float x = init_value;
    for (int k = 0; k < 8; ++k)
    {
        float ex1 = exp(-x);
        float ex3 = exp(-x / 3.0f);
        float f = 1.0f - 0.25f * ex1 - 0.75f * ex3 - xi;
        float fp = 0.25f * ex1 + 0.25f * ex3;
        x -= f / fp;
        if (x < 0.0f) x = 0.0f;
    }
    return x;
}

void KH_BSSRDF::PrecomputeInvertCDF()
{
    float step = (1.0f - EPS) / (InvertCDF_Resolution - 1.0f);
    std::vector<float> table(InvertCDF_Resolution);

    for (int i = 0; i < InvertCDF_Resolution; i++)
    {
        float init_value = (i == 0) ? 0.0f : table[i - 1];
        float xi = i * step;
        table[i] = InvertCDF_Newton(init_value, xi);
    }

    InvertCDF_SSBO.SetData(table, GL_STATIC_DRAW);
}

void KH_BSSRDF::SetEnableSkybox(bool bEnable)
{
    EnableSkybox = bEnable ? 1 : 0;
}

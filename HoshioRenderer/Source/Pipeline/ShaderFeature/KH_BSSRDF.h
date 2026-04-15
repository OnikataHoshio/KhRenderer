#pragma once

#include "Pipeline/KH_ShaderFeature.h"
#include "Pipeline/KH_Buffer.h"

struct KH_BSSRDFMaterial
{
    glm::vec3 Emissive = glm::vec3(0.0f);
    glm::vec3 BaseColor = glm::vec3(0.0f);
    glm::vec3 Radius = glm::vec3(1.0f, 0.2f, 0.1f);
    float Eta = 1.3f;
    float Scale = 0.05f;
};

struct KH_BSSRDFMaterialEncoded
{
    glm::vec4 Emissive;
    glm::vec4 BaseColor;
    glm::vec4 Radius; 
    glm::vec2 Eta;
    glm::vec2 Scale;
};

class KH_BSSRDF : public KH_ShaderFeatureBase
{
public:
    KH_BSSRDF();
    explicit KH_BSSRDF(const KH_Shader& shader);
    ~KH_BSSRDF() override = default;

    int AddMaterial(const KH_BSSRDFMaterial& material = KH_BSSRDFMaterial{});
    std::vector<KH_BSSRDFMaterial>& GetMaterials();
    const std::vector<KH_BSSRDFMaterial>& GetMaterials() const;

    void DrawControlPanel() override;
    void ApplyUniforms() override;

    void UploadMaterialBuffer() override;
    void BindBuffers() override;
    void ClearMaterials() override;
    int GetMaterialCount() const override;
    bool DeleteMaterial(int materialID) override;

private:
    std::vector<KH_BSSRDFMaterialEncoded> EncodeMaterials() const;

    float InvertCDF_Newton(float init_value, float xi);
    void PrecomputeInvertCDF();

private:
    int InvertCDF_Resolution = 4096;
    float Rmax = 0.01f;
    int EnableSkybox = 0;

    std::vector<KH_BSSRDFMaterial> Materials;
    KH_SSBO<KH_BSSRDFMaterialEncoded> Material_SSBO;
    KH_SSBO<float> InvertCDF_SSBO;

    void SetEnableSkybox(bool bEnable);
};
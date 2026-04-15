#pragma once

#include "Pipeline/KH_ShaderFeature.h"
#include "Pipeline/KH_Buffer.h"
struct KH_BRDFMaterial
{
    glm::vec3 Emissive = glm::vec3(0.0f);
    glm::vec3 BaseColor = glm::vec3(1.0f);
    float Subsurface = 0.0f;
    float Metallic = 0.0f;
    float Specular = 0.0f;
    float SpecularTint = 0.0f;
    float Roughness = 0.0f;
    float Anisotropic = 0.0f;
    float Sheen = 0.0f;
    float SheenTint = 0.0f;
    float Clearcoat = 0.0f;
    float ClearcoatGloss = 0.0f;
    float IOR = 1.0f;
    float Transmission = 0.0f;
};

struct KH_BRDFMaterialEncoded
{
    glm::vec4 Emissive;
    glm::vec4 BaseColor;
    glm::vec4 Param1;
    glm::vec4 Param2;
    glm::vec4 Param3;
};

class KH_DisneyBRDF : public KH_ShaderFeatureBase
{
public:
    KH_DisneyBRDF();
    explicit KH_DisneyBRDF(const KH_Shader& shader);

    ~KH_DisneyBRDF() override = default;

    int AddMaterial(const KH_BRDFMaterial& material = KH_BRDFMaterial{});
    std::vector<KH_BRDFMaterial>& GetMaterials();
    const std::vector<KH_BRDFMaterial>& GetMaterials() const;

    void DrawControlPanel() override;
    void ApplyUniforms() override;

    void UploadMaterialBuffer() override;
    void BindBuffers() override;
    void ClearMaterials() override;
    int GetMaterialCount() const override;
    bool DeleteMaterial(int materialID) override;

private:
    std::vector<KH_BRDFMaterialEncoded> EncodeMaterials() const;

private:
    std::vector<KH_BRDFMaterial> Materials;
    KH_SSBO<KH_BRDFMaterialEncoded> Material_SSBO;

    int uEnableSobol = 0;
    int uEnableSkybox = 0;
    int uEnableImportanceSampling = 0;
    int uEnableMIS = 0;
    int uAllowSingleIS = 0;
    int uEnableDiffuseIS = 0;
    int uEnableSpecularIS = 0;
    int uEnableClearcoatIS = 0;

    void SetEnableSobol(bool bEnable);
    void SetEnableSkybox(bool bEnable);
    void SetEnableImportanceSampling(bool bEnable);
    void SetEnableMIS(bool bEnable);
    void SetEnableDiffuseIS(bool bEnable);
    void SetEnableSpecularIS(bool bEnable);
    void SetEnableClearcoatIS(bool bEnable);
};
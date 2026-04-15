#pragma once
#include "Hit/KH_BVH.h"
#include "KH_Model.h"
#include "Hit/KH_LBVH.h"
#include "Utils/KH_DebugUtils.h"
#include "Pipeline/ShaderFeature/KH_DisneyBRDF.h"
#include "Pipeline/ShaderFeature/KH_BSSRDF.h"

enum class KH_BVH_BUILD_MODE;

struct KH_CameraParam {
    glm::vec4 AspectAndFovy; // .x = Aspect, .y = Fovy
    glm::vec4 Position;
    glm::vec4 Right;
    glm::vec4 Up;
    glm::vec4 Front;
};

class KH_SceneBase
{
    friend class KH_GpuLBVH;

protected:
    KH_SSBO<KH_PrimitiveEncoded> Primitive_SSBO;
    KH_UBO<KH_CameraParam> CameraParam_UB0;

    std::vector<KH_SceneObject> Objects;
    uint32_t PrimitiveCount = 0;

    std::array<std::unique_ptr<KH_ShaderFeatureBase>, KH_ShaderFeatureTypeCount> ShaderFeatures;
    KH_ShaderFeatureType ActiveShaderFeatureType = KH_ShaderFeatureType::DisneyBRDF;

    void SetCameraParamUBO();
    void SetAndBindCameraParamUB0();

    std::vector<KH_PrimitiveEncoded> EncodePrimitives();

    void InitializeModelMaterialSlots(KH_Model& model, int defaultMaterialSlotID);
    void RemapMaterialSlots(KH_ShaderFeatureType type, int removedMaterialID);

public:
    KH_AABB AABB;

    KH_SceneBase() = default;
    virtual ~KH_SceneBase() = default;

    template<typename TFeature, typename... Args>
    TFeature& EmplaceShaderFeature(KH_ShaderFeatureType type, Args&&... args)
    {
        auto feature = std::make_unique<TFeature>(std::forward<Args>(args)...);
        TFeature& ref = *feature;

        ShaderFeatures[KH_ShaderFeatureTypeToIndex(type)] = std::move(feature);
        return ref;
    }

    KH_ShaderFeatureBase* GetShaderFeature(KH_ShaderFeatureType type);
    const KH_ShaderFeatureBase* GetShaderFeature(KH_ShaderFeatureType type) const;

    template<typename TFeature>
    TFeature* GetShaderFeatureAs(KH_ShaderFeatureType type)
    {
        return dynamic_cast<TFeature*>(GetShaderFeature(type));
    }

    template<typename TFeature>
    const TFeature* GetShaderFeatureAs(KH_ShaderFeatureType type) const
    {
        return dynamic_cast<const TFeature*>(GetShaderFeature(type));
    }

    KH_ShaderFeatureBase* GetActiveShaderFeature();
    const KH_ShaderFeatureBase* GetActiveShaderFeature() const;

    KH_ShaderFeatureType GetActiveShaderFeatureType() const;
    bool SetActiveShaderFeature(KH_ShaderFeatureType type);

    std::vector<KH_SceneObject>& GetObjects();
    const std::vector<KH_SceneObject>& GetObjects() const;

    KH_Model& AddModel(int MaterialSlotID, const std::string& Path);
    KH_Model& AddModel(int MaterialSlotID, KH_Model&& Model);
    KH_Model& AddEmptyModel(int MaterialSlotID);

    bool DeleteMaterial(KH_ShaderFeatureType type, int materialID);

    void Clear();

    bool RemoveObjectAt(size_t Index);

    bool SaveToXml(const std::string& filePath) const;
    bool LoadFromXml(const std::string& filePath);

    KH_PickResult Pick(const KH_Ray& ray) const;

    virtual void BindAndBuild() = 0;
};

class KH_GpuLBVHScene : public KH_SceneBase
{
    friend class KH_GpuLBVH;

private:
    void SetSSBOs();
    void SetRayTracingParam(KH_Shader& Shader);
    void UpdateAABB();

public:
    KH_GpuLBVH BVH;

    KH_GpuLBVHScene()
    {
        Primitive_SSBO.SetBindPoint(0);
        CameraParam_UB0.SetBindPoint(5);

        auto& DisneyBRDF_Feature =
            EmplaceShaderFeature<KH_DisneyBRDF>(KH_ShaderFeatureType::DisneyBRDF);
        auto& BSSRDF_Feature =
            EmplaceShaderFeature<KH_BSSRDF>(KH_ShaderFeatureType::BSSRDF);
        DisneyBRDF_Feature.SetShader(KH_ExampleShaders::Instance().DisneyBRDF_4);
        BSSRDF_Feature.SetShader(KH_ExampleShaders::Instance().BSSRDF_3);

        SetActiveShaderFeature(KH_ShaderFeatureType::DisneyBRDF);
    }

    ~KH_GpuLBVHScene() override = default;

    void BindAndBuild() override;
    void UpdateMaterialSSBO();
    void UpdatePrimitiveSSBO();
    void Render();
};

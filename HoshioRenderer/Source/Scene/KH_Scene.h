#pragma once
#include "Hit/KH_BVH.h"
#include "KH_Model.h"
#include "Hit/KH_LBVH.h"
#include "Utils/KH_DebugUtils.h"

enum class KH_BVH_BUILD_MODE;

struct KH_BRDFMaterialEncoded
{
	glm::vec4 Emissive;
	glm::vec4 BaseColor;
	glm::vec4 Param1;
	glm::vec4 Param2;
	glm::vec4 Param3;
};

struct KH_FlatBVHNodeEncoded
{
	glm::ivec4 Param1; //(Left, Right, )
	glm::ivec4 Param2; //(bIsLeaf, Offset, Size)
	glm::vec4 AABB_MinPos;
	glm::vec4 AABB_MaxPos;
};

struct KH_CameraParam {
    glm::vec4 AspectAndFovy; // .x = Aspect, .y = Fovy
    glm::vec4 Position;
    glm::vec4 Right;
    glm::vec4 Up;
    glm::vec4 Front;
};

struct KH_PickResult
{
    bool bIsHit = false;
    int ObjectIndex = -1;
    int MaterialSlotID = KH_MATERIAL_UNDEFINED_SLOT;
    float Distance = std::numeric_limits<float>::max();
    glm::vec3 HitPoint = glm::vec3(0.0f);
    glm::vec3 Normal = glm::vec3(0.0f);
};

class KH_SceneBase {
    friend class KH_GpuLBVH;
protected:
    KH_SSBO<KH_PrimitiveEncoded> Primitive_SSBO;
    KH_SSBO<KH_BRDFMaterialEncoded> Material_SSBO;
    KH_UBO<KH_CameraParam> CameraParam_UB0;

    std::vector<KH_SceneObject> Objects;
    uint32_t PrimitiveCount = 0;

    void SetCameraParamUBO();
    void SetAndBindCameraParamUB0();

    std::vector<KH_PrimitiveEncoded> EncodePrimitives();
    std::vector<KH_BRDFMaterialEncoded> EncodeBRDFMaterials() const;

public:
    std::vector<KH_BRDFMaterial> Materials;

    KH_AABB AABB;

    KH_SceneBase() = default;
    virtual ~KH_SceneBase() = default;

    std::vector<KH_SceneObject>& GetObjects();
    const std::vector<KH_SceneObject>& GetObjects() const;

	KH_Model& AddModel(int MaterialSlotID, const std::string& Path);
    KH_Triangle& AddTriangle(int MaterialSlotID, KH_Triangle Triangle);

    void Clear();

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

    KH_GpuLBVHScene() {
        Primitive_SSBO.SetBindPoint(0);
        Material_SSBO.SetBindPoint(4);
        CameraParam_UB0.SetBindPoint(5);
    }

    ~KH_GpuLBVHScene() = default;

    void BindAndBuild() override;
    void Render();
};

class KH_GpuLBVHExampleScenes : public KH_Singleton<KH_GpuLBVHExampleScenes>
{
    friend class KH_Singleton<KH_GpuLBVHExampleScenes>;

private:
    KH_GpuLBVHExampleScenes() {
        InitBunny();
        InitDebugBox();
        InitSingleTriangle();
    }
    ~KH_GpuLBVHExampleScenes() override = default;

    void InitSingleTriangle();
    void InitDebugBox();
    void InitBunny();

public:
    KH_GpuLBVHScene Bunny;
    KH_GpuLBVHScene SingleTriangle;
    KH_GpuLBVHScene DebugBox;
};


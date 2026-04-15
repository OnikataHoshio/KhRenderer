#include "KH_Scene.h"
#include "Editor/KH_Editor.h"
#include "Pipeline/KH_Shader.h"
#include "Pipeline/KH_Texture.h"
#include "KH_SceneXmlSerializer.h"

void KH_SceneBase::SetCameraParamUBO()
{
    KH_CameraParam CameraParam;
    const KH_Camera& Camera = KH_Editor::Instance().Camera;
    CameraParam.AspectAndFovy = glm::vec4(Camera.Aspect, Camera.Fovy, 0.0f, 0.0f);
    CameraParam.Position = glm::vec4(Camera.Position, 1.0f);
    CameraParam.Right = glm::vec4(Camera.Right, 1.0f);
    CameraParam.Up = glm::vec4(Camera.Up, 1.0f);
    CameraParam.Front = glm::vec4(Camera.Front, 1.0f);

    CameraParam_UB0.SetSingleData(CameraParam);
}

void KH_SceneBase::SetAndBindCameraParamUB0()
{
    SetCameraParamUBO();
    CameraParam_UB0.Bind();
}

std::vector<KH_PrimitiveEncoded> KH_SceneBase::EncodePrimitives()
{
    std::vector<KH_PrimitiveEncoded> Encoded;

    PrimitiveCount = 0;
    for (int i = 0; i < static_cast<int>(Objects.size()); i++)
    {
        PrimitiveCount += Objects[i]->GetPrimitiveCount();
    }

    Encoded.reserve(PrimitiveCount);

    const KH_ShaderFeatureType ActiveType = GetActiveShaderFeatureType();

    for (const auto& sceneObject : Objects)
    {
        sceneObject->EncodePrimitives(Encoded, ActiveType);
    }

    return Encoded;
}

void KH_SceneBase::InitializeModelMaterialSlots(KH_Model& model, int defaultMaterialSlotID)
{
    for (auto& mesh : model.GetMeshes())
    {
        for (size_t i = 0; i < KH_ShaderFeatureTypeCount; ++i)
        {
            if (!ShaderFeatures[i])
                continue;

            mesh.SetMaterialSlotID(
                static_cast<KH_ShaderFeatureType>(i),
                defaultMaterialSlotID);
        }
    }
}

void KH_SceneBase::RemapMaterialSlots(
    KH_ShaderFeatureType type,
    int removedMaterialID)
{
    KH_ShaderFeatureBase* feature = GetShaderFeature(type);
    const int materialCount = feature ? feature->GetMaterialCount() : 0;
    const int fallbackID = (materialCount > 0)
        ? std::min(removedMaterialID, materialCount - 1)
        : KH_MATERIAL_UNDEFINED_SLOT;

    for (auto& sceneObject : Objects)
    {
        for (auto& mesh : sceneObject->GetMeshes())
        {
            int slot = mesh.GetMaterialSlotID(type);

            if (slot == KH_MATERIAL_UNDEFINED_SLOT)
                continue;

            if (slot == removedMaterialID)
            {
                mesh.SetMaterialSlotID(type, fallbackID);
            }
            else if (slot > removedMaterialID)
            {
                mesh.SetMaterialSlotID(type, slot - 1);
            }
        }
    }
}

std::vector<KH_SceneObject>& KH_SceneBase::GetObjects()
{
    return Objects;
}

const std::vector<KH_SceneObject>& KH_SceneBase::GetObjects() const
{
    return Objects;
}

KH_ShaderFeatureBase* KH_SceneBase::GetShaderFeature(KH_ShaderFeatureType type)
{
    return ShaderFeatures[KH_ShaderFeatureTypeToIndex(type)].get();
}

const KH_ShaderFeatureBase* KH_SceneBase::GetShaderFeature(KH_ShaderFeatureType type) const
{
    return ShaderFeatures[KH_ShaderFeatureTypeToIndex(type)].get();
}

KH_ShaderFeatureBase* KH_SceneBase::GetActiveShaderFeature()
{
    return GetShaderFeature(ActiveShaderFeatureType);
}

const KH_ShaderFeatureBase* KH_SceneBase::GetActiveShaderFeature() const
{
    return GetShaderFeature(ActiveShaderFeatureType);
}

KH_ShaderFeatureType KH_SceneBase::GetActiveShaderFeatureType() const
{
    return ActiveShaderFeatureType;
}

bool KH_SceneBase::SetActiveShaderFeature(KH_ShaderFeatureType type)
{
    KH_ShaderFeatureBase* feature = GetShaderFeature(type);
    if (!feature)
        return false;

    ActiveShaderFeatureType = type;
    feature->UploadMaterialBuffer();
    return true;
}

KH_Model& KH_SceneBase::AddModel(int MaterialSlotID, const std::string& Path)
{
    auto obj = std::make_unique<KH_Model>(Path);
    KH_Model& ref = *obj;

    InitializeModelMaterialSlots(ref, MaterialSlotID);

    Objects.push_back(std::move(obj));
    return ref;
}

KH_Model& KH_SceneBase::AddModel(int MaterialSlotID, KH_Model&& Model)
{
    auto obj = std::make_unique<KH_Model>(std::move(Model));
    KH_Model& ref = *obj;

    InitializeModelMaterialSlots(ref, MaterialSlotID);

    Objects.push_back(std::move(obj));
    return ref;
}

KH_Model& KH_SceneBase::AddEmptyModel(int MaterialSlotID)
{
    auto obj = std::make_unique<KH_Model>();
    KH_Model& ref = *obj;

    InitializeModelMaterialSlots(ref, MaterialSlotID);

    Objects.push_back(std::move(obj));
    return ref;
}

bool KH_SceneBase::DeleteMaterial(KH_ShaderFeatureType type, int materialID)
{
    KH_ShaderFeatureBase* feature = GetShaderFeature(type);
    if (!feature)
        return false;

    if (!feature->DeleteMaterial(materialID))
        return false;

    RemapMaterialSlots(type, materialID);
    feature->UploadMaterialBuffer();
    return true;
}

bool KH_SceneBase::RemoveObjectAt(size_t Index)
{
    if (Index >= Objects.size())
        return false;

    Objects.erase(Objects.begin() + static_cast<std::ptrdiff_t>(Index));
    return true;
}

void KH_SceneBase::Clear()
{
    Objects.clear();
    PrimitiveCount = 0;
    AABB.Reset();

    for (auto& feature : ShaderFeatures)
    {
        if (feature)
        {
            feature->ClearMaterials();
        }
    }
}

bool KH_SceneBase::SaveToXml(const std::string& filePath) const
{
    return KH_SceneXmlSerializer::SaveScene(*this, filePath);
}

bool KH_SceneBase::LoadFromXml(const std::string& filePath)
{
    return KH_SceneXmlSerializer::LoadScene(*this, filePath);
}

KH_PickResult KH_SceneBase::Pick(const KH_Ray& ray) const
{
    KH_PickResult best;

    for (int i = 0; i < static_cast<int>(Objects.size()); ++i)
    {
        const KH_SceneObject& sceneObject = Objects[i];
        if (!sceneObject)
            continue;

        if (!sceneObject->GetAABB().Hit(ray).bIsHit)
            continue;

        KH_PickResult pick = sceneObject->Pick(ray);

        if (pick.bIsHit && pick.Distance < best.Distance)
        {
            best = pick;
            best.ObjectIndex = i;
        }
    }

    return best;
}

void KH_GpuLBVHScene::SetSSBOs()
{
    std::vector<KH_PrimitiveEncoded> PrimitiveEncodeds = EncodePrimitives();
    Primitive_SSBO.SetData(PrimitiveEncodeds);

    for (size_t i = 0; i < KH_ShaderFeatureTypeCount; ++i)
    {
        if (ShaderFeatures[i])
        {
            ShaderFeatures[i]->UploadMaterialBuffer();
        }
    }
}

void KH_GpuLBVHScene::SetRayTracingParam(KH_Shader& Shader)
{
    Shader.Use();

    Primitive_SSBO.Bind();
    BVH.Morton3DSSBO.Bind();
    BVH.LBVHNodeSSBO.Bind();
    BVH.AuxiliarySSBO.Bind();

    Shader.SetInt("uLBVHNodeCount", BVH.LBVHNodeCount);
    Shader.SetUint("uFrameCounter", KH_Editor::Instance().GetFrameCounter());
    Shader.SetUvec2(
        "uResolution",
        glm::uvec2(KH_Editor::GetCanvasWidth(), KH_Editor::GetCanvasHeight()));

    KH_Editor::Instance().GetLastFramebuffer().BindColorAttachment(0, 0);
    KH_ExampleTextures::Instance().SkyboxHDR.Bind(1);
    KH_ExampleTextures::Instance().SkyboxHDRCache.Bind(2);

    Shader.SetInt("uLastFrame", 0);
    Shader.SetInt("uSkybox", 1);
    Shader.SetInt("uHDRCache", 2);

    KH_ShaderFeatureBase* feature = GetActiveShaderFeature();
    if (feature)
    {
        feature->BindBuffers();
        feature->ApplyUniforms();
    }

    SetAndBindCameraParamUB0();
}

void KH_GpuLBVHScene::UpdateAABB()
{
    AABB.Reset();
    for (auto& Object : Objects)
    {
        AABB.Merge(Object->GetAABB());
    }
}

void KH_GpuLBVHScene::BindAndBuild()
{
    SetSSBOs();
    UpdateAABB();
    BVH.BindAndBuild(this);
}

void KH_GpuLBVHScene::UpdateMaterialSSBO()
{
    KH_ShaderFeatureBase* feature = GetActiveShaderFeature();
    if (feature)
    {
        feature->UploadMaterialBuffer();
    }
}

void KH_GpuLBVHScene::UpdatePrimitiveSSBO()
{
    std::vector<KH_PrimitiveEncoded> PrimitiveEncodeds = EncodePrimitives();
    Primitive_SSBO.SetData(PrimitiveEncodeds);
}

void KH_GpuLBVHScene::Render()
{
    KH_ShaderFeatureBase* feature = GetActiveShaderFeature();
    if (!feature || !feature->GetShader().IsValid())
        return;

    SetRayTracingParam(feature->GetShader());

    KH_Editor::Instance().BindCanvasFramebuffer();

    glBindVertexArray(KH_DefaultModels::Instance().FullscreenQuad.GetVAO());
    glDrawElements(
        GL_TRIANGLES,
        KH_DefaultModels::Instance().FullscreenQuad.GetNumIndices(),
        GL_UNSIGNED_INT,
        0);
    glBindVertexArray(0);

    KH_Editor::Instance().UnbindCanvasFramebuffer();
}

#include "KH_Scene.h"
#include "Editor/KH_Editor.h"
#include "Pipeline/KH_Shader.h"
#include "Pipeline/KH_Texture.h"

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
    for (int i = 0; i < Objects.size(); i++)
    {
        PrimitiveCount += Objects[i].Object->GetPrimitiveCount();
    }

    Encoded.reserve(PrimitiveCount);

    for (const auto& sceneObject : Objects)
    {
        sceneObject.Object->EncodePrimitives(Encoded, sceneObject.MaterialSlotID);
    }

    return Encoded;
}

std::vector<KH_BRDFMaterialEncoded> KH_SceneBase::EncodeBRDFMaterials() const
{
    const int nMaterials = Materials.size();
    std::vector<KH_BRDFMaterialEncoded> BSDFMaterialEncodeds(nMaterials);

    for (int i = 0; i < nMaterials; i++)
    {
        const KH_BRDFMaterial& Mat = Materials[i];

        BSDFMaterialEncodeds[i].Emissive = glm::vec4(Mat.Emissive, 1.0);
        BSDFMaterialEncodeds[i].BaseColor = glm::vec4(Mat.BaseColor, 1.0);
        BSDFMaterialEncodeds[i].Param1 = glm::vec4(Mat.Subsurface, Mat.Metallic, Mat.Specular, Mat.SpecularTint);
        BSDFMaterialEncodeds[i].Param2 = glm::vec4(Mat.Roughness, Mat.Anisotropic, Mat.Sheen, Mat.SheenTint);
        BSDFMaterialEncodeds[i].Param3 = glm::vec4(Mat.Clearcoat, Mat.ClearcoatGloss, Mat.IOR, Mat.Transmission);
    }

    return BSDFMaterialEncodeds;
}

const std::vector<KH_SceneObject>& KH_SceneBase::GetObjects() const
{
    return Objects;
}

KH_Model& KH_SceneBase::AddModel(int MaterialSlotID, const std::string& Path)
{
    auto obj = std::make_unique<KH_Model>(Path);
    KH_Model& ref = *obj;

    Objects.push_back({
        std::move(obj),
        MaterialSlotID
        });

    return ref;
}

KH_Triangle& KH_SceneBase::AddTriangle(int MaterialSlotID, KH_Triangle Triangle)
{
    auto obj = std::make_unique<KH_Triangle>(Triangle);
    KH_Triangle& ref = *obj;

    Objects.push_back({
        std::move(obj),
        MaterialSlotID
        });

    return ref;
}

void KH_SceneBase::Clear()
{
    Objects.clear();
    AABB.Reset();
}

void KH_Scene<KH_GpuLBVH>::SetSSBOs()
{
    std::vector<KH_PrimitiveEncoded> PrimitiveEncodeds = EncodePrimitives();
    std::vector<KH_BRDFMaterialEncoded> BSDFMaterialEncodeds = EncodeBRDFMaterials();

    Primitive_SSBO.SetData(PrimitiveEncodeds);
    Material_SSBO.SetData(BSDFMaterialEncodeds);
}

void KH_Scene<KH_GpuLBVH>::SetRayTracingParam(KH_Shader& Shader)
{
    Shader.Use();

    const KH_Camera& Camera = KH_Editor::Instance().Camera;

    Primitive_SSBO.Bind();
    BVH.Morton3DSSBO.Bind();
    BVH.LBVHNodeSSBO.Bind();
    BVH.AuxiliarySSBO.Bind();
    Material_SSBO.Bind();

    Shader.SetInt("uLBVHNodeCount", BVH.LBVHNodeCount);
    Shader.SetUint("uFrameCounter", KH_Editor::Instance().GetFrameCounter());
    Shader.SetUvec2("uResolution", glm::uvec2(KH_Editor::GetCanvasWidth(), KH_Editor::GetCanvasHeight()));

    KH_Editor::Instance().GetLastFramebuffer().ActiveAndBindTexture(Shader, "uLastFrame", 0);
    KH_ExampleTextures::Instance().FirePlaceHDR.Bind(Shader, "uSkybox", 1);

    SetAndBindCameraParamUB0();
}

void KH_Scene<KH_GpuLBVH>::UpdateAABB()
{
    for (auto& Object :Objects)
    {
        AABB.Merge(Object.Object->GetAABB());
    }
}

void KH_Scene<KH_GpuLBVH>::BindAndBuild()
{
    SetSSBOs();
    UpdateAABB();
    BVH.BindAndBuild(this);
}

void KH_Scene<KH_GpuLBVH>::Render()
{
    SetRayTracingParam(KH_ExampleShaders::Instance().RayTracingShader2_4);

    KH_Editor::Instance().BindCanvasFramebuffer();

    glBindVertexArray(KH_DefaultModels::Instance().Plane.GetVAO());
    glDrawElements(GL_TRIANGLES, KH_DefaultModels::Instance().Plane.GetNumIndices(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    KH_Editor::Instance().UnbindCanvasFramebuffer();
}

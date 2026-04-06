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

std::vector<KH_SceneObject>& KH_SceneBase::GetObjects()
{
    return Objects;
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
        if (!sceneObject.Object)
            continue;

        if (!sceneObject.Object->GetAABB().Hit(ray).bIsHit)
            continue;

        KH_HitResult hit = sceneObject.Object->Pick(ray);

        if (hit.bIsHit && hit.Distance < best.Distance)
        {
            best.bIsHit = true;
            best.ObjectIndex = i;
            best.MaterialSlotID = sceneObject.MaterialSlotID;
            best.Distance = hit.Distance;
            best.HitPoint = hit.HitPoint;
            best.Normal = hit.Normal;
        }
    }

    return best;
}

void KH_GpuLBVHScene::SetSSBOs()
{
    std::vector<KH_PrimitiveEncoded> PrimitiveEncodeds = EncodePrimitives();
    std::vector<KH_BRDFMaterialEncoded> BSDFMaterialEncodeds = EncodeBRDFMaterials();

    Primitive_SSBO.SetData(PrimitiveEncodeds);
    Material_SSBO.SetData(BSDFMaterialEncodeds);
}

void KH_GpuLBVHScene::SetRayTracingParam(KH_Shader& Shader)
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

    KH_Editor::Instance().GetLastFramebuffer().BindColorAttachment(0, 0);
    KH_ExampleTextures::Instance().FirePlaceHDR.Bind(1);

    Shader.SetInt("uLastFrame", 0);
    Shader.SetInt("uSkybox", 1);

    SetAndBindCameraParamUB0();
}

void KH_GpuLBVHScene::UpdateAABB()
{
    AABB.Reset();
    for (auto& Object :Objects)
    {
        AABB.Merge(Object.Object->GetAABB());
    }
}

void KH_GpuLBVHScene::BindAndBuild()
{
    SetSSBOs();
    UpdateAABB();
    BVH.BindAndBuild(this);
}

void KH_GpuLBVHScene::Render()
{
    KH_Editor::Instance().Scene = this;

    SetRayTracingParam(KH_ExampleShaders::Instance().DisneyBRDF_0);

    KH_Editor::Instance().BindCanvasFramebuffer();

    glBindVertexArray(KH_DefaultModels::Instance().FullscreenQuad.GetVAO());
    glDrawElements(GL_TRIANGLES, KH_DefaultModels::Instance().FullscreenQuad.GetNumIndices(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    KH_Editor::Instance().UnbindCanvasFramebuffer();
}


void KH_GpuLBVHExampleScenes::InitSingleTriangle()
{
    KH_BRDFMaterial Material;
    Material.BaseColor = glm::vec3(1.0, 0.0, 0.0);

    SingleTriangle.Materials.push_back(Material);

    KH_Triangle Triangle(
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0)
    );

    SingleTriangle.AddTriangle(0, Triangle);
    SingleTriangle.BindAndBuild();
}

void KH_GpuLBVHExampleScenes::InitBunny()
{
    KH_BRDFMaterial Material1;
    Material1.BaseColor = glm::vec3(1.0, 0.0, 0.0);

    KH_BRDFMaterial Material2;
    Material2.BaseColor = glm::vec3(0.0, 1.0, 0.0);

    KH_BRDFMaterial Material3;
    Material3.Emissive = glm::vec3(5.0, 5.0, 5.0);
    Material3.BaseColor = glm::vec3(1.0, 1.0, 1.0);

    glm::vec3 v0(-1.0f, 0.0f, -1.0f);
    glm::vec3 v1(1.0f, 0.0f, -1.0f);
    glm::vec3 v2(1.0f, 0.0f, 1.0f);
    glm::vec3 v3(-1.0f, 0.0f, 1.0f);

    KH_Triangle Triangle1(v0, v1, v2);
    KH_Triangle Triangle2(v0, v2, v3);

    Bunny.Materials.push_back(Material1);
    Bunny.Materials.push_back(Material2);
    Bunny.Materials.push_back(Material3);

    KH_Model& Model = Bunny.AddModel(0, "Assert/Models/bunny.obj");
    Model.SetUniformScale(4.0f);
    Model.SetPosition(0.0f, -0.1f, 0.0f);

    Bunny.AddTriangle(1, Triangle1);
    Bunny.AddTriangle(1, Triangle2);

    KH_Triangle& t1 = Bunny.AddTriangle(2, Triangle1);
    KH_Triangle& t2 = Bunny.AddTriangle(2, Triangle2);
    t1.SetUniformScale(0.4f);
    t2.SetUniformScale(0.4f);
    t1.SetPosition(0.0f, 0.8f, 0.0);
    t2.SetPosition(0.0f, 0.8f, 0.0);

    Bunny.BindAndBuild();
}

void KH_GpuLBVHExampleScenes::InitDebugBox()
{
    glm::vec3 Colors[6] = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1),
        glm::vec3(1, 1, 0),
        glm::vec3(1, 0, 1),
        glm::vec3(0, 1, 1)
    };

    for (int i = 0; i < 6; ++i) {
        KH_BRDFMaterial Mat;
        Mat.BaseColor = glm::vec4(Colors[i], 1.0f);
        DebugBox.Materials.push_back(Mat);
    }

    glm::vec3 v[8] = {
        {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, // Front 0,1,2,3
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}  // Back  4,5,6,7
    };

    auto AddQuad = [&](int a, int b, int c, int d, int matID) {
        KH_Triangle& t1 = DebugBox.AddTriangle(matID, KH_Triangle(v[a], v[b], v[c]));
        //KH_Triangle& t2 = DebugBox.AddTriangle(matID, KH_Triangle(v[a], v[c], v[d]));
        };

    AddQuad(0, 1, 2, 3, 0); // Front
    AddQuad(5, 4, 7, 6, 1); // Back
    AddQuad(4, 0, 3, 7, 2); // Left
    AddQuad(1, 5, 6, 2, 3); // Right
    AddQuad(3, 2, 6, 7, 4); // Top
    AddQuad(4, 5, 1, 0, 5); // Bottom

    DebugBox.BindAndBuild();
}


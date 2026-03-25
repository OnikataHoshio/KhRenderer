#pragma once
#include "Hit/KH_BVH.h"
#include "KH_Model.h"
#include "Hit/KH_LBVH.h"
#include "Editor/KH_Editor.h"
#include "Pipeline/KH_Shader.h"
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

    const std::vector<KH_SceneObject>& GetObjects() const;

	KH_Model& AddModel(int MaterialSlotID, const std::string& Path);
    KH_Triangle& AddTriangle(int MaterialSlotID, KH_Triangle Triangle);

    void Clear();
};

template<typename BVHType>
class KH_Scene : public KH_SceneBase{
private:
    KH_SSBO<KH_FlatBVHNodeEncoded> LBVHNode_SSBO;
    KH_SSBO<uint32_t> SortedIndices_SSBO;

    void SetSSBOs();
    void SetRayTracingParam(KH_Shader& Shader);

    std::vector<KH_FlatBVHNodeEncoded> EncodeLBVHNodes();
    std::vector<KH_PrimitiveEncoded> EncodeBVHPrimitives();

    void UpdateAABB();

public:
    BVHType BVH;

    template<typename... Args>
    KH_Scene(Args&&... args) : BVH(std::forward<Args>(args)...) {
        Primitive_SSBO.SetBindPoint(0);
        Material_SSBO.SetBindPoint(1);
        LBVHNode_SSBO.SetBindPoint(2);
        SortedIndices_SSBO.SetBindPoint(3);
        CameraParam_UB0.SetBindPoint(4);
    }

    ~KH_Scene() = default;

    void BindAndBuild();
    void Render();
};

template<>
class KH_Scene<KH_GpuLBVH> : public KH_SceneBase
{
    friend class KH_GpuLBVH;
private:
    void SetSSBOs();
    void SetRayTracingParam(KH_Shader& Shader);
    void UpdateAABB();

public:
    KH_GpuLBVH BVH;

    KH_Scene() {
        Primitive_SSBO.SetBindPoint(0);
        Material_SSBO.SetBindPoint(4);
        CameraParam_UB0.SetBindPoint(5);
    }

    ~KH_Scene() = default;

    void BindAndBuild();
    void Render();
};

using KH_BVHScene = KH_Scene<KH_BVH>;
using KH_FlatBVHScene = KH_Scene<KH_FlatBVH>;
using KH_LBVHScene = KH_Scene<KH_LBVH>;
using KH_GpuLBVHScene = KH_Scene<KH_GpuLBVH>;

template <typename BVHType>
void KH_Scene<BVHType>::SetSSBOs()
{
    if constexpr (std::is_same_v<BVHType, KH_BVH>)
        return;

    std::vector<KH_PrimitiveEncoded> TriangleEncodeds = EncodeBVHPrimitives();
    std::vector<KH_BRDFMaterialEncoded> BSDFMaterialEncodeds = EncodeBRDFMaterials();
    std::vector<KH_FlatBVHNodeEncoded> LBVHNodeEncodeds = EncodeLBVHNodes();

    Primitive_SSBO.SetData(TriangleEncodeds);
    Material_SSBO.SetData(BSDFMaterialEncodeds);
    LBVHNode_SSBO.SetData(LBVHNodeEncodeds);

    if constexpr (std::is_same_v<BVHType, KH_LBVH>)
        SortedIndices_SSBO.SetData(BVH.SortedIndices);
}

template <typename BVHType>
void KH_Scene<BVHType>::SetRayTracingParam(KH_Shader& Shader)
{
    if constexpr (std::is_same_v<BVHType, KH_BVH>)
        return;

    Shader.Use();

    const KH_Camera& Camera = KH_Editor::Instance().Camera;

    Primitive_SSBO.Bind();
    Material_SSBO.Bind();
    LBVHNode_SSBO.Bind();

    if constexpr (std::is_same_v<BVHType, KH_LBVH>)
        SortedIndices_SSBO.Bind();

    Shader.SetInt("PrimitiveCount", PrimitiveCount);
    Shader.SetInt("LBVHNodeCount", BVH.BVHNodes.size());
    Shader.SetInt("RootNodeID", BVH.Root);

    SetAndBindCameraParamUB0();
}

template<typename BVHType>
void KH_Scene<BVHType>::BindAndBuild() {
    UpdateAABB();

    if constexpr (std::is_same_v<BVHType, KH_LBVH>) {
        BVH.BindAndBuild(Objects, AABB);
    }
    else {
        BVH.BindAndBuild(Objects);
    }

    SetSSBOs();

}

template <typename BVHType>
void KH_Scene<BVHType>::Render()
{
    if constexpr (std::is_same_v<BVHType, KH_BVH>)
        return;

    KH_Editor::Instance().BindCanvasFramebuffer();
    
    if constexpr (std::is_same_v<BVHType, KH_FlatBVH>)
        SetRayTracingParam(KH_ExampleShaders::Instance().RayTracingShader1_3);
    else if constexpr (std::is_same_v<BVHType, KH_LBVH>)
        SetRayTracingParam(KH_ExampleShaders::Instance().RayTracingShader2_0);

    glBindVertexArray(KH_DefaultModels::Instance().Plane.GetVAO());
    glDrawElements(GL_TRIANGLES, KH_DefaultModels::Instance().Plane.GetNumIndices(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    KH_Editor::Instance().UnbindCanvasFramebuffer();
}


template <typename BVHType>
std::vector<KH_FlatBVHNodeEncoded> KH_Scene<BVHType>::EncodeLBVHNodes()
{
    if constexpr (std::is_same_v<BVHType, KH_BVH>)
        return std::vector<KH_FlatBVHNodeEncoded>();

    const int nLBVHNodes = BVH.BVHNodes.size();
    std::vector<KH_FlatBVHNodeEncoded> LBVHNodeEncoded(nLBVHNodes);

    for (int i = 0; i < nLBVHNodes; i++)
    {
        auto& Node = BVH.BVHNodes[i];

        LBVHNodeEncoded[i].Param1 = glm::ivec4(Node.Left, Node.Right, 0.0, 0.0);
        if constexpr (std::is_same_v<BVHType, KH_FlatBVH>)
        {
            LBVHNodeEncoded[i].Param2 = glm::ivec4(Node.bIsLeaf ? 1 : 0, Node.Offset, Node.Size, 0);
        }
    	else if constexpr (std::is_same_v<BVHType, KH_LBVH>)
        {
            bool bIsLeaf = BVH.IsLeafNode(i);
            LBVHNodeEncoded[i].Param2 = glm::ivec4(bIsLeaf ? 1 : 0, i, 1, 0);
        }
        LBVHNodeEncoded[i].AABB_MinPos = glm::vec4(Node.AABB.MinPos, 1.0);
        LBVHNodeEncoded[i].AABB_MaxPos = glm::vec4(Node.AABB.MaxPos, 1.0);
    }
    return LBVHNodeEncoded;
}

template <typename BVHType>
std::vector<KH_PrimitiveEncoded> KH_Scene<BVHType>::EncodeBVHPrimitives()
{
    std::vector<KH_PrimitiveEncoded> Encoded;

    PrimitiveCount = BVH.PrimitiveCount;

    Encoded.reserve(PrimitiveCount);

    for (auto& Primitive : BVH.Primitives)
    {
        Primitive.Primitive->EncodePrimitives(Encoded, Primitive.MaterialSlotID);
    }

    return Encoded;
}

template <typename BVHType>
void KH_Scene<BVHType>::UpdateAABB()
{
    for (auto& Object : Objects)
        AABB.Merge(Object.Object->GetAABB());
}


template<typename SceneType>
class KH_ExampleScenes : public KH_Singleton<KH_ExampleScenes<SceneType>>
{
    friend class KH_Singleton<KH_ExampleScenes<SceneType>>;

private:
    KH_ExampleScenes() {
        InitBunny();
        InitDebugBox();
        InitSingleTriangle();
    }
    ~KH_ExampleScenes() override = default;

    void InitSingleTriangle();
    void InitDebugBox();
    void InitBunny();

public:
    SceneType Bunny;
    SceneType SingleTriangle;
    SceneType DebugBox;
};

using KH_BVHExampleScenes = KH_ExampleScenes<KH_BVHScene>;
using KH_FlatBVHExampleScenes = KH_ExampleScenes<KH_FlatBVHScene>;
using KH_LBVHExampleScenes = KH_ExampleScenes<KH_LBVHScene>;
using KH_GpuLBVHExampleScenes = KH_ExampleScenes<KH_GpuLBVHScene>;

template <typename SceneType>
void KH_ExampleScenes<SceneType>::InitSingleTriangle()
{
    if constexpr (requires { SceneType::BVH.bIsBuildOnCPU; })
    {
        if constexpr (decltype(SceneType::BVH)::bIsBuildOnCPU) {
            SingleTriangle.BVH.MaxBVHDepth = 11;
            SingleTriangle.BVH.MaxLeafPrimitives = 1;
            SingleTriangle.BVH.BuildMode = KH_BVH_BUILD_MODE::SAH;
        }
    }

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

template <typename SceneType>
void KH_ExampleScenes<SceneType>::InitBunny()
{
    if constexpr (requires { SceneType::BVH.bIsBuildOnCPU; })
    {
        if constexpr (decltype(SceneType::BVH)::bIsBuildOnCPU) {
		    Bunny.BVH.MaxBVHDepth = 5;
		    Bunny.BVH.MaxLeafPrimitives = 1;

		    Bunny.BVH.BuildMode = KH_BVH_BUILD_MODE::SAH;
        }
    }

    KH_BRDFMaterial Material1;
    Material1.BaseColor = glm::vec3(1.0, 0.0, 0.0);

    KH_BRDFMaterial Material2;
    Material2.BaseColor = glm::vec3(0.0, 1.0, 0.0);

    KH_BRDFMaterial Material3;
    Material3.Emissive = glm::vec3(10.0, 10.0, 10.0);
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

template <typename SceneType>
void KH_ExampleScenes<SceneType>::InitDebugBox()
{
    if constexpr (requires { SceneType::BVH.bIsBuildOnCPU; })
    {
        if constexpr (decltype(SceneType::BVH)::bIsBuildOnCPU) {
		    DebugBox.BVH.MaxBVHDepth = 5;
		    DebugBox.BVH.MaxLeafPrimitives = 1;
        }
    }

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


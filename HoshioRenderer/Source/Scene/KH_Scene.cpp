#include "KH_Scene.h"
#include "Editor/KH_Editor.h"
#include "Pipeline/KH_Shader.h"
#include "Utils/KH_DebugUtils.h"

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

std::vector<KH_TriangleEncoded> KH_SceneBase::EncodeTriangles() const
{
    const int nTriangles = Triangles.size();
    std::vector<KH_TriangleEncoded> TriangleEncodeds(nTriangles);

    for (int i = 0; i < nTriangles; i++)
    {
        TriangleEncodeds[i].P1 = glm::vec4(Triangles[i].P1, 1.0);
        TriangleEncodeds[i].P2 = glm::vec4(Triangles[i].P2, 1.0);
        TriangleEncodeds[i].P3 = glm::vec4(Triangles[i].P3, 1.0);

        TriangleEncodeds[i].N1 = glm::vec4(Triangles[i].N1, 1.0);
        TriangleEncodeds[i].N2 = glm::vec4(Triangles[i].N2, 1.0);
        TriangleEncodeds[i].N3 = glm::vec4(Triangles[i].N3, 1.0);

        TriangleEncodeds[i].MaterialSlot = glm::ivec4(Triangles[i].MaterialSlot, 0.0, 0.0, 0.0);
    }

    return TriangleEncodeds;
}

std::vector<KH_BRDFMaterialEncoded> KH_SceneBase::EncodeBSDFMaterials() const
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

void KH_SceneBase::LoadObj(const std::string& path, float scale, int MaterialSlot)
{
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path)) {
        std::cerr << "TinyObjReader Error: " << reader.Error() << std::endl;
        return;
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();


    size_t totalIndices = 0;
    for (const auto& shape : shapes) totalIndices += shape.mesh.indices.size();
    Triangles.reserve(totalIndices / 3);

    std::vector<glm::vec3> vertexNormals(attrib.vertices.size() / 3, glm::vec3(0.0f));

    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            tinyobj::index_t idx0 = shape.mesh.indices[index_offset + 0];
            tinyobj::index_t idx1 = shape.mesh.indices[index_offset + 1];
            tinyobj::index_t idx2 = shape.mesh.indices[index_offset + 2];

            glm::vec3 p0(attrib.vertices[3 * idx0.vertex_index + 0], attrib.vertices[3 * idx0.vertex_index + 1], attrib.vertices[3 * idx0.vertex_index + 2]);
            glm::vec3 p1(attrib.vertices[3 * idx1.vertex_index + 0], attrib.vertices[3 * idx1.vertex_index + 1], attrib.vertices[3 * idx1.vertex_index + 2]);
            glm::vec3 p2(attrib.vertices[3 * idx2.vertex_index + 0], attrib.vertices[3 * idx2.vertex_index + 1], attrib.vertices[3 * idx2.vertex_index + 2]);

            glm::vec3 faceNormal = glm::cross(p1 - p0, p2 - p0);

            vertexNormals[idx0.vertex_index] += faceNormal;
            vertexNormals[idx1.vertex_index] += faceNormal;
            vertexNormals[idx2.vertex_index] += faceNormal;

            index_offset += 3;
        }
    }

    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            tinyobj::index_t idx[3];
            glm::vec3 p[3], n[3];

            for (size_t v = 0; v < 3; v++) {
                idx[v] = shape.mesh.indices[index_offset + v];
                p[v] = { attrib.vertices[3 * idx[v].vertex_index + 0], attrib.vertices[3 * idx[v].vertex_index + 1], attrib.vertices[3 * idx[v].vertex_index + 2] };

                n[v] = glm::normalize(vertexNormals[idx[v].vertex_index]);
            }

            Triangles.emplace_back(scale * p[0], scale * p[1], scale * p[2], n[0], n[1], n[2], MaterialSlot);

            AABB.Merge(Triangles.back());

            index_offset += 3;
        }
    }
}

void KH_SceneBase::AddTriangles(const KH_Triangle& Triangle)
{
    Triangles.emplace_back(Triangle);
    AABB.Merge(Triangles.back());
}

void KH_SceneBase::Clear()
{
    Triangles.clear();
    AABB.Reset();
}

void KH_Scene<KH_GpuLBVH>::SetSSBOs()
{
    std::vector<KH_TriangleEncoded> TriangleEncodeds = EncodeTriangles();
    std::vector<KH_BRDFMaterialEncoded> BSDFMaterialEncodeds = EncodeBSDFMaterials();

    Triangle_SSBO.SetData(TriangleEncodeds);
    Material_SSBO.SetData(BSDFMaterialEncodeds);
}

void KH_Scene<KH_GpuLBVH>::SetRayTracingParam(KH_Shader& Shader)
{
    Shader.Use();

    const KH_Camera& Camera = KH_Editor::Instance().Camera;

    Triangle_SSBO.Bind();
    BVH.Morton3DSSBO.Bind();
    BVH.LBVHNodeSSBO.Bind();
    BVH.AuxiliarySSBO.Bind();
    Material_SSBO.Bind();

    Shader.SetInt("LBVHNodeCount", BVH.LBVHNodeCount);

    SetAndBindCameraParamUB0();
}

void KH_Scene<KH_GpuLBVH>::BindAndBuild()
{
    SetSSBOs();
    BVH.BindAndBuild(this);
}

void KH_Scene<KH_GpuLBVH>::Render()
{
    SetRayTracingParam(KH_ExampleShaders::Instance().RayTracingShader2_1);

    KH_Editor::Instance().BindCanvasFramebuffer();

    glBindVertexArray(KH_DefaultModels::Instance().Plane.VAO);
    glDrawElements(KH_DefaultModels::Instance().Plane.GetDrawMode(), static_cast<GLsizei>(KH_DefaultModels::Instance().Plane.GetIndicesSize()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    KH_Editor::Instance().UnbindCanvasFramebuffer();
}

#pragma once

#include "KH_Common.h"

#include "KH_Shape.h"
#include "Hit/KH_Ray.h"

class KH_Shader;
class KH_Camera;
class KH_Texture;

struct KH_Vertex {
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Normal = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 Bitangent = glm::vec3(0.0f, 1.0f, 0.0f);;
    glm::vec2 UV = glm::vec2(0.0f);
    //glm::vec4 Color;
    //float Value;
};

class KH_Mesh{
    friend class KH_Model;
public:
    KH_Mesh() = default;
    KH_Mesh(std::vector<KH_Vertex>& Vertices, std::vector<unsigned int>& Indices, std::vector<KH_Texture>& Textures, GLenum DrawMode = GL_TRIANGLES);
    KH_Mesh(const KH_Mesh&) = delete;
    KH_Mesh& operator=(const KH_Mesh&) = delete;
    KH_Mesh(KH_Mesh&& other) noexcept;
    KH_Mesh& operator=(KH_Mesh&& other) noexcept;
    ~KH_Mesh();

    void Create(std::vector<KH_Vertex>& Vertices, std::vector<unsigned int>& Indices, std::vector<KH_Texture>& Textures, GLenum DrawMode = GL_TRIANGLES);
    void SetDrawMode(GLenum DrawMode);

    const unsigned int GetVAO() const;
    GLsizei GetNumIndices() const;
    uint32_t GetPrimitiveCount() const;

    void EncodePrimitives(std::vector<KH_PrimitiveEncoded>& outPrimitives,
        int MaterialSlotID,
        const glm::mat4& ModelMatrix,
        const glm::mat3& NormalMatrix) const;

    void CollectPrimitives(std::vector<KH_ScenePrimitive>& outPrimitives,
        int MaterialSlotID,
        const glm::mat4& ModelMatrix,
        const glm::mat3& NormalMatrix) const;

    void CollectPrimitiveAABBCenters(std::vector<glm::vec4>& outCenters,
        const glm::mat4& ModelMatrix) const;

    const KH_AABB& GetLocalAABB() const;

    void Render(KH_Shader& Shader);

private:
    unsigned int VAO, VBO, EBO;
    GLenum DrawMode = GL_TRIANGLES;

    std::vector<KH_Vertex> Vertices;
    std::vector<unsigned int> Indices;
    std::vector<KH_Texture> Textures;

    

    KH_AABB LocalAABB;

    void SetupMesh();
    void UpdateLocalAABB();

};





#pragma once

#include "KH_Mesh.h"
#include "Pipeline/KH_Texture.h"

class KH_Model : public KH_Object
{
public:
    KH_Model() = default;
    KH_Model(const std::string& path);
    KH_Model(const KH_Model&) = delete;
    KH_Model& operator=(const KH_Model&) = delete;
    KH_Model(KH_Model&& other) noexcept;
    KH_Model& operator=(KH_Model&& other) noexcept;
    void LoadModel(const std::string& path);
    //void AddMesh(const KH_Mesh& mesh);
    void AddMesh(KH_Mesh&& mesh);
    virtual uint32_t GetPrimitiveCount() const override;
    virtual void EncodePrimitives(std::vector<KH_PrimitiveEncoded>& outPrimitives,
        int MaterialSlotID) const override;
    virtual void CollectPrimitives(std::vector<KH_ScenePrimitive>& outPrimitives,
        int MaterialSlotID) const override;
    virtual void CollectPrimitiveAABBCenters(std::vector<glm::vec4>& outCenters) const override;
    virtual const KH_AABB& GetAABB() const override;

    void Render(KH_Shader& Shader);
private:
    std::vector<KH_Mesh> Meshes;
    std::string Directory;
    KH_AABB AABB;

    void ProcessNode(aiNode* node, const aiScene* scene);
    KH_Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<KH_Texture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type);

    void UpdateAABB();
    void OnTransformChanged() override;
};

class KH_PrimitiveFactory
{
public:
    static KH_Mesh CreatePlaneMesh(float size = 1.0f);
    static KH_Mesh CreateCubeMesh(float size = 1.0f);
    static KH_Mesh CreateEmptyCubeMesh(float size = 1.0f);
    static KH_Model CreatePlane(float size = 1.0f);
    static KH_Model CreateCube(float size = 1.0f);
    static KH_Model CreateEmptyCube(float size = 1.0f);
};

class KH_DefaultModels : public KH_Singleton<KH_DefaultModels>
{
	friend class KH_Singleton<KH_DefaultModels>;
public:
	KH_Mesh Cube;
	KH_Mesh EmptyCube;
	KH_Mesh Plane;
	KH_Model Bunny;

	KH_DefaultModels(const KH_DefaultModels&) = delete;
	KH_DefaultModels& operator=(const KH_DefaultModels&) = delete;

private:
	KH_DefaultModels();
	~KH_DefaultModels() override = default;

	void InitCube();

	void InitEmptyCube();

	void InitPlane();

	void InitBunny();

};
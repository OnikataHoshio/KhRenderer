#pragma once
#include "Hit/KH_BVH.h"
#include "KH_Object.h"

enum class KH_BVH_BUILD_MODE;

struct KH_TriangleEncoded
{
	glm::vec4 P1, P2, P3;
	glm::vec4 N1, N2, N3;
	uint32_t Padding[3];
	uint32_t MaterialSlot = 0;
};

struct KH_BSDFMaterialEncoded
{
	glm::vec4 Emissive;
	glm::vec4 BaseColor;
	glm::vec4 Param1;
	glm::vec4 Param2;
	glm::vec4 Param3;
};

class KH_Scene
{
private:
	void CreateSSBOs();
	void DestroySSBOs();

	GLuint TriangleSSBO = 0;
	GLuint MaterialSSBO = 0;

	void SetRTShaderV1() const;

public:
	KH_Scene() = default;
	KH_Scene(uint32_t MaxBVHDepth, uint32_t MaxLeafTrianglesm, KH_BVH_BUILD_MODE BuildMode);
	~KH_Scene();

	std::vector<KH_Triangle> Triangles;
	std::vector<KH_BSDFMaterial> Materials;
	KH_BVH BVH;

	void LoadObj(const std::string& path, uint32_t MaterialSlot);

	void AddTriangles(KH_Triangle& Triangle);

	void BindAndBuild();

	std::vector<KH_TriangleEncoded> EncodeTriangles();

	std::vector<KH_BSDFMaterialEncoded> EncodeBSDFMaterials();

	void Render();

	void Clear();
};


class KH_ExampleScenes : public KH_Singleton<KH_ExampleScenes>
{
	friend class KH_Singleton<KH_ExampleScenes>;

private:
	KH_ExampleScenes();
	virtual ~KH_ExampleScenes() override = default;

	void InitExampleScene1();

	void InitSingleTriangle();
public:
	KH_Scene SingleTriangle;

	KH_Scene ExampleScene1;


};




#pragma once
#include "KH_AABB.h"

class KH_Shader;
struct KH_Triangle;

enum class KH_BVH_SPLIT_MODE
{
	X_AXIS_SPLIT = 0,
	Y_AXIS_SPLIT = 1,
	Z_AXIS_SPLIT = 2
};

enum class KH_BVH_BUILD_MODE
{
	Base = 0,
	SAH = 1
};

struct KH_BVHHitInfo
{
	bool bIsHit = false;
	float HitTime = std::numeric_limits<float>::max();
	uint32_t BeginIndex = 0;
	uint32_t EndIndex = 0;
};

struct KH_BVHSplitInfo
{
	KH_BVH_SPLIT_MODE SplitMode = KH_BVH_SPLIT_MODE::X_AXIS_SPLIT;
	float Cost = std::numeric_limits<float>::max();
	uint32_t SplitIndex = 0;
};


class KH_BVHNode
{
public:
	std::unique_ptr<KH_BVHNode> Left;
	std::unique_ptr<KH_BVHNode> Right;

	bool bIsLeaf = false;
	int Offset = 0;
	int Size = 0;

	KH_AABB AABB;

	void BuildNode(std::vector<KH_Triangle>& Triangles, uint32_t BeginIndex, uint32_t EndIndex, uint32_t Depth, uint32_t MaxNum, uint32_t MaxDepth);

	void BuildNodeSAH(std::vector<KH_Triangle>& Triangles, uint32_t BeginIndex, uint32_t EndIndex, uint32_t Depth, uint32_t MaxNum, uint32_t MaxDepth);

	void Hit(std::vector<KH_BVHHitInfo>& HitInfos, KH_Ray& Ray);

private:
	static KH_BVH_SPLIT_MODE SelectSplitMode(KH_AABB& AABB);

	static KH_BVHSplitInfo SelectSplitModeSAH(std::vector<KH_Triangle>& Triangles, int BeginIndex, int EndIndex);
};


class KH_BVH
{
public:
	std::unique_ptr<KH_BVHNode> Root;

	std::vector<KH_Triangle>* Triangles = nullptr;
	uint32_t MaxBVHDepth = 8;
	uint32_t MaxLeafTriangles = 8;

	std::vector<glm::mat4> ModelMats;
	uint32_t MatCount = 0;

	KH_BVH_BUILD_MODE BuildMode = KH_BVH_BUILD_MODE::Base;

	unsigned int ModelMats_SSBO = 0;

	KH_BVH();
	KH_BVH(uint32_t MaxBVHDepth, uint32_t MaxLeafTriangles);
	~KH_BVH() = default;

	//void LoadObj(const std::string& path);

	void BindAndBuild(std::vector<KH_Triangle>& Triangles);

	void RenderAABB(KH_Shader Shader, glm::vec3 Color);

	std::vector<KH_BVHHitInfo> Hit(KH_Ray& Ray);


private:
	void FillModelMatrices(uint32_t TargetDepth);

	void FillModelMatrices_Inner(KH_BVHNode* BVHNode, uint32_t CurrentDepth, uint32_t TargetDepth);

	void UpdateModelMatsSSBO();

	void BuildBVH();

};

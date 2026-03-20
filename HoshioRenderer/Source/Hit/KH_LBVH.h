#pragma once

#include "KH_AABB.h"
#include "Pipeline/KH_Buffer.h"
#include "KH_BVH.h"
#include "Pipeline/KH_Shader.h"

class KH_Triangle;

struct KH_LBVHNodeEncoded
{
	glm::ivec4 Param1; //(Left, Right, bIsLeaf, )
	glm::ivec4 Param2; //(Front, Back, , )
	glm::vec4 AABB_MinPos;
	glm::vec4 AABB_MaxPos;
};


#define KH_LBVH_NULL_NODE -1

class KH_LBVHNode
{
public:
	int Left = KH_LBVH_NULL_NODE;
	int Right = KH_LBVH_NULL_NODE;
	KH_AABB AABB;
	glm::ivec2 Range = glm::ivec2(0);

	void Hit(std::vector<KH_BVHHitInfo>& HitInfos, std::vector<KH_LBVHNode>& LBVHNodes, uint32_t TriangleCount, int NodeID, KH_Ray& Ray);
};

class KH_GpuLBVH;

class KH_LBVH : public KH_IBVH
{
	friend class KH_GpuLBVH;
public:
	KH_LBVH() = default;
	~KH_LBVH() override = default;

	KH_AABB AABB;
	int Root;

	std::vector<KH_LBVHNode> BVHNodes;
	std::vector<uint32_t> SortedIndices;

	void BindAndBuild(std::vector<KH_Triangle>& Triangles) override;

	void BindAndBuild(std::vector<KH_Triangle>& Triangles, KH_AABB AABB);

	bool IsLeafNode(int NodeID) const;

	int GetTriangleIndices(int NodeID) const;

	std::vector<KH_BVHHitInfo> Hit(KH_Ray& Ray) override;

private:
	void SortTriangleIndices();

	void FillDeltaBuffer();

	void InitLBVHNodes();

	void BuildBVH() override;

	int ComputeDelta(int i);

	bool IsLeftChild(int NodeID) const;

	bool IsRootNode(int NodeID) const;

	bool IsAllDataReady() const;

	bool CheckTriangles() const;

	bool CheckAABB() const;

	void FillModelMatrices(uint32_t TargetDepth) override;

	uint32_t TriangleCount = 0;

	std::vector<uint64_t> Tri2Mortons;

	std::vector<int> DeltaBuffer;

	std::vector<int> AtomicTags;
};


template<typename BVHType> class KH_Scene;

template<> class KH_Scene<KH_GpuLBVH>;

class KH_GpuLBVH
{
	friend class KH_Scene<KH_GpuLBVH>;

public:
	KH_GpuLBVH();

	void BindAndBuild(KH_Scene<KH_GpuLBVH>* Scene);

	void Initialize();

	void RunGenerateMorton3D() const;

	void RunRadixSort2uiv() const;

	void RunPrecomputeDelta() const;

	void RunBuildLBVH() const;

	void BuildLBVH();

	void RenderAABB(const KH_Shader& Shader, glm::vec3 Color) const;

	void CheckAllData(KH_LBVH& CPU_LBVH) const;

	KH_SSBO<glm::vec4> CentersSSBO;
	KH_SSBO<glm::uvec2> Morton3DSSBO;

	int ElementCount = 0;

	static constexpr bool bIsBuildOnCPU = false;

private:
#define KH_LBVH_RADIXSORT_THREAD_NUM 256
#define KH_LBVH_GPUBUILDER_THREAD_NUM 512

	int LBVHNodeCount = 0;
	int LBVHBuilder_NumBlocks = 0;
	int RadixSort_NumBlocks = 0;
	int Scan_NumBlocks = 0;

	KH_Scene<KH_GpuLBVH>* pScene = nullptr;
	KH_AABB AABB;

	std::vector<glm::mat4> ModelMats;
	KH_SSBO<glm::mat4> ModelMats_SSBO;

	KH_SSBO<glm::uvec2> RadixSort_LocalShuffleSSBO;
	KH_SSBO<glm::uvec4> RadixSort_BlockSumSSBO;
	KH_SSBO<glm::uvec4> Scan_ScanSSBO;
	KH_SSBO<glm::uvec4> Scan_BlockSumSSBO;

	KH_SSBO<int> AuxiliarySSBO;
	KH_SSBO<KH_LBVHNodeEncoded> LBVHNodeSSBO;
	KH_SSBO<int> AtomicFlagSSBO;

	KH_Shader GenerateMorton3D_Shader;
	KH_Shader RadixSort_Pass1_Shader;
	KH_Shader RadixSort_Pass2_Shader;
	KH_Shader RadixSort_Scan_Pass1_Shader;
	KH_Shader RadixSort_Scan_Pass2_Shader;
	KH_Shader RadixSort_Scan_Pass3_Shader;

	KH_Shader PrecomputeDelta_Shader;
	KH_Shader BuildLBVH_Shader;

	void SetSSBOs();
	void SetSSBOBindings();
	void CreateShaders();

	void FillModelMatrices();
	void RunRadixSort2uiv_Inner(int BitShift) const;

	bool CheckElementCount(KH_LBVH& CPU_LBVH) const;
	bool CheckMorton3D(KH_LBVH& CPU_LBVH) const;
	bool CheckRootAndDelta(KH_LBVH& CPU_LBVH) const;
	bool CheckAtomicFlags(KH_LBVH& CPU_LBVH) const;
	bool CheckLBVHNodes(KH_LBVH& CPU_LBVH) const;

};

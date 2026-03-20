#include "KH_LBVH.h"

#include "GLFW/glfw3native.h"
#include "Scene/KH_Object.h"
#include "Utils/KH_DebugUtils.h"
#include "Utils/KH_Algorithms.h"

#include "Scene/KH_Scene.h"


void KH_LBVHNode::Hit(std::vector<KH_BVHHitInfo>& HitInfos, std::vector<KH_LBVHNode>& LBVHNodes, uint32_t TriangleCount, int NodeID,  KH_Ray& Ray)
{
	KH_AABBHitInfo AABBHit = AABB.Hit(Ray);
	if (!AABBHit.bIsHit) return;

	if (NodeID < TriangleCount)
	{
		KH_BVHHitInfo BVHHitInfo;
		BVHHitInfo.BeginIndex = Range.x;
		BVHHitInfo.EndIndex = Range.y + 1;
		BVHHitInfo.HitTime = AABBHit.HitTime;
		BVHHitInfo.bIsHit = true;
		HitInfos.push_back(BVHHitInfo);
		return;
	}

	if (Left != KH_LBVH_NULL_NODE)  LBVHNodes[Left].Hit(HitInfos, LBVHNodes, TriangleCount, Left, Ray);
	if (Right != KH_LBVH_NULL_NODE) LBVHNodes[Right].Hit(HitInfos, LBVHNodes, TriangleCount, Right,  Ray);
}

void KH_LBVH::BindAndBuild(std::vector<KH_Triangle>& Triangles)
{
	this->Triangles = &Triangles;

	if (!IsAllDataReady())
		return;

	TriangleCount = Triangles.size();

	SortTriangleIndices();
	FillDeltaBuffer();
	InitLBVHNodes();
	BuildBVH();
	FillModelMatrices(MaxBVHDepth);
}

void KH_LBVH::BindAndBuild(std::vector<KH_Triangle>& Triangles, KH_AABB AABB)
{
	this->Triangles = &Triangles;
	this->AABB = AABB;

	if (!IsAllDataReady())
		return;

	TriangleCount = Triangles.size();

	SortTriangleIndices();
	FillDeltaBuffer();
	InitLBVHNodes();
	BuildBVH();
	FillModelMatrices(MaxBVHDepth);
}

std::vector<KH_BVHHitInfo> KH_LBVH::Hit(KH_Ray& Ray)
{
	std::vector<KH_BVHHitInfo> HitInfos;
	if (Root != KH_FLAT_BVH_NULL_NODE)
		BVHNodes[Root].Hit(HitInfos, BVHNodes, TriangleCount, Root, Ray);
	return HitInfos;
}

void KH_LBVH::SortTriangleIndices()
{
	const std::vector<KH_Triangle>& Tris = *Triangles;

	Tri2Mortons.resize(TriangleCount);
	SortedIndices.resize(TriangleCount);

	glm::vec3 AABB_InvSize = AABB.GetInvSize();

	for (int i = 0; i < TriangleCount; i++)
	{
		glm::vec3 Position = Tris[i].Center;
		glm::vec3 p = (Position - AABB.MinPos) * AABB_InvSize;
		p = glm::clamp(p, glm::vec3(0.0f), glm::vec3(1.0f));
		Tri2Mortons[i] = KH_MortonCode::Morton3DFloat_IndexAugmentation(p, i);
	}

	std::ranges::sort(Tri2Mortons);

	for (int i = 0; i < TriangleCount; ++i)
	{
		SortedIndices[i] = static_cast<uint32_t>(Tri2Mortons[i] & 0xFFFFFFFFU);
	}
}

int KH_LBVH::ComputeDelta(int i)
{
	uint64_t first = Tri2Mortons[i - 1];
	uint64_t second = Tri2Mortons[i];
	uint64_t diff = first ^ second;

	if (diff == 0) return 64;

#ifdef _MSC_VER
	unsigned long leading;
	_BitScanReverse64(&leading, diff);
	return 63 - (int)leading;
#else
	return __builtin_clzll(diff);
#endif

}

void KH_LBVH::FillDeltaBuffer()
{
	const int N = TriangleCount;
	DeltaBuffer.assign(N + 1, -1);

	DeltaBuffer[0] = -1;
	DeltaBuffer[N] = -1;

	for (int i = 1; i < N; i++)
	{
		DeltaBuffer[i] = ComputeDelta(i);
	}
}

void KH_LBVH::InitLBVHNodes()
{
	const std::vector<KH_Triangle>& Tris = *Triangles;
	const int N = TriangleCount;
	BVHNodes.resize(2 * N - 1);

	for (int i = 0; i < N; i++)
	{
		BVHNodes[i].Range = glm::ivec2(i, i);
		BVHNodes[i].AABB.Update(Tris[SortedIndices[i]]);
	}

	AtomicTags.assign( N - 1, -1);
}


bool KH_LBVH::IsLeftChild(int NodeID) const
{
	glm::ivec2 Range = BVHNodes[NodeID].Range;
	return DeltaBuffer[Range.x] < DeltaBuffer[Range.y + 1];
}


bool KH_LBVH::IsRootNode(int NodeID) const
{
	const int N = TriangleCount;
	glm::ivec2 Range = BVHNodes[NodeID].Range;
	return Range.x == BVHNodes[0].Range.x && Range.y == BVHNodes[N - 1].Range.y;
}

bool KH_LBVH::IsLeafNode(int NodeID) const
{
	return NodeID < TriangleCount;
}

int KH_LBVH::GetTriangleIndices(int NodeID) const
{
	if (IsLeafNode(NodeID))
		return SortedIndices[NodeID];
	return -1;
}

void KH_LBVH::BuildBVH()
{
	const std::vector<KH_Triangle>& Tris = *Triangles;
	const int N = TriangleCount;

	if (N == 1)
	{
		Root = 0;
		return;
	}


	for (int i = 0; i < N; i++)
	{
		int NodeID = i;
		int ParentLocalID;
		int ParentGlobalID;
		while (true)
		{
			if (IsLeftChild(NodeID))
			{
				ParentLocalID = BVHNodes[NodeID].Range.y;
				AtomicTags[ParentLocalID] += 1;
				ParentGlobalID = ParentLocalID + N;
				BVHNodes[ParentGlobalID].Range.x = BVHNodes[NodeID].Range.x;
				BVHNodes[ParentGlobalID].Left = NodeID;
			}
			else
			{
				ParentLocalID = BVHNodes[NodeID].Range.x - 1;
				AtomicTags[ParentLocalID] += 1;
				ParentGlobalID = ParentLocalID + N;
				BVHNodes[ParentGlobalID].Range.y = BVHNodes[NodeID].Range.y;
				BVHNodes[ParentGlobalID].Right = NodeID;
			}

			BVHNodes[ParentGlobalID].AABB.Merge(BVHNodes[NodeID].AABB);

			if (AtomicTags[ParentLocalID] <= 0)
				break;

			if (IsRootNode(ParentGlobalID))
			{
				Root = ParentGlobalID;
				break;
			}

			NodeID = ParentGlobalID;
		}
	}
}

bool KH_LBVH::IsAllDataReady() const
{
	return CheckTriangles() && CheckAABB();
}

bool KH_LBVH::CheckTriangles() const
{
	if (Triangles == nullptr)
	{
		std::string DebugMessage = std::format("KH_LBVH::CheckTriangles: Triangles array pointer has not been set!");
		LOG_E(DebugMessage);
		return false;
	}

	if (Triangles->empty())
	{
		std::string DebugMessage = std::format("KH_LBVH::CheckTriangles: Triangles array is empty!");
		LOG_E(DebugMessage);
		return false;
	}

	return true;

}

bool KH_LBVH::CheckAABB() const
{
	if (AABB.IsInvalid())
	{
		std::string DebugMessage = std::format("KH_LBVH::CheckAABB: AABB is invalid!");
		LOG_E(DebugMessage);
		return false;
	}
	return true;
}

void KH_LBVH::FillModelMatrices(uint32_t TargetDepth)
{
	ModelMats.clear();
	MatCount = BVHNodes.size();
	for (int i = 0; i < MatCount; i++)
	{
		ModelMats.push_back(BVHNodes[i].AABB.GetModelMatrix());
	}
	ModelMats_SSBO.SetData(ModelMats, GL_STATIC_DRAW);
}


//void KH_LBVH::FillModelMatrices_Inner(int LBVHNodeID, uint32_t CurrentDepth, uint32_t TargetDepth)
//{
//	if (LBVHNodeID == KH_LBVH_NULL_NODE)
//		return;
//
//	if (CurrentDepth == TargetDepth || IsLeafNode(LBVHNodeID))
//	{
//		//TODO : Do some optimization
//		ModelMats.push_back(BVHNodes[LBVHNodeID].AABB.GetModelMatrix());
//		MatCount += 1;
//		return;
//	}
//
//	FillModelMatrices_Inner(BVHNodes[LBVHNodeID].Left, CurrentDepth + 1, TargetDepth);
//	FillModelMatrices_Inner(BVHNodes[LBVHNodeID].Right, CurrentDepth + 1, TargetDepth);
//}


void KH_GpuLBVH::Initialize()
{
	this->ElementCount = pScene->Triangles.size();
	this->LBVHNodeCount = 2 * ElementCount - 1;
	//this->pTriangles = &Triangles;
	this->AABB = pScene->AABB;
	LBVHBuilder_NumBlocks = (ElementCount + KH_LBVH_GPUBUILDER_THREAD_NUM - 1) / KH_LBVH_GPUBUILDER_THREAD_NUM;
	RadixSort_NumBlocks = (ElementCount + KH_LBVH_RADIXSORT_THREAD_NUM - 1) / KH_LBVH_RADIXSORT_THREAD_NUM;
	Scan_NumBlocks = (RadixSort_NumBlocks + KH_LBVH_RADIXSORT_THREAD_NUM - 1) / KH_LBVH_RADIXSORT_THREAD_NUM;

	SetSSBOs();

}

void KH_GpuLBVH::SetSSBOs()
{
	SetSSBOBindings();

	std::vector<glm::vec4> Centers(ElementCount);
	std::vector<KH_Triangle>& Triangles = pScene->Triangles;
	for (int i = 0; i < ElementCount; i++)
		Centers[i] = glm::vec4(Triangles[i].Center, 1.0f);
	CentersSSBO.SetData(Centers, GL_DYNAMIC_DRAW);

	if (Morton3DSSBO.GetCount() != ElementCount)
		Morton3DSSBO.SetData(nullptr, ElementCount, GL_DYNAMIC_DRAW);

	if (RadixSort_LocalShuffleSSBO.GetCount() != ElementCount)
		RadixSort_LocalShuffleSSBO.SetData(nullptr, ElementCount, GL_DYNAMIC_DRAW);

	if (RadixSort_BlockSumSSBO.GetCount() != RadixSort_NumBlocks)
		RadixSort_BlockSumSSBO.SetData(nullptr, RadixSort_NumBlocks, GL_DYNAMIC_DRAW);

	if (Scan_ScanSSBO.GetCount() != RadixSort_NumBlocks)
		Scan_ScanSSBO.SetData(nullptr, RadixSort_NumBlocks, GL_DYNAMIC_DRAW);

	if (Scan_BlockSumSSBO.GetCount() != Scan_NumBlocks)
		Scan_BlockSumSSBO.SetData(nullptr, Scan_NumBlocks, GL_DYNAMIC_DRAW);

	if (AuxiliarySSBO.GetCount() != ElementCount + 2)
		AuxiliarySSBO.SetData(nullptr, ElementCount + 2, GL_DYNAMIC_DRAW);

	if (LBVHNodeSSBO.GetCount() != LBVHNodeCount)
		LBVHNodeSSBO.SetData(nullptr, LBVHNodeCount, GL_DYNAMIC_DRAW);

	std::vector<int> AtomicFlags(ElementCount - 1, -1);
	AtomicFlagSSBO.SetData(AtomicFlags, GL_DYNAMIC_DRAW);
}

void KH_GpuLBVH::SetSSBOBindings()
{
	ModelMats_SSBO.SetBindPoint(0);

	CentersSSBO.SetBindPoint(0);
	Morton3DSSBO.SetBindPoint(1);

	RadixSort_LocalShuffleSSBO.SetBindPoint(2);
	RadixSort_BlockSumSSBO.SetBindPoint(3);
	Scan_ScanSSBO.SetBindPoint(4);
	Scan_BlockSumSSBO.SetBindPoint(5);

	LBVHNodeSSBO.SetBindPoint(2);
	AuxiliarySSBO.SetBindPoint(3);
	AtomicFlagSSBO.SetBindPoint(4);
}

void KH_GpuLBVH::CreateShaders()
{
	GenerateMorton3D_Shader.Create("Assert/Shaders/ComputeShaders/LBVHBuilder/GenerateMorton3D.comp");
	RadixSort_Pass1_Shader.Create("Assert/Shaders/ComputeShaders/LBVHBuilder/RadixSort_Pass1.comp");
	RadixSort_Pass2_Shader.Create("Assert/Shaders/ComputeShaders/LBVHBuilder/RadixSort_Pass2.comp");
	RadixSort_Scan_Pass1_Shader.Create("Assert/Shaders/ComputeShaders/LBVHBuilder/RadixSort_Scan_Pass1.comp");
	RadixSort_Scan_Pass2_Shader.Create("Assert/Shaders/ComputeShaders/LBVHBuilder/RadixSort_Scan_Pass2.comp");
	RadixSort_Scan_Pass3_Shader.Create("Assert/Shaders/ComputeShaders/LBVHBuilder/RadixSort_Scan_Pass3.comp");

	PrecomputeDelta_Shader.Create("Assert/Shaders/ComputeShaders/LBVHBuilder/PrecomputeDelta.comp");
	BuildLBVH_Shader.Create("Assert/Shaders/ComputeShaders/LBVHBuilder/BuildLBVH.comp");
}

void KH_GpuLBVH::FillModelMatrices()
{
	std::vector<KH_LBVHNodeEncoded> LBVHNodeEncodeds;
	LBVHNodeSSBO.GetData(LBVHNodeEncodeds);
	for (int i = 0; i < LBVHNodeCount; i++)
	{
		KH_AABB AABB(LBVHNodeEncodeds[i].AABB_MinPos, LBVHNodeEncodeds[i].AABB_MaxPos);
		ModelMats.push_back(AABB.GetModelMatrix());
	}
	ModelMats_SSBO.SetData(ModelMats, GL_STATIC_DRAW);
}

void KH_GpuLBVH::RunGenerateMorton3D() const
{
	CentersSSBO.Bind();
	Morton3DSSBO.Bind();
	GenerateMorton3D_Shader.Use();
	GenerateMorton3D_Shader.SetInt("uElementCount", ElementCount);
	GenerateMorton3D_Shader.SetUint("uMortonResolution", 1024);
	GenerateMorton3D_Shader.SetVec4("uAABBMinPos", glm::vec4(AABB.MinPos, 1.0f));
	GenerateMorton3D_Shader.SetVec4("uAABBInvSize", glm::vec4(AABB.GetInvSize(), 1.0f));
	glDispatchCompute(LBVHBuilder_NumBlocks, 1, 1);
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

void KH_GpuLBVH::RunRadixSort2uiv() const
{
	Morton3DSSBO.Bind();
	RadixSort_LocalShuffleSSBO.Bind();
	RadixSort_BlockSumSSBO.Bind();
	Scan_ScanSSBO.Bind();
	Scan_BlockSumSSBO.Bind();

	for (int BitShift = 0; BitShift <= 62; BitShift += 2)
		RunRadixSort2uiv_Inner(BitShift);

	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}


void KH_GpuLBVH::RunPrecomputeDelta() const
{
	Morton3DSSBO.Bind();
	AuxiliarySSBO.Bind();
	PrecomputeDelta_Shader.Use();
	PrecomputeDelta_Shader.SetInt("uElementCount", ElementCount);
	glDispatchCompute(LBVHBuilder_NumBlocks, 1, 1);
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

void KH_GpuLBVH::RunBuildLBVH() const
{
	pScene->Triangle_SSBO.Bind();
	Morton3DSSBO.Bind();
	LBVHNodeSSBO.Bind();
	AuxiliarySSBO.Bind();
	AtomicFlagSSBO.Bind();
	BuildLBVH_Shader.Use();
	BuildLBVH_Shader.SetInt("uElementCount", ElementCount);
	glDispatchCompute(LBVHBuilder_NumBlocks, 1, 1);
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

void KH_GpuLBVH::BuildLBVH()
{
	RunGenerateMorton3D();
	RunRadixSort2uiv();
	RunPrecomputeDelta();
	RunBuildLBVH();

	FillModelMatrices();
}

void KH_GpuLBVH::RenderAABB(const KH_Shader& Shader, glm::vec3 Color) const
{
	KH_Editor::Instance().BindCanvasFramebuffer();

	Shader.Use();
	Shader.SetMat4("view", KH_Editor::Instance().Camera.GetViewMatrix());
	Shader.SetMat4("projection", KH_Editor::Instance().Camera.GetProjMatrix());
	Shader.SetVec3("Color", Color);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glDisable(GL_CULL_FACE);

	ModelMats_SSBO.Bind();

	glBindVertexArray(KH_DefaultModels::Instance().EmptyCube.VAO);
	glDrawElementsInstanced(
		KH_DefaultModels::Instance().EmptyCube.GetDrawMode(),
		static_cast<GLsizei>(KH_DefaultModels::Instance().Cube.GetIndicesSize()),
		GL_UNSIGNED_INT,
		0,
		static_cast<GLsizei>(LBVHNodeCount)
	);
	glBindVertexArray(0);

	ModelMats_SSBO.Unbind();

	KH_Editor::Instance().UnbindCanvasFramebuffer();
}

void KH_GpuLBVH::CheckAllData(KH_LBVH& CPU_LBVH) const
{
	if (CheckElementCount(CPU_LBVH))
	{
		CheckMorton3D(CPU_LBVH);
		CheckRootAndDelta(CPU_LBVH);
		CheckAtomicFlags(CPU_LBVH);
		CheckLBVHNodes(CPU_LBVH);
	}
}

KH_GpuLBVH::KH_GpuLBVH()
{
	CreateShaders();
}

void KH_GpuLBVH::BindAndBuild(KH_Scene<KH_GpuLBVH>* Scene)
{
	this->pScene = Scene;
	Initialize();
	BuildLBVH();
}

void KH_GpuLBVH::RunRadixSort2uiv_Inner(int BitShift) const
{
	// --- Pass 1: Radix Sort Local Pass ---
	RadixSort_Pass1_Shader.Use();
	RadixSort_Pass1_Shader.SetInt("uElementCount", ElementCount);
	RadixSort_Pass1_Shader.SetInt("uBitShift", BitShift);
	glDispatchCompute(RadixSort_NumBlocks, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// --- Pass 2: Local Scan Pass---
	RadixSort_Scan_Pass1_Shader.Use();
	RadixSort_Scan_Pass1_Shader.SetInt("uElementCount", RadixSort_NumBlocks);

	glDispatchCompute(Scan_NumBlocks, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// --- Pass 3: Global Scan Pass---
	RadixSort_Scan_Pass2_Shader.Use();
	RadixSort_Scan_Pass2_Shader.SetInt("uBlockCount", Scan_NumBlocks);
	glDispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// --- Pass 4: Add Scan Offset ---
	RadixSort_Scan_Pass3_Shader.Use();
	RadixSort_Scan_Pass3_Shader.SetInt("uElementCount", RadixSort_NumBlocks);
	glDispatchCompute(Scan_NumBlocks, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// --- Pass 5: Add Radix Sort Offset ---
	RadixSort_Pass2_Shader.Use();
	RadixSort_Pass2_Shader.SetInt("uElementCount", ElementCount);
	RadixSort_Pass2_Shader.SetInt("uBitShift", BitShift);
	glDispatchCompute(RadixSort_NumBlocks, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

bool KH_GpuLBVH::CheckElementCount(KH_LBVH& CPU_LBVH) const
{
	if (ElementCount != CPU_LBVH.TriangleCount)
	{
		LOG_E(std::format(
			"KH_GpuLBVH::CheckElementCount: ElementCount({}) is inconsistent with the TriangleCount({}) of CPU_LBVH", ElementCount, CPU_LBVH.TriangleCount
		));
		return false;
	}
	return true;
}

bool KH_GpuLBVH::CheckMorton3D(KH_LBVH& CPU_LBVH) const
{
	std::vector<glm::uvec2> Morton3D;
	Morton3DSSBO.GetData(Morton3D);

	bool bIsConsistent = true;
	for (int i = 0; i < ElementCount; i++)
	{
		const uint64_t JointMorton3D = static_cast<uint64_t>(Morton3D[i].x) << 32u | static_cast<uint64_t>(Morton3D[i].y);
		if (JointMorton3D != CPU_LBVH.Tri2Mortons[i])
		{
			LOG_E(std::format(
				"KH_GpuLBVH::CheckMorton3D: Morton3D[{}]({:#018x}) is inconsistent with the Morton3D[{}]({:#018x}) of CPU_LBVH",
				i, JointMorton3D, i, CPU_LBVH.Tri2Mortons[i]
			));

			bIsConsistent = false;
		}
	}



	if (bIsConsistent)
	{
		LOG_T(std::format(
			"KH_GpuLBVH::CheckMorton3D: Morton3D is consistent with CPU_LBVH"
		));
	}

	return bIsConsistent;
}

bool KH_GpuLBVH::CheckRootAndDelta(KH_LBVH& CPU_LBVH) const
{
	std::vector<int> AuxiliaryData;
	AuxiliarySSBO.GetData(AuxiliaryData);

	bool bIsConsistent = true;

	if (AuxiliaryData[0] != CPU_LBVH.Root)
	{
		LOG_E(std::format(
			"KH_GpuLBVH::CheckRootAndDelta: Root({}) is inconsistent with the Root({}) of CPU_LBVH", AuxiliaryData[0], CPU_LBVH.Root
		));
		bIsConsistent = false;
	}


	if (AuxiliaryData.size() - 1 != CPU_LBVH.DeltaBuffer.size())
	{
		LOG_E(std::format(
			"KH_GpuLBVH::CheckRootAndDelta: Delta size({}) is inconsistent with the Delta size({}) of CPU_LBVH",
			AuxiliaryData.size() - 1, CPU_LBVH.DeltaBuffer.size()
		));
		bIsConsistent = false;
	}

	for (int i = 1; i < AuxiliaryData.size(); i++)
	{
		int real_idx = i - 1;
		if (AuxiliaryData[i] != CPU_LBVH.DeltaBuffer[real_idx])
		{
			LOG_E(std::format(
				"KH_GpuLBVH::CheckRootAndDelta: Delta[{}]({}) is inconsistent with the Delta[{}]({}) of CPU_LBVH",
				real_idx, AuxiliaryData[i], real_idx, CPU_LBVH.DeltaBuffer[real_idx]
			));

			bIsConsistent = false;
		}
	}

	if (bIsConsistent)
	{
		LOG_T(std::format(
			"KH_GpuLBVH::CheckRootAndDelta: RootAndDelta is consistent with CPU_LBVH"
		));
	}

	return bIsConsistent;
}

bool KH_GpuLBVH::CheckAtomicFlags(KH_LBVH& CPU_LBVH) const
{
	std::vector<int> AtomicFlags;
	AtomicFlagSSBO.GetData(AtomicFlags);

	bool bIsConsistent = true;

	if (AtomicFlags.size() != CPU_LBVH.AtomicTags.size())
	{
		LOG_E(std::format(
			"KH_GpuLBVH::CheckAtomicFlags: AtomicFlags Size({}) is inconsistent with the AtomicTags Size({}) of CPU_LBVH",
			AtomicFlags.size(), CPU_LBVH.AtomicTags.size()
		));
		return false;
	}

	//for (int i = 0; i < AtomicFlags.size();i++)
	//{
	//	if (AtomicFlags[i] != 2)
	//	{
	//		LOG_E(std::format(
	//			"KH_GpuLBVH::CheckAtomicFlags: AtomicFlags[{}]({}) isincorrected", i, AtomicFlags[i]
	//		));
	//		bIsConsistent = false;
	//	}
	//}

	if (bIsConsistent)
	{
		LOG_T(std::format(
			"KH_GpuLBVH::CheckAtomicFlags: AtomicFlags is consistent with CPU_LBVH"
		));
	}

	return bIsConsistent;
}

bool KH_GpuLBVH::CheckLBVHNodes(KH_LBVH& CPU_LBVH) const
{
	std::vector<KH_LBVHNodeEncoded> LBVHNodes;
	LBVHNodeSSBO.GetData(LBVHNodes);

	bool bIsConsistent = true;

	if (LBVHNodes.size() != CPU_LBVH.BVHNodes.size())
	{
		LOG_E(std::format(
			"KH_GpuLBVH::CheckLBVHNodes: LBVHNodes Size({}) is inconsistent with the BVHNodes Size({}) of CPU_LBVH",
			LBVHNodes.size(), CPU_LBVH.BVHNodes.size()
		));
		return false;
	}

	for (int i = 0; i < LBVHNodes.size(); i++)
	{
		if (glm::ivec2(LBVHNodes[i].Param2) != CPU_LBVH.BVHNodes[i].Range)
		{
			LOG_E(std::format(
				"KH_GpuLBVH::CheckLBVHNodes: LBVHNodes[{}] Range({}) is inconsistent with the BVHNodes({}) Range({}) of CPU_LBVH",
				i, glm::ivec2(LBVHNodes[i].Param2),
				i, CPU_LBVH.BVHNodes[i].Range
			));
			bIsConsistent = false;
		}
	}

	for (int i = 0; i < LBVHNodes.size(); i++)
	{
		if (glm::vec3(LBVHNodes[i].AABB_MinPos) != CPU_LBVH.BVHNodes[i].AABB.MinPos ||
			glm::vec3(LBVHNodes[i].AABB_MaxPos) != CPU_LBVH.BVHNodes[i].AABB.MaxPos)
		{
			LOG_E(std::format(
				"KH_GpuLBVH::CheckLBVHNodes: LBVHNodes[{}] AABB(min({}), max({})) is inconsistent with the BVHNodes({}) AABB(min({}), max({})) of CPU_LBVH",
				i, glm::vec3(LBVHNodes[i].AABB_MinPos), glm::vec3(LBVHNodes[i].AABB_MaxPos),
				i, CPU_LBVH.BVHNodes[i].AABB.MinPos, CPU_LBVH.BVHNodes[i].AABB.MaxPos
			));
			bIsConsistent = false;
		}
	}


	if (bIsConsistent)
	{
		LOG_T(std::format(
			"KH_GpuLBVH::CheckLBVHNodes: LBVHNodes is consistent with CPU_LBVH"
		));
	}

	return bIsConsistent;
}


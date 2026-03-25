#pragma once
#include "KH_Common.h"
#include "Scene/KH_Scene.h"

struct KH_HitResult;
class KH_Ray;
class KH_BVH;

enum class KH_PRIMITIVE_TRAVERSAL_MODE
{
	BASE = 0,
	BASE_BVH = 1
};

class KH_RendererBase
{
public:
	void Render(KH_BVHScene& Scene);

	KH_PRIMITIVE_TRAVERSAL_MODE TraversalMode = KH_PRIMITIVE_TRAVERSAL_MODE::BASE_BVH;

private:
	uint32_t SAMPLE_COUNT = 1;

private:
	KH_HitResult CastRay(KH_BVHScene& Scene, KH_Ray& Ray);

	glm::vec3 PathTracing(KH_BVHScene& Scene, KH_Ray& Ray, unsigned int Depth);

	KH_HitResult CastRayBase(KH_BVHScene& Scene, KH_Ray& Ray);

	KH_HitResult CastRayBVH(KH_BVHScene& Scene, KH_Ray& Ray);

	void SaveImage(const char* FilePath, const int Width, const int Height, const int Channel, const void* Data);
};

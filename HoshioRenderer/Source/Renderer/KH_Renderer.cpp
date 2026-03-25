#include "KH_Renderer.h"
#include "Scene/KH_Shape.h"
#include "Hit/KH_Ray.h"
#include "Hit/KH_BVH.h"
#include "Utils/KH_RandomUtils.h"
#include "utils/KH_DebugUtils.h"
#include "Pipeline/KH_Material.h"
#include "Editor/KH_Editor.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Scene/KH_Scene.h"
#include "stb_image/stb_image.h"
#include "stb_image/stb_image_write.h"

KH_BaseMaterial& Material = KH_DefaultMaterial::Instance().BaseMaterial1;

void KH_RendererBase::Render(KH_BVHScene& Scene)
{
	const int Width = KH_Editor::GetCanvasWidth();
	const int Height = KH_Editor::GetCanvasHeight();
	const int Channel = 3;
	const unsigned int TotalBytes = Width * Height * Channel;
	std::vector<unsigned char> PixelData(TotalBytes);

	const glm::vec3 EyePos = KH_Editor::Instance().Camera.Position;

	auto ToByte = [](float c) -> unsigned char {
		return (unsigned char)std::clamp(int(c * 255.0f), 0, 255);
		};


	omp_set_num_threads(64);
	#pragma omp parallel for
	for (int j = 0; j < Height; j++)
	{
		for (int i = 0; i < Width; i++)
		{
			KH_Ray Ray = KH_Editor::Instance().Camera.GetRay(i, j);

			glm::vec3 Color = glm::vec3(0.0f);

			for (int k = 0; k < SAMPLE_COUNT; k++)
			{
				Color += PathTracing(Scene, Ray, 0);
			}

			Color = float(SAMPLE_COUNT) * Color;

			int pixelOffset = (j * Width + i) * Channel;
			PixelData[pixelOffset + 0] = ToByte(Color.r);
			PixelData[pixelOffset + 1] = ToByte(Color.g);
			PixelData[pixelOffset + 2] = ToByte(Color.b);
		}
	}

	SaveImage("output.png", Width, Height, Channel, PixelData.data());
}

KH_HitResult KH_RendererBase::CastRay(KH_BVHScene& Scene, KH_Ray& Ray)
{
	KH_HitResult HitResult;
	switch (TraversalMode)
	{
	case KH_PRIMITIVE_TRAVERSAL_MODE::BASE:
		HitResult = CastRayBase(Scene, Ray);
		break;
	case KH_PRIMITIVE_TRAVERSAL_MODE::BASE_BVH:
		HitResult = CastRayBVH(Scene, Ray);
		break;
	}
	return HitResult;
}

glm::vec3 KH_RendererBase::PathTracing(KH_BVHScene& Scene, KH_Ray& Ray, unsigned int Depth)
{
	if (Depth > 8)
		return glm::vec3(0.0f);

	KH_HitResult HitResult = CastRay(Scene, Ray);

	if (!HitResult.bIsHit)
		return glm::vec3(0.0f);

	if (Material.bIsEmissive)
		return Material.Color;

	float P = 0.8;

	if (KH_RandomUtils::Instance().RandomFloat() > 0.8)
		return glm::vec3(0.0f);


	KH_Ray RandomRay;
	RandomRay.Start = HitResult.HitPoint;
	RandomRay.Direction = KH_RandomUtils::Instance().RandomDirection(HitResult.Normal);
	float cosine = std::max(glm::dot(HitResult.Normal, RandomRay.Direction), 0.0f);

	glm::vec3 Color;

	float RandomValue = KH_RandomUtils::Instance().RandomFloat();

	if (RandomValue < Material.SpecularRate)
	{
		glm::vec3 ReflectDirection = glm::reflect(Ray.Direction, HitResult.Normal);
		RandomRay.Direction = glm::mix(ReflectDirection, RandomRay.Direction, Material.ReflectRoughness);
		Color = PathTracing(Scene, RandomRay, ++Depth) * cosine;
	}
	else if (RandomValue < Material.RefractRate &&
		RandomValue >= Material.SpecularRate)
	{
		float CosTheta = glm::dot(Ray.Direction, HitResult.Normal);
		glm::vec3 Normal = HitResult.Normal;
		float Eta = Material.Eta;

		float Ratio;
		if (CosTheta > 0) { // 从内部射向外部
			Normal = -Normal;
			Ratio = Eta; // n_inside / n_outside
		}
		else { // 从外部射向内部
			Ratio = 1.0f / Eta; // n_outside / n_inside
		}

		glm::vec3 RefractDir = glm::refract(Ray.Direction, Normal, Ratio);

		if (glm::length(RefractDir) < EPS) { // 发生全反射
			RandomRay.Direction = glm::reflect(Ray.Direction, Normal);
		}
		else {
			RandomRay.Direction = RefractDir;
		}

		RandomRay.Start = HitResult.HitPoint - Normal * float(EPS * 100.0f);
		Color = PathTracing(Scene, RandomRay, Depth + 1);
	}
	else {
		glm::vec3 MaterialColor = Material.Color;
		glm::vec3 LightColor = PathTracing(Scene, RandomRay, ++Depth) * cosine;
		Color = MaterialColor * LightColor;
	}

	return Color / float(P);
}

KH_HitResult KH_RendererBase::CastRayBase(KH_BVHScene& Scene, KH_Ray& Ray)
{
	KH_HitResult HitResult, temp;
	for (auto& Primitive : Scene.BVH.Primitives)
	{
		temp = Primitive.Hit(Ray);
		if (temp.bIsHit && temp.Distance < HitResult.Distance)
			HitResult = temp;
	}
	return HitResult;
}

KH_HitResult KH_RendererBase::CastRayBVH(KH_BVHScene& Scene, KH_Ray& Ray)
{
	std::vector<KH_BVHHitInfo> BVHHitInfos = Scene.BVH.Hit(Ray);
	KH_HitResult HitResult, temp;

	for (auto& BVHHitInfo : BVHHitInfos)
	{
		for (int i = BVHHitInfo.BeginIndex; i < BVHHitInfo.EndIndex; i++)
		{
			temp = Scene.BVH.Primitives[i].Hit(Ray);
			if (temp.bIsHit && temp.Distance < HitResult.Distance)
				HitResult = temp;
		}
	}

	return HitResult;

}


void KH_RendererBase::SaveImage(const char* FilePath, const int Width, const int Height, const int Channel,
                                const void* Data)
{
	const int StrideBtyes = Width * Channel;
	stbi_write_png(FilePath, Width, Height, Channel, Data, StrideBtyes);
}

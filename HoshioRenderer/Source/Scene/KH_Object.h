#pragma once

#include "KH_Shape.h"

class KH_Shader;
class KH_Camera;

class KH_Object
{
	//TODO
public:
	KH_Object() = default;
	~KH_Object() = default;

	glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 Scale = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 Rotation = glm::vec3(0.0f, 0.0f, 0.0f);

	glm::mat4 GetModelMatrix();

};

class KH_Model : public KH_Object
{
public:
	KH_Model() = default;
	~KH_Model() = default;

	bool LoadOBJ(const std::string& path);

	void UpdateBuffer();

	void Render(KH_Shader& Shader);

	void SetVertices(std::vector<glm::vec3>& Vertices);

	void SetIndices(std::vector<uint32_t>& Indices);

	void SetDrawMode(GLenum DrawMode);

	uint32_t GetIndicesSize() const;

	uint32_t GetVerticesSize() const;

	GLenum GetDrawMode() const;

	unsigned int VBO = 0;
	unsigned int EBO = 0;
	unsigned int VAO = 0;

private:
	void Clear();

	void DeleteBuffer();

	std::vector<glm::vec3> Vertices;
	std::vector<uint32_t> Indices;

	GLenum DrawMode = GL_TRIANGLES;
};

class KH_DefaultModels
{
public:
	KH_Model Cube;
	KH_Model EmptyCube;
	KH_Model Plane;

	KH_DefaultModels(const KH_DefaultModels&) = delete;
	KH_DefaultModels& operator=(const KH_DefaultModels&) = delete;

	static KH_DefaultModels& Get();

private:
	KH_DefaultModels();
	~KH_DefaultModels() = default;

	void InitCube();

	void InitEmptyCube();

	void InitPlane();
};




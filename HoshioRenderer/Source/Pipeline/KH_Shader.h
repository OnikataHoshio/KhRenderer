#pragma once

#include "KH_Common.h"

enum class KH_SHADER_TYPE
{
    NONE = 0,
    GRAPHICS,
    COMPUTE
};

struct KH_ShaderResource
{
    unsigned int ID = 0;
    KH_SHADER_TYPE Type = KH_SHADER_TYPE::NONE;

    std::string VertexPath;
    std::string FragmentPath;
    std::string ComputePath;

    ~KH_ShaderResource();
};

class KH_Shader
{
public:
    KH_Shader() = default;
    explicit KH_Shader(std::shared_ptr<KH_ShaderResource> resource);

    KH_Shader(const KH_Shader&) = default;
    KH_Shader& operator=(const KH_Shader&) = default;
    KH_Shader(KH_Shader&&) noexcept = default;
    KH_Shader& operator=(KH_Shader&&) noexcept = default;
    ~KH_Shader() = default;

    bool IsValid() const;
    unsigned int GetID() const;
    KH_SHADER_TYPE GetType() const;

    const std::string& GetVertexPath() const;
    const std::string& GetFragmentPath() const;
    const std::string& GetComputePath() const;

    void Use() const;

    void SetInt(const std::string& name, int value) const;
    void SetUint(const std::string& name, uint32_t value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;
    void SetUvec2(const std::string& name, const glm::uvec2& value) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;

    void PrintActiveUniform() const;

private:
    std::shared_ptr<KH_ShaderResource> Resource;
};

class KH_ShaderManager : public KH_Singleton<KH_ShaderManager>
{
    friend class KH_Singleton<KH_ShaderManager>;

public:
    KH_Shader LoadShader(const std::string& vertexPath, const std::string& fragmentPath);
    KH_Shader LoadComputeShader(const std::string& computePath);

    void GarbageCollect();
    void Clear();

private:
    KH_ShaderManager() = default;
    ~KH_ShaderManager() override = default;

    KH_ShaderManager(const KH_ShaderManager&) = delete;
    KH_ShaderManager& operator=(const KH_ShaderManager&) = delete;

private:
    std::unordered_map<std::string, std::weak_ptr<KH_ShaderResource>> ShaderCache;

private:
    std::string NormalizePath(const std::string& path) const;
    std::string BuildGraphicsShaderKey(const std::string& vertexPath, const std::string& fragmentPath) const;
    std::string BuildComputeShaderKey(const std::string& computePath) const;

    std::shared_ptr<KH_ShaderResource> CreateGraphicsShaderResource(const std::string& normalizedVertexPath,
        const std::string& normalizedFragmentPath);

    std::shared_ptr<KH_ShaderResource> CreateComputeShaderResource(const std::string& normalizedComputePath);

    bool ReadFileToString(const std::string& filePath, std::string& outCode) const;

    unsigned int CompileShader(GLenum shaderType, const char* code, const std::string& debugName) const;
    unsigned int LinkProgram(const std::vector<unsigned int>& shaders, const std::string& debugName) const;

    static void CheckCompileErrors(unsigned int object, const std::string& type, bool isProgram);
};

class KH_ShaderHelper
{
public:
    static void SetupCommonMatrices(KH_Shader& Shader,
        const glm::mat4& model,
        const glm::mat4& view,
        const glm::mat4& projection);

    static void SetupTestShader(KH_Shader& Shader, glm::vec3 Color);
};

class KH_ExampleShaders : public KH_Singleton<KH_ExampleShaders>
{
    friend class KH_Singleton<KH_ExampleShaders>;

private:
    KH_ExampleShaders();
    virtual ~KH_ExampleShaders() override = default;

    void InitShaders();
    void PrintShaderLoadMessage(std::string ShaderName);

public:
    KH_Shader TestShader;
    KH_Shader AABBShader;
    KH_Shader TestCanvasShader;
    KH_Shader RayTracingShader1_0;
    KH_Shader RayTracingShader1_1;
    KH_Shader RayTracingShader1_2;
    KH_Shader RayTracingShader1_3;
    KH_Shader RayTracingShader2_0;
    KH_Shader RayTracingShader2_1;
    KH_Shader RayTracingShader2_2;
    KH_Shader RayTracingShader2_3;
    KH_Shader RayTracingShader2_4;
    KH_Shader DisneyBRDF_0;
    KH_Shader DisneyBRDF_1;
    KH_Shader DisneyBRDF_2;

    KH_Shader GammaCorrectionShader;
    KH_Shader DrawSobolShader;
};
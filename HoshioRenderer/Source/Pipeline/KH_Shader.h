#pragma once

#include "KH_Common.h"

class KH_Shader {
public:
    unsigned int ID = 0;

    KH_Shader() = default;
    KH_Shader(const char* vertexPath, const char* fragmentPath);
    KH_Shader(const char* computePath);
    ~KH_Shader();

    void Create(const char* vertexPath, const char* fragmentPath);
    void Create(const char* computePath);
    void Use() const;

    void SetInt(const std::string& name, int value) const;
    void SetUint(const std::string& name, uint32_t value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;
    void SetUvec2(const std::string& name, const glm::uvec2& value) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;

    void PrintActiveUniform();

private:
    static void CheckCompileErrors(unsigned int shader, std::string type);
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
};
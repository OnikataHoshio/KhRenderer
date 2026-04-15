#include "KH_Shader.h"
#include "Utils/KH_DebugUtils.h"
#include "Editor/KH_Editor.h"

KH_ShaderResource::~KH_ShaderResource()
{
    if (ID != 0)
    {
        glDeleteProgram(ID);
        ID = 0;
    }
}

KH_Shader::KH_Shader(std::shared_ptr<KH_ShaderResource> resource)
    : Resource(std::move(resource))
{
}

bool KH_Shader::IsValid() const
{
    return Resource != nullptr && Resource->ID != 0;
}

unsigned int KH_Shader::GetID() const
{
    return Resource ? Resource->ID : 0;
}

KH_SHADER_TYPE KH_Shader::GetType() const
{
    return Resource ? Resource->Type : KH_SHADER_TYPE::NONE;
}

const std::string& KH_Shader::GetVertexPath() const
{
    static std::string Empty;
    return Resource ? Resource->VertexPath : Empty;
}

const std::string& KH_Shader::GetFragmentPath() const
{
    static std::string Empty;
    return Resource ? Resource->FragmentPath : Empty;
}

const std::string& KH_Shader::GetComputePath() const
{
    static std::string Empty;
    return Resource ? Resource->ComputePath : Empty;
}

void KH_Shader::Use() const
{
    if (!IsValid())
        return;

    glUseProgram(Resource->ID);
}

void KH_Shader::SetInt(const std::string& name, int value) const
{
    if (!IsValid())
        return;

    GLint location = glGetUniformLocation(Resource->ID, name.c_str());
    glUniform1i(location, value);
}

void KH_Shader::SetUint(const std::string& name, uint32_t value) const
{
    if (!IsValid())
        return;

    GLint location = glGetUniformLocation(Resource->ID, name.c_str());
    glUniform1ui(location, value);
}

void KH_Shader::SetFloat(const std::string& name, float value) const
{
    if (!IsValid())
        return;

    GLint location = glGetUniformLocation(Resource->ID, name.c_str());
    glUniform1f(location, value);
}

void KH_Shader::SetMat4(const std::string& name, const glm::mat4& mat) const
{
    if (!IsValid())
        return;

    GLint location = glGetUniformLocation(Resource->ID, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
}

void KH_Shader::SetUvec2(const std::string& name, const glm::uvec2& value) const
{
    if (!IsValid())
        return;

    GLint location = glGetUniformLocation(Resource->ID, name.c_str());
    glUniform2uiv(location, 1, &value[0]);
}

void KH_Shader::SetVec2(const std::string& name, const glm::vec2& value) const
{
    if (!IsValid())
        return;

    GLint location = glGetUniformLocation(Resource->ID, name.c_str());
    glUniform2fv(location, 1, &value[0]);
}

void KH_Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    if (!IsValid())
        return;

    GLint location = glGetUniformLocation(Resource->ID, name.c_str());
    glUniform3fv(location, 1, &value[0]);
}

void KH_Shader::SetVec4(const std::string& name, const glm::vec4& value) const
{
    if (!IsValid())
        return;

    GLint location = glGetUniformLocation(Resource->ID, name.c_str());
    glUniform4fv(location, 1, &value[0]);
}

void KH_Shader::PrintActiveUniform() const
{
    if (!IsValid())
        return;

    GLint count = 0;
    glGetProgramiv(Resource->ID, GL_ACTIVE_UNIFORMS, &count);

    std::cout << "Program ID: " << Resource->ID << std::endl;
    std::cout << "Active uniforms: " << count << std::endl;

    for (GLint i = 0; i < count; ++i)
    {
        char name[256];
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;

        glGetActiveUniform(Resource->ID, i, sizeof(name), &length, &size, &type, name);
        GLint loc = glGetUniformLocation(Resource->ID, name);
        std::cout << i << ": " << name << ", loc = " << loc << std::endl;
    }
}

KH_Shader KH_ShaderManager::LoadShader(const std::string& vertexPath, const std::string& fragmentPath)
{
    const std::string normalizedVertexPath = NormalizePath(vertexPath);
    const std::string normalizedFragmentPath = NormalizePath(fragmentPath);
    const std::string key = BuildGraphicsShaderKey(normalizedVertexPath, normalizedFragmentPath);

    auto it = ShaderCache.find(key);
    if (it != ShaderCache.end())
    {
        if (auto shared = it->second.lock())
        {
            return KH_Shader(shared);
        }
    }

    auto resource = CreateGraphicsShaderResource(normalizedVertexPath, normalizedFragmentPath);
    if (!resource || resource->ID == 0)
    {
        return KH_Shader();
    }

    ShaderCache[key] = resource;
    return KH_Shader(resource);
}

KH_Shader KH_ShaderManager::LoadComputeShader(const std::string& computePath)
{
    const std::string normalizedComputePath = NormalizePath(computePath);
    const std::string key = BuildComputeShaderKey(normalizedComputePath);

    auto it = ShaderCache.find(key);
    if (it != ShaderCache.end())
    {
        if (auto shared = it->second.lock())
        {
            return KH_Shader(shared);
        }
    }

    auto resource = CreateComputeShaderResource(normalizedComputePath);
    if (!resource || resource->ID == 0)
    {
        return KH_Shader();
    }

    ShaderCache[key] = resource;
    return KH_Shader(resource);
}

void KH_ShaderManager::GarbageCollect()
{
    for (auto it = ShaderCache.begin(); it != ShaderCache.end();)
    {
        if (it->second.expired())
            it = ShaderCache.erase(it);
        else
            ++it;
    }
}

void KH_ShaderManager::Clear()
{
    ShaderCache.clear();
}

std::string KH_ShaderManager::NormalizePath(const std::string& path) const
{
    try
    {
        std::filesystem::path p(path);
        p = std::filesystem::weakly_canonical(p);
        return p.generic_string();
    }
    catch (...)
    {
        return std::filesystem::path(path).generic_string();
    }
}

std::string KH_ShaderManager::BuildGraphicsShaderKey(const std::string& vertexPath, const std::string& fragmentPath) const
{
    return std::format("GRAPHICS|{}|{}", vertexPath, fragmentPath);
}

std::string KH_ShaderManager::BuildComputeShaderKey(const std::string& computePath) const
{
    return std::format("COMPUTE|{}", computePath);
}

std::shared_ptr<KH_ShaderResource> KH_ShaderManager::CreateGraphicsShaderResource(
    const std::string& normalizedVertexPath,
    const std::string& normalizedFragmentPath)
{
    std::string vertexCode;
    std::string fragmentCode;

    if (!ReadFileToString(normalizedVertexPath, vertexCode))
    {
        LOG_E(std::format("Failed to read vertex shader file: {}", normalizedVertexPath));
        return nullptr;
    }

    if (!ReadFileToString(normalizedFragmentPath, fragmentCode))
    {
        LOG_E(std::format("Failed to read fragment shader file: {}", normalizedFragmentPath));
        return nullptr;
    }

    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexCode.c_str(), normalizedVertexPath);
    if (vertexShader == 0)
        return nullptr;

    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str(), normalizedFragmentPath);
    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return nullptr;
    }

    unsigned int program = LinkProgram({ vertexShader, fragmentShader },
        std::format("{} + {}", normalizedVertexPath, normalizedFragmentPath));

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (program == 0)
        return nullptr;

    auto resource = std::make_shared<KH_ShaderResource>();
    resource->ID = program;
    resource->Type = KH_SHADER_TYPE::GRAPHICS;
    resource->VertexPath = normalizedVertexPath;
    resource->FragmentPath = normalizedFragmentPath;
    return resource;
}

std::shared_ptr<KH_ShaderResource> KH_ShaderManager::CreateComputeShaderResource(const std::string& normalizedComputePath)
{
    std::string computeCode;
    if (!ReadFileToString(normalizedComputePath, computeCode))
    {
        LOG_E(std::format("Failed to read compute shader file: {}", normalizedComputePath));
        return nullptr;
    }

    unsigned int computeShader = CompileShader(GL_COMPUTE_SHADER, computeCode.c_str(), normalizedComputePath);
    if (computeShader == 0)
        return nullptr;

    unsigned int program = LinkProgram({ computeShader }, normalizedComputePath);
    glDeleteShader(computeShader);

    if (program == 0)
        return nullptr;

    auto resource = std::make_shared<KH_ShaderResource>();
    resource->ID = program;
    resource->Type = KH_SHADER_TYPE::COMPUTE;
    resource->ComputePath = normalizedComputePath;
    return resource;
}

bool KH_ShaderManager::ReadFileToString(const std::string& filePath, std::string& outCode) const
{
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        outCode = shaderStream.str();
        return true;
    }
    catch (const std::ifstream::failure& e)
    {
        LOG_E(std::format("ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: {} | {}", filePath, e.what()));
        return false;
    }
}

unsigned int KH_ShaderManager::CompileShader(GLenum shaderType, const char* code, const std::string& debugName) const
{
    unsigned int shader = glCreateShader(shaderType);
    if (shader == 0)
    {
        LOG_E(std::format("glCreateShader failed: {}", debugName));
        return 0;
    }

    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    const std::string typeName =
        shaderType == GL_VERTEX_SHADER ? "VERTEX" :
        shaderType == GL_FRAGMENT_SHADER ? "FRAGMENT" :
        shaderType == GL_COMPUTE_SHADER ? "COMPUTE" : "UNKNOWN";

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        CheckCompileErrors(shader, typeName, false);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

unsigned int KH_ShaderManager::LinkProgram(const std::vector<unsigned int>& shaders, const std::string& debugName) const
{
    unsigned int program = glCreateProgram();
    if (program == 0)
    {
        LOG_E(std::format("glCreateProgram failed: {}", debugName));
        return 0;
    }

    for (unsigned int shader : shaders)
    {
        glAttachShader(program, shader);
    }

    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        CheckCompileErrors(program, "PROGRAM", true);
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

void KH_ShaderManager::CheckCompileErrors(unsigned int object, const std::string& type, bool isProgram)
{
    int success = 0;
    char infoLog[1024] = {};

    if (!isProgram)
    {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(object, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: "
                << type << "\n" << infoLog
                << "\n -- --------------------------------------------------- -- "
                << std::endl;
        }
    }
    else
    {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(object, 1024, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: "
                << type << "\n" << infoLog
                << "\n -- --------------------------------------------------- -- "
                << std::endl;
        }
    }
}

void KH_ShaderHelper::SetupCommonMatrices(KH_Shader& Shader, const glm::mat4& model, const glm::mat4& view,
    const glm::mat4& projection)
{
    Shader.Use();
    Shader.SetMat4("model", model);
    Shader.SetMat4("view", view);
    Shader.SetMat4("projection", projection);
}

void KH_ShaderHelper::SetupTestShader(KH_Shader& Shader, glm::vec3 Color)
{
    Shader.Use();
    Shader.SetMat4("model", glm::mat4(1.0f));
    Shader.SetMat4("view", KH_Editor::Instance().Camera.GetViewMatrix());
    Shader.SetMat4("projection", KH_Editor::Instance().Camera.GetProjMatrix());
    Shader.SetVec3("Color", Color);
}

KH_ExampleShaders::KH_ExampleShaders()
{
    InitShaders();
}

void KH_ExampleShaders::InitShaders()
{
    auto& ShaderManager = KH_ShaderManager::Instance();

    TestShader = ShaderManager.LoadShader("Assert/Shaders/test.vert", "Assert/Shaders/test.frag");
    AABBShader = ShaderManager.LoadShader("Assert/Shaders/DrawAABBs.vert", "Assert/Shaders/DrawAABBs.frag");
    TestCanvasShader = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/DefaultCanvas.frag");
    RayTracingShader1_0 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version1/RayTracing1_0.frag");
    RayTracingShader1_1 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version1/RayTracing1_1.frag");
    RayTracingShader1_2 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version1/RayTracing1_2.frag");
    RayTracingShader1_3 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version1/RayTracing1_3.frag");
    RayTracingShader2_0 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version2/RayTracing2_0.frag");
    RayTracingShader2_1 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version2/RayTracing2_1.frag");
    RayTracingShader2_2 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version2/RayTracing2_2.frag");
    RayTracingShader2_3 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version2/RayTracing2_3.frag");
    RayTracingShader2_4 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version2/RayTracing2_4.frag");
    DisneyBRDF_0 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version3/DisneyBRDF_0.frag");
    DisneyBRDF_1 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version3/DisneyBRDF_1.frag");
    DisneyBRDF_2 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version3/DisneyBRDF_2.frag");
    DisneyBRDF_3 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version3/DisneyBRDF_3.frag");
    DisneyBRDF_4 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version3/DisneyBRDF_4.frag");

    BSSRDF_0 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version4/BSSRDF_0.frag");
    BSSRDF_1 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version4/BSSRDF_1.frag");
    BSSRDF_2 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version4/BSSRDF_2.frag");
    BSSRDF_3 = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version4/BSSRDF_3.frag");

    GammaCorrectionShader = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/PostProcess/GammaCorrection.frag");
    DrawSobolShader = ShaderManager.LoadShader("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/ScenePass/DrawSobol.frag");

    LOG_D(std::format("All Shaders have been loaded."));
}

void KH_ExampleShaders::PrintShaderLoadMessage(std::string ShaderName)
{
    std::string DebugMessage = std::format("Shader ({}) has been loaded.", ShaderName);
    LOG_D(DebugMessage);
}

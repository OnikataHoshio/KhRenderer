#include "KH_Shader.h"
#include "Utils/KH_DebugUtils.h"
#include "Editor/KH_Editor.h"

KH_Shader::KH_Shader(const char* vertexPath, const char* fragmentPath)
{
    Create(vertexPath, fragmentPath);
}

KH_Shader::KH_Shader(const char* computePath)
{
    Create(computePath);
}

KH_Shader::~KH_Shader()
{
    glDeleteProgram(ID);
}

void KH_Shader::Create(const char* vertexPath, const char* fragmentPath)
{
    if (ID != 0)
        return;

    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();

        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "VERTEX");

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "FRAGMENT");

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    CheckCompileErrors(ID, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void KH_Shader::Create(const char* computePath)
{
    if (ID != 0)
        return;

    std::string computeCode;
    std::ifstream cShaderFile;

    cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        cShaderFile.open(computePath);
        std::stringstream cShaderStream;
        cShaderStream << cShaderFile.rdbuf();
        cShaderFile.close();
        computeCode = cShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::COMPUTE_SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
    }

    const char* cShaderCode = computeCode.c_str();

    unsigned int compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &cShaderCode, NULL);
    glCompileShader(compute);
    CheckCompileErrors(compute, "COMPUTE");

    ID = glCreateProgram();
    glAttachShader(ID, compute);
    glLinkProgram(ID);
    CheckCompileErrors(ID, "PROGRAM");

    glDeleteShader(compute);
}

void KH_Shader::Use() const
{
	glUseProgram(ID);
}


void KH_Shader::SetInt(const std::string& name, int value) const
{
    unsigned int location = glGetUniformLocation(ID, name.c_str());
    glUniform1i(location, value);
}

void KH_Shader::SetUint(const std::string& name, uint32_t value) const
{
    unsigned int location = glGetUniformLocation(ID, name.c_str());
    glUniform1ui(location, value);
}

void KH_Shader::SetFloat(const std::string& name, float value) const
{
    unsigned int location = glGetUniformLocation(ID, name.c_str());
    glUniform1f(location, value);
}

void KH_Shader::SetMat4(const std::string& name, const glm::mat4& mat) const
{
    unsigned int location = glGetUniformLocation(ID, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
}

void KH_Shader::SetUvec2(const std::string& name, const glm::uvec2& value) const
{
    unsigned int location = glGetUniformLocation(ID, name.c_str());
    glUniform2uiv(location, 1, &value[0]);
}

void KH_Shader::SetVec2(const std::string& name, const glm::vec2& value) const
{
    unsigned int location = glGetUniformLocation(ID, name.c_str());
    glUniform2fv(location, 1, &value[0]);
}

void KH_Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    unsigned int location = glGetUniformLocation(ID, name.c_str());
    glUniform3fv(location, 1, &value[0]);
}


void KH_Shader::SetVec4(const std::string& name, const glm::vec4& value) const
{
    unsigned int location = glGetUniformLocation(ID, name.c_str());
    glUniform4fv(location, 1, &value[0]);
}

void KH_Shader::PrintActiveUniform()
{
    GLint count = 0;
    glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &count);
    std::cout << "ID: " << ID << std::endl << "Active uniforms: " << count << std::endl;

    for (GLint i = 0; i < count; ++i)
    {
        char name[256];
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveUniform(ID, i, sizeof(name), &length, &size, &type, name);
        GLint loc = glGetUniformLocation(ID, name);
        std::cout << i << ": " << name << ", loc = " << loc << std::endl;
    }
}

void KH_Shader::CheckCompileErrors(unsigned int shader, std::string type)
{
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: "
        	<< type << "\n" << infoLog
        	<< "\n -- --------------------------------------------------- -- "
        	<< std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
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

void KH_ShaderHelper::SetupTestShader(KH_Shader& Shader,  glm::vec3 Color)
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
    TestShader.Create("Assert/Shaders/test.vert", "Assert/Shaders/test.frag");
    AABBShader.Create("Assert/Shaders/DrawAABBs.vert", "Assert/Shaders/DrawAABBs.frag");
    TestCanvasShader.Create("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/DefaultCanvas.frag");
    RayTracingShader1_0.Create("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version1/RayTracing1_0.frag");
    RayTracingShader1_1.Create("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version1/RayTracing1_1.frag");
    RayTracingShader1_2.Create("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version1/RayTracing1_2.frag");
    RayTracingShader1_3.Create("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version1/RayTracing1_3.frag");
    RayTracingShader2_0.Create("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version2/RayTracing2_0.frag");
    RayTracingShader2_1.Create("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version2/RayTracing2_1.frag");
    RayTracingShader2_2.Create("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version2/RayTracing2_2.frag");
    RayTracingShader2_3.Create("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version2/RayTracing2_3.frag");
    RayTracingShader2_4.Create("Assert/Shaders/DefaultCanvas.vert", "Assert/Shaders/RayTracing/Version2/RayTracing2_4.frag");
    LOG_D(std::format("All Shaders have been loaded."));
}

void KH_ExampleShaders::PrintShaderLoadMessage(std::string ShaderName)
{
    std::string DebugMessage = std::format("Shader ({}) has been loaded.", ShaderName);
    LOG_D(DebugMessage);
}

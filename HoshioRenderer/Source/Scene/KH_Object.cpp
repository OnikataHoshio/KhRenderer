#define TINYOBJLOADER_IMPLEMENTATION
#include "KH_Object.h"
#include "Pipeline/KH_Shader.h"
#include "Editor/KH_Camera.h"
#include "Editor/KH_Editor.h"
#include "Utils/KH_Algorithms.h"



glm::mat4 KH_Object::GetModelMatrix()
{
    glm::mat4 Model = glm::mat4(1.0f);

    Model = glm::translate(Model, Position);

    Model = glm::rotate(Model, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    Model = glm::rotate(Model, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    Model = glm::rotate(Model, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    Model = glm::scale(Model, Scale);

    return Model;
}


bool KH_Model::LoadOBJ(const std::string& path)
{
    Clear();

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path))
    {
        std::cerr << reader.Error() << std::endl;
        return false;
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    //TODO Optimize
    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            glm::vec3 Vertex = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            Vertices.push_back(Vertex);
            Indices.push_back(static_cast<uint32_t>(Indices.size()));
        }
    }

    UpdateBuffer();

    return true;
}

void KH_Model::UpdateBuffer()
{
    DeleteBuffer(); 

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(glm::vec3), Vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(uint32_t), Indices.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO); 
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void KH_Model::Render(KH_Shader& Shader)
{
    KH_Editor::Instance().BindCanvasFramebuffer();

    Shader.Use();
    Shader.SetMat4("model", GetModelMatrix());
    Shader.SetMat4("view", KH_Editor::Instance().Camera.GetViewMatrix());
    Shader.SetMat4("projection", KH_Editor::Instance().Camera.GetProjMatrix());

    glBindVertexArray(this->VAO);
    glDrawElements(DrawMode, static_cast<GLsizei>(Indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    KH_Editor::Instance().UnbindCanvasFramebuffer();
}

void KH_Model::Clear()
{
	Vertices.clear();
	Indices.clear();

    DeleteBuffer();
}

void KH_Model::DeleteBuffer()
{
    if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    if (VBO) { glDeleteBuffers(1, &VBO); VBO = 0; }
    if (EBO) { glDeleteBuffers(1, &EBO); EBO = 0; }
}

void KH_Model::SetVertices(std::vector<glm::vec3>& Vertices)
{
    this->Vertices = Vertices;
}

void KH_Model::SetIndices(std::vector<uint32_t>& Indices)
{
    this->Indices = Indices;
}

void KH_Model::SetDrawMode(GLenum DrawMode)
{
    this->DrawMode = DrawMode;
}

uint32_t KH_Model::GetIndicesSize() const
{
    return Indices.size();
}

uint32_t KH_Model::GetVerticesSize() const
{
    return Vertices.size();
}

GLenum KH_Model::GetDrawMode() const 
{
    return DrawMode;
}



KH_DefaultModels::KH_DefaultModels()
{
    InitCube();
    InitEmptyCube();
    InitPlane();
    InitBunny();
    InitMortonCurve();
}

void KH_DefaultModels::InitCube()
{
   std::vector<glm::vec3> Vertices = {
        // 前面 (Z+)
        {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f},
        // 后面 (Z-)
        {-1.0f, -1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f},
        // 左面 (X-)
        {-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f, -1.0f},
        // 右面 (X+)
        { 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f},
        // 上面 (Y+)
        {-1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f, -1.0f},
        // 下面 (Y-)
        {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f}
    };

    std::vector<uint32_t> Indices = {
        0,  1,  2,  2,  3,  0,   // 前
	    4,  5,  6,  6,  7,  4,   // 后
	    8,  9,  10, 10, 11, 8,   // 左
	    12, 13, 14, 14, 15, 12,  // 右
	    16, 17, 18, 18, 19, 16,  // 上
	    20, 21, 22, 22, 23, 20   // 下
    };

    Cube.SetVertices(Vertices);
    Cube.SetIndices(Indices);

    Cube.UpdateBuffer();
}

void KH_DefaultModels::InitEmptyCube()
{
    std::vector<glm::vec3> Vertices = {
        {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}, // 前 4 个 (0-3)
        {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1}  // 后 4 个 (4-7)
    };

    std::vector<unsigned int> Indices = {
        // 底面
        0, 1, 1, 5, 5, 4, 4, 0,
        // 顶面
        3, 2, 2, 6, 6, 7, 7, 3,
        // 四条立柱
        0, 3, 1, 2, 5, 6, 4, 7
    };

    EmptyCube.SetVertices(Vertices);
    EmptyCube.SetIndices(Indices);
    EmptyCube.SetDrawMode(GL_LINES);

    EmptyCube.UpdateBuffer();
}

void KH_DefaultModels::InitPlane()
{
    std::vector<glm::vec3> Vertices = {
            {-1.0f, -1.0f, 0.0f}, // 0: 左下
            { 1.0f, -1.0f, 0.0f}, // 1: 右下
            { 1.0f,  1.0f, 0.0f}, // 2: 右上
            {-1.0f,  1.0f, 0.0f}  // 3: 左上
    };

    std::vector<unsigned int> Indices = {
        0, 1, 2,  // 第一个三角形 (左下 -> 右下 -> 右上)
        0, 2, 3   // 第二个三角形 (左下 -> 右上 -> 左上)
    };

    Plane.SetVertices(Vertices);
    Plane.SetIndices(Indices);

    Plane.UpdateBuffer();

    Plane.SetDrawMode(GL_TRIANGLES);
}

void KH_DefaultModels::InitBunny()
{
    Bunny.LoadOBJ("Assert/Models/bunny.obj");
}

void KH_DefaultModels::InitMortonCurve()
{
    const int Dim = 8;
    const int VertCount = Dim * Dim * Dim;
    std::vector<glm::vec3> Vertices(VertCount);
    std::vector<unsigned int> Indices(VertCount);


    for (int x = 0; x < Dim; x++) {
        for (int y = 0; y < Dim; y++) {
            for (int z = 0; z < Dim; z++) {
                int linearIdx = z + Dim * y + Dim * Dim * x;
                Vertices[linearIdx] = glm::vec3(x, y, z) / (float)Dim;
                uint32_t MortonCode = KH_MortonCode::Morton3D_MagicBits(x, y, z);
                if (MortonCode < VertCount)
                {
                    Indices[MortonCode] = linearIdx;
                }
            }
        }
    }

    MortonCurve.SetVertices(Vertices);
    MortonCurve.SetIndices(Indices);
    MortonCurve.UpdateBuffer();
    MortonCurve.SetDrawMode(GL_LINE_STRIP);
}

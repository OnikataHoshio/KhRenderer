#include "KH_SceneXmlSerializer.h"
#include "KH_Scene.h"
namespace
{
    using namespace tinyxml2;

    std::string Vec3ToString(const glm::vec3& v)
    {
        std::ostringstream oss;
        oss << std::setprecision(9) << v.x << " " << v.y << " " << v.z;
        return oss.str();
    }

    bool StringToVec3(const char* text, glm::vec3& outValue)
    {
        if (text == nullptr)
            return false;

        std::istringstream iss(text);
        iss >> outValue.x >> outValue.y >> outValue.z;
        return !iss.fail();
    }

    void WriteTransform(XMLDocument& doc, XMLElement* parent, const KH_Object& obj)
    {
        XMLElement* transformElem = doc.NewElement("Transform");
        transformElem->SetAttribute("position", Vec3ToString(obj.GetPosition()).c_str());
        transformElem->SetAttribute("rotation", Vec3ToString(obj.GetRotation()).c_str());
        transformElem->SetAttribute("scale", Vec3ToString(obj.GetScale()).c_str());
        parent->InsertEndChild(transformElem);
    }

    void ReadTransform(const XMLElement* transformElem, KH_Object& obj)
    {
        if (transformElem == nullptr)
            return;

        glm::vec3 position(0.0f);
        glm::vec3 rotation(0.0f);
        glm::vec3 scale(1.0f);

        if (StringToVec3(transformElem->Attribute("position"), position))
            obj.SetPosition(position);

        if (StringToVec3(transformElem->Attribute("rotation"), rotation))
            obj.SetRotation(rotation);

        if (StringToVec3(transformElem->Attribute("scale"), scale))
            obj.SetScale(scale);
    }

    void WriteMaterial(XMLDocument& doc, XMLElement* materialsElem, const KH_BRDFMaterial& mat, int id)
    {
        XMLElement* materialElem = doc.NewElement("Material");

        materialElem->SetAttribute("id", id);
        materialElem->SetAttribute("baseColor", Vec3ToString(mat.BaseColor).c_str());
        materialElem->SetAttribute("emissive", Vec3ToString(mat.Emissive).c_str());

        materialElem->SetAttribute("subsurface", mat.Subsurface);
        materialElem->SetAttribute("metallic", mat.Metallic);
        materialElem->SetAttribute("specular", mat.Specular);
        materialElem->SetAttribute("specularTint", mat.SpecularTint);

        materialElem->SetAttribute("roughness", mat.Roughness);
        materialElem->SetAttribute("anisotropic", mat.Anisotropic);
        materialElem->SetAttribute("sheen", mat.Sheen);
        materialElem->SetAttribute("sheenTint", mat.SheenTint);

        materialElem->SetAttribute("clearcoat", mat.Clearcoat);
        materialElem->SetAttribute("clearcoatGloss", mat.ClearcoatGloss);
        materialElem->SetAttribute("ior", mat.IOR);
        materialElem->SetAttribute("transmission", mat.Transmission);

        materialsElem->InsertEndChild(materialElem);
    }

    KH_BRDFMaterial ReadMaterial(const XMLElement* materialElem)
    {
        KH_BRDFMaterial mat{};

        glm::vec3 baseColor(1.0f);
        glm::vec3 emissive(0.0f);

        StringToVec3(materialElem->Attribute("baseColor"), baseColor);
        StringToVec3(materialElem->Attribute("emissive"), emissive);

        mat.BaseColor = baseColor;
        mat.Emissive = emissive;

        materialElem->QueryFloatAttribute("subsurface", &mat.Subsurface);
        materialElem->QueryFloatAttribute("metallic", &mat.Metallic);
        materialElem->QueryFloatAttribute("specular", &mat.Specular);
        materialElem->QueryFloatAttribute("specularTint", &mat.SpecularTint);

        materialElem->QueryFloatAttribute("roughness", &mat.Roughness);
        materialElem->QueryFloatAttribute("anisotropic", &mat.Anisotropic);
        materialElem->QueryFloatAttribute("sheen", &mat.Sheen);
        materialElem->QueryFloatAttribute("sheenTint", &mat.SheenTint);

        materialElem->QueryFloatAttribute("clearcoat", &mat.Clearcoat);
        materialElem->QueryFloatAttribute("clearcoatGloss", &mat.ClearcoatGloss);
        materialElem->QueryFloatAttribute("ior", &mat.IOR);
        materialElem->QueryFloatAttribute("transmission", &mat.Transmission);

        return mat;
    }

    void WriteTriangle(XMLDocument& doc, XMLElement* objectsElem, const KH_Triangle& tri, int materialSlotID)
    {
        XMLElement* triangleElem = doc.NewElement("Triangle");
        triangleElem->SetAttribute("material", materialSlotID);

        WriteTransform(doc, triangleElem, tri);

        XMLElement* geometryElem = doc.NewElement("Geometry");
        geometryElem->SetAttribute("p1", Vec3ToString(tri.P1).c_str());
        geometryElem->SetAttribute("p2", Vec3ToString(tri.P2).c_str());
        geometryElem->SetAttribute("p3", Vec3ToString(tri.P3).c_str());
        geometryElem->SetAttribute("n1", Vec3ToString(tri.N1).c_str());
        geometryElem->SetAttribute("n2", Vec3ToString(tri.N2).c_str());
        geometryElem->SetAttribute("n3", Vec3ToString(tri.N3).c_str());

        triangleElem->InsertEndChild(geometryElem);
        objectsElem->InsertEndChild(triangleElem);
    }

    bool ReadTriangle(const XMLElement* triangleElem, KH_SceneBase& scene)
    {
        if (triangleElem == nullptr)
            return false;

        const XMLElement* geometryElem = triangleElem->FirstChildElement("Geometry");
        if (geometryElem == nullptr)
            return false;

        glm::vec3 p1(0.0f), p2(0.0f), p3(0.0f);
        glm::vec3 n1(0.0f, 0.0f, 1.0f), n2(0.0f, 0.0f, 1.0f), n3(0.0f, 0.0f, 1.0f);

        if (!StringToVec3(geometryElem->Attribute("p1"), p1)) return false;
        if (!StringToVec3(geometryElem->Attribute("p2"), p2)) return false;
        if (!StringToVec3(geometryElem->Attribute("p3"), p3)) return false;

        StringToVec3(geometryElem->Attribute("n1"), n1);
        StringToVec3(geometryElem->Attribute("n2"), n2);
        StringToVec3(geometryElem->Attribute("n3"), n3);

        int materialSlotID = triangleElem->IntAttribute("material", KH_MATERIAL_UNDEFINED_SLOT);

        KH_Triangle tri(p1, p2, p3, n1, n2, n3);
        KH_Triangle& triRef = scene.AddTriangle(materialSlotID, tri);

        ReadTransform(triangleElem->FirstChildElement("Transform"), triRef);
        return true;
    }

    void WriteModel(XMLDocument& doc, XMLElement* objectsElem, const KH_Model& model, int materialSlotID)
    {
        XMLElement* modelElem = doc.NewElement("Model");
        modelElem->SetAttribute("material", materialSlotID);
        modelElem->SetAttribute("path", model.GetSourcePath().c_str());

        WriteTransform(doc, modelElem, model);

        objectsElem->InsertEndChild(modelElem);
    }

    bool ReadModel(const XMLElement* modelElem, KH_SceneBase& scene)
    {
        if (modelElem == nullptr)
            return false;

        const char* path = modelElem->Attribute("path");
        if (path == nullptr || path[0] == '\0')
            return false;

        int materialSlotID = modelElem->IntAttribute("material", KH_MATERIAL_UNDEFINED_SLOT);

        KH_Model& modelRef = scene.AddModel(materialSlotID, path);
        ReadTransform(modelElem->FirstChildElement("Transform"), modelRef);

        return true;
    }

    void WriteMaterials(XMLDocument& doc, XMLElement* root, const KH_SceneBase& scene)
    {
        XMLElement* materialsElem = doc.NewElement("Materials");
        root->InsertEndChild(materialsElem);

        for (int i = 0; i < static_cast<int>(scene.Materials.size()); ++i)
        {
            WriteMaterial(doc, materialsElem, scene.Materials[i], i);
        }
    }

    void WriteObjects(XMLDocument& doc, XMLElement* root, const KH_SceneBase& scene)
    {
        XMLElement* objectsElem = doc.NewElement("Objects");
        root->InsertEndChild(objectsElem);

        for (const KH_SceneObject& sceneObject : scene.GetObjects())
        {
            const KH_Object* object = sceneObject.Object.get();
            if (object == nullptr)
                continue;

            if (const KH_Model* model = dynamic_cast<const KH_Model*>(object))
            {
                WriteModel(doc, objectsElem, *model, sceneObject.MaterialSlotID);
            }
            else if (const KH_Triangle* tri = dynamic_cast<const KH_Triangle*>(object))
            {
                WriteTriangle(doc, objectsElem, *tri, sceneObject.MaterialSlotID);
            }
        }
    }

    bool ReadMaterials(const XMLElement* root, KH_SceneBase& scene)
    {
        const XMLElement* materialsElem = root->FirstChildElement("Materials");
        if (materialsElem == nullptr)
            return true;

        for (const XMLElement* materialElem = materialsElem->FirstChildElement("Material");
            materialElem != nullptr;
            materialElem = materialElem->NextSiblingElement("Material"))
        {
            scene.Materials.push_back(ReadMaterial(materialElem));
        }

        return true;
    }

    bool ReadObjects(const XMLElement* root, KH_SceneBase& scene)
    {
        const XMLElement* objectsElem = root->FirstChildElement("Objects");
        if (objectsElem == nullptr)
            return true;

        for (const XMLElement* objectElem = objectsElem->FirstChildElement();
            objectElem != nullptr;
            objectElem = objectElem->NextSiblingElement())
        {
            const char* name = objectElem->Name();
            if (name == nullptr)
                continue;

            if (std::strcmp(name, "Model") == 0)
            {
                if (!ReadModel(objectElem, scene))
                    return false;
            }
            else if (std::strcmp(name, "Triangle") == 0)
            {
                if (!ReadTriangle(objectElem, scene))
                    return false;
            }
        }

        return true;
    }
}

namespace KH_SceneXmlSerializer
{
    bool SaveScene(const KH_SceneBase& scene, const std::string& filePath)
    {
        tinyxml2::XMLDocument doc;

        tinyxml2::XMLDeclaration* decl = doc.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")");
        doc.InsertFirstChild(decl);

        tinyxml2::XMLElement* root = doc.NewElement("Scene");
        root->SetAttribute("version", 1);
        doc.InsertEndChild(root);

        WriteMaterials(doc, root, scene);
        WriteObjects(doc, root, scene);

        return doc.SaveFile(filePath.c_str()) == tinyxml2::XML_SUCCESS;
    }

    bool LoadScene(KH_SceneBase& scene, const std::string& filePath)
    {
        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(filePath.c_str()) != tinyxml2::XML_SUCCESS)
            return false;

        const tinyxml2::XMLElement* root = doc.FirstChildElement("Scene");
        if (root == nullptr)
            return false;

        scene.Clear();
        scene.Materials.clear();

        if (!ReadMaterials(root, scene))
            return false;

        if (!ReadObjects(root, scene))
            return false;

        return true;
    }
}
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

    std::string Vec2ToString(const glm::vec2& v)
    {
        std::ostringstream oss;
        oss << std::setprecision(9) << v.x << " " << v.y;
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

    bool StringToVec2(const char* text, glm::vec2& outValue)
    {
        if (text == nullptr)
            return false;

        std::istringstream iss(text);
        iss >> outValue.x >> outValue.y;
        return !iss.fail();
    }

    std::string UIntsToString(const std::vector<unsigned int>& values)
    {
        std::ostringstream oss;
        for (size_t i = 0; i < values.size(); ++i)
        {
            if (i > 0)
                oss << ' ';
            oss << values[i];
        }
        return oss.str();
    }

    bool StringToUInts(const char* text, std::vector<unsigned int>& outValues)
    {
        outValues.clear();
        if (text == nullptr)
            return false;

        std::istringstream iss(text);
        unsigned int v = 0;
        while (iss >> v)
            outValues.push_back(v);

        return !outValues.empty();
    }

    std::string DrawModeToString(GLenum drawMode)
    {
        switch (drawMode)
        {
        case GL_LINES:     return "Lines";
        case GL_TRIANGLES: return "Triangles";
        default:           return "Triangles";
        }
    }

    GLenum StringToDrawMode(const char* text)
    {
        if (text == nullptr)
            return GL_TRIANGLES;

        if (std::strcmp(text, "Lines") == 0)
            return GL_LINES;

        return GL_TRIANGLES;
    }

    bool StringToBuiltinType(const char* text, KH_BuiltinModelType& outType)
    {
        if (text == nullptr)
            return false;

        auto result = magic_enum::enum_cast<KH_BuiltinModelType>(text);
        if (!result.has_value())
            return false;

        outType = result.value();
        return true;
    }

    bool StringToShaderFeatureType(const char* text, KH_ShaderFeatureType& outType)
    {
        if (text == nullptr)
            return false;

        auto result = magic_enum::enum_cast<KH_ShaderFeatureType>(text);
        if (!result.has_value())
            return false;

        outType = result.value();
        return true;
    }

    const char* ShaderFeatureTypeToString(KH_ShaderFeatureType type)
    {
        return magic_enum::enum_name(type).data();
    }

    bool EnsureShaderFeatureExists(KH_SceneBase& scene, KH_ShaderFeatureType type)
    {
        if (scene.GetShaderFeature(type) != nullptr)
            return true;

        switch (type)
        {
        case KH_ShaderFeatureType::DisneyBRDF:
            scene.EmplaceShaderFeature<KH_DisneyBRDF>(KH_ShaderFeatureType::DisneyBRDF);
            return true;

        case KH_ShaderFeatureType::BSSRDF:
            scene.EmplaceShaderFeature<KH_BSSRDF>(KH_ShaderFeatureType::BSSRDF);
            return true;

        default:
            return false;
        }
    }

    void WriteTransform(XMLDocument& doc, XMLElement* parent, const KH_Model& model)
    {
        XMLElement* transformElem = doc.NewElement("Transform");
        transformElem->SetAttribute("position", Vec3ToString(model.GetPosition()).c_str());
        transformElem->SetAttribute("rotation", Vec3ToString(model.GetRotation()).c_str());
        transformElem->SetAttribute("scale", Vec3ToString(model.GetScale()).c_str());
        parent->InsertEndChild(transformElem);
    }

    void ReadTransform(const XMLElement* transformElem, KH_Model& model)
    {
        if (transformElem == nullptr)
            return;

        glm::vec3 position(0.0f);
        glm::vec3 rotation(0.0f);
        glm::vec3 scale(1.0f);

        if (StringToVec3(transformElem->Attribute("position"), position))
            model.SetPosition(position);

        if (StringToVec3(transformElem->Attribute("rotation"), rotation))
            model.SetRotation(rotation);

        if (StringToVec3(transformElem->Attribute("scale"), scale))
            model.SetScale(scale);
    }

    void WriteDisneyMaterial(XMLDocument& doc, XMLElement* materialsElem, const KH_BRDFMaterial& mat, int id)
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

    KH_BRDFMaterial ReadDisneyMaterial(const XMLElement* materialElem)
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

    void WriteBSSRDFMaterial(XMLDocument& doc, XMLElement* materialsElem, const KH_BSSRDFMaterial& mat, int id)
    {
        XMLElement* materialElem = doc.NewElement("Material");

        materialElem->SetAttribute("id", id);
        materialElem->SetAttribute("emissive", Vec3ToString(mat.Emissive).c_str());
        materialElem->SetAttribute("baseColor", Vec3ToString(mat.BaseColor).c_str());
        materialElem->SetAttribute("radius", Vec3ToString(mat.Radius).c_str());
        materialElem->SetAttribute("eta", mat.Eta);
        materialElem->SetAttribute("scale", mat.Scale);

        materialsElem->InsertEndChild(materialElem);
    }

    KH_BSSRDFMaterial ReadBSSRDFMaterial(const XMLElement* materialElem)
    {
        KH_BSSRDFMaterial mat{};

        glm::vec3 baseColor(1.0f);
        glm::vec3 emissive(0.0f);
        glm::vec3 radius(1.0f, 0.2f, 0.1f);


        StringToVec3(materialElem->Attribute("emissive"), emissive);
        StringToVec3(materialElem->Attribute("baseColor"), baseColor);
        StringToVec3(materialElem->Attribute("radius"), radius);

        mat.Emissive = emissive;
        mat.BaseColor = baseColor;
        mat.Radius = radius;

        materialElem->QueryFloatAttribute("eta", &mat.Eta);
        materialElem->QueryFloatAttribute("scale", &mat.Scale);
        return mat;
    }

    void WriteSceneSettings(XMLDocument& doc, XMLElement* root, const KH_SceneBase& scene)
    {
        XMLElement* settingsElem = doc.NewElement("SceneSettings");
        settingsElem->SetAttribute(
            "activeShaderFeature",
            ShaderFeatureTypeToString(scene.GetActiveShaderFeatureType()));
        root->InsertEndChild(settingsElem);
    }

    bool ReadSceneSettings(const XMLElement* root, KH_ShaderFeatureType& outActiveType)
    {
        outActiveType = KH_ShaderFeatureType::DisneyBRDF;

        const XMLElement* settingsElem = root->FirstChildElement("SceneSettings");
        if (settingsElem == nullptr)
            return true;

        const char* activeTypeText = settingsElem->Attribute("activeShaderFeature");
        if (activeTypeText == nullptr)
            return true;

        return StringToShaderFeatureType(activeTypeText, outActiveType);
    }

    void WriteShaderFeature(XMLDocument& doc, XMLElement* featuresElem, const KH_SceneBase& scene, KH_ShaderFeatureType type)
    {
        const KH_ShaderFeatureBase* feature = scene.GetShaderFeature(type);
        if (feature == nullptr)
            return;

        XMLElement* featureElem = doc.NewElement("ShaderFeature");
        featureElem->SetAttribute("type", ShaderFeatureTypeToString(type));
        featureElem->SetAttribute("enabled", feature->IsEnabled());

        XMLElement* materialsElem = doc.NewElement("Materials");
        featureElem->InsertEndChild(materialsElem);

        if (const KH_DisneyBRDF* disney = dynamic_cast<const KH_DisneyBRDF*>(feature))
        {
            const auto& materials = disney->GetMaterials();
            for (int i = 0; i < static_cast<int>(materials.size()); ++i)
            {
                WriteDisneyMaterial(doc, materialsElem, materials[i], i);
            }
        }
        else if (const KH_BSSRDF* bssrdf = dynamic_cast<const KH_BSSRDF*>(feature))
        {
            const auto& materials = bssrdf->GetMaterials();
            for (int i = 0; i < static_cast<int>(materials.size()); ++i)
            {
                WriteBSSRDFMaterial(doc, materialsElem, materials[i], i);
            }
        }

        featuresElem->InsertEndChild(featureElem);
    }

    void WriteShaderFeatures(XMLDocument& doc, XMLElement* root, const KH_SceneBase& scene)
    {
        XMLElement* featuresElem = doc.NewElement("ShaderFeatures");
        root->InsertEndChild(featuresElem);

        for (size_t i = 0; i < KH_ShaderFeatureTypeCount; ++i)
        {
            const KH_ShaderFeatureType type = static_cast<KH_ShaderFeatureType>(i);
            WriteShaderFeature(doc, featuresElem, scene, type);
        }
    }

    bool ReadShaderFeature(const XMLElement* featureElem, KH_SceneBase& scene)
    {
        KH_ShaderFeatureType type = KH_ShaderFeatureType::DisneyBRDF;
        if (!StringToShaderFeatureType(featureElem->Attribute("type"), type))
            return false;

        if (!EnsureShaderFeatureExists(scene, type))
            return false;

        KH_ShaderFeatureBase* feature = scene.GetShaderFeature(type);
        if (feature == nullptr)
            return false;

        feature->SetEnabled(featureElem->BoolAttribute("enabled", true));
        feature->ClearMaterials();

        const XMLElement* materialsElem = featureElem->FirstChildElement("Materials");
        if (materialsElem == nullptr)
            return true;

        if (KH_DisneyBRDF* disney = dynamic_cast<KH_DisneyBRDF*>(feature))
        {
            for (const XMLElement* materialElem = materialsElem->FirstChildElement("Material");
                materialElem != nullptr;
                materialElem = materialElem->NextSiblingElement("Material"))
            {
                disney->AddMaterial(ReadDisneyMaterial(materialElem));
            }
            return true;
        }

        if (KH_BSSRDF* bssrdf = dynamic_cast<KH_BSSRDF*>(feature))
        {
            for (const XMLElement* materialElem = materialsElem->FirstChildElement("Material");
                materialElem != nullptr;
                materialElem = materialElem->NextSiblingElement("Material"))
            {
                bssrdf->AddMaterial(ReadBSSRDFMaterial(materialElem));
            }
            return true;
        }

        return false;
    }

    bool ReadShaderFeatures(const XMLElement* root, KH_SceneBase& scene)
    {
        const XMLElement* featuresElem = root->FirstChildElement("ShaderFeatures");
        if (featuresElem == nullptr)
            return true;

        for (const XMLElement* featureElem = featuresElem->FirstChildElement("ShaderFeature");
            featureElem != nullptr;
            featureElem = featureElem->NextSiblingElement("ShaderFeature"))
        {
            if (!ReadShaderFeature(featureElem, scene))
                return false;
        }

        return true;
    }

    void WriteMeshMaterialSlots(XMLDocument& doc, XMLElement* modelElem, const KH_Model& model)
    {
        XMLElement* slotsElem = doc.NewElement("MeshMaterialSlots");

        const auto& meshes = model.GetMeshes();
        for (int meshID = 0; meshID < static_cast<int>(meshes.size()); ++meshID)
        {
            XMLElement* meshSlotElem = doc.NewElement("MeshMaterialSlot");
            meshSlotElem->SetAttribute("meshID", meshID);

            for (size_t i = 0; i < KH_ShaderFeatureTypeCount; ++i)
            {
                const KH_ShaderFeatureType type = static_cast<KH_ShaderFeatureType>(i);

                XMLElement* slotElem = doc.NewElement("Slot");
                slotElem->SetAttribute("feature", ShaderFeatureTypeToString(type));
                slotElem->SetAttribute("material", meshes[meshID].GetMaterialSlotID(type));
                meshSlotElem->InsertEndChild(slotElem);
            }

            slotsElem->InsertEndChild(meshSlotElem);
        }

        modelElem->InsertEndChild(slotsElem);
    }

    bool ReadMeshMaterialSlots(const XMLElement* modelElem, KH_Model& model, KH_SceneBase& scene)
    {
        const XMLElement* slotsElem = modelElem->FirstChildElement("MeshMaterialSlots");
        if (slotsElem == nullptr)
            return true;

        for (const XMLElement* meshSlotElem = slotsElem->FirstChildElement("MeshMaterialSlot");
            meshSlotElem != nullptr;
            meshSlotElem = meshSlotElem->NextSiblingElement("MeshMaterialSlot"))
        {
            const int meshID = meshSlotElem->IntAttribute("meshID", -1);
            if (meshID < 0)
                continue;

            for (const XMLElement* slotElem = meshSlotElem->FirstChildElement("Slot");
                slotElem != nullptr;
                slotElem = slotElem->NextSiblingElement("Slot"))
            {
                KH_ShaderFeatureType type = KH_ShaderFeatureType::DisneyBRDF;
                if (!StringToShaderFeatureType(slotElem->Attribute("feature"), type))
                    return false;

                if (!EnsureShaderFeatureExists(scene, type))
                    return false;

                const int materialSlotID = slotElem->IntAttribute("material", KH_MATERIAL_UNDEFINED_SLOT);
                model.SetMeshMaterialSlotID(type, materialSlotID, meshID);
            }
        }

        return true;
    }

    void WriteInlineMeshes(XMLDocument& doc, XMLElement* modelElem, const KH_Model& model)
    {
        XMLElement* meshesElem = doc.NewElement("Meshes");

        for (const KH_Mesh& mesh : model.GetMeshes())
        {
            XMLElement* meshElem = doc.NewElement("Mesh");
            meshElem->SetAttribute("drawMode", DrawModeToString(mesh.GetDrawMode()).c_str());

            XMLElement* verticesElem = doc.NewElement("Vertices");
            for (const KH_Vertex& v : mesh.GetVertices())
            {
                XMLElement* vertexElem = doc.NewElement("Vertex");
                vertexElem->SetAttribute("position", Vec3ToString(v.Position).c_str());
                vertexElem->SetAttribute("normal", Vec3ToString(v.Normal).c_str());
                vertexElem->SetAttribute("tangent", Vec3ToString(v.Tangent).c_str());
                vertexElem->SetAttribute("bitangent", Vec3ToString(v.Bitangent).c_str());
                vertexElem->SetAttribute("uv", Vec2ToString(v.UV).c_str());
                verticesElem->InsertEndChild(vertexElem);
            }

            XMLElement* indicesElem = doc.NewElement("Indices");
            indicesElem->SetText(UIntsToString(mesh.GetIndices()).c_str());

            meshElem->InsertEndChild(verticesElem);
            meshElem->InsertEndChild(indicesElem);
            meshesElem->InsertEndChild(meshElem);
        }

        modelElem->InsertEndChild(meshesElem);
    }

    bool ReadInlineMeshes(const XMLElement* modelElem, KH_Model& model)
    {
        const XMLElement* meshesElem = modelElem->FirstChildElement("Meshes");
        if (meshesElem == nullptr)
            return false;

        bool hasAnyMesh = false;

        for (const XMLElement* meshElem = meshesElem->FirstChildElement("Mesh");
            meshElem != nullptr;
            meshElem = meshElem->NextSiblingElement("Mesh"))
        {
            std::vector<KH_Vertex> vertices;
            std::vector<unsigned int> indices;
            std::vector<KH_Texture> textures;

            const GLenum drawMode = StringToDrawMode(meshElem->Attribute("drawMode"));

            const XMLElement* verticesElem = meshElem->FirstChildElement("Vertices");
            if (verticesElem == nullptr)
                return false;

            for (const XMLElement* vertexElem = verticesElem->FirstChildElement("Vertex");
                vertexElem != nullptr;
                vertexElem = vertexElem->NextSiblingElement("Vertex"))
            {
                KH_Vertex v{};
                if (!StringToVec3(vertexElem->Attribute("position"), v.Position))
                    return false;

                StringToVec3(vertexElem->Attribute("normal"), v.Normal);
                StringToVec3(vertexElem->Attribute("tangent"), v.Tangent);
                StringToVec3(vertexElem->Attribute("bitangent"), v.Bitangent);
                StringToVec2(vertexElem->Attribute("uv"), v.UV);

                vertices.push_back(v);
            }

            const XMLElement* indicesElem = meshElem->FirstChildElement("Indices");
            if (indicesElem == nullptr || !StringToUInts(indicesElem->GetText(), indices))
                return false;

            KH_Mesh mesh(vertices, indices, textures, drawMode);
            model.AddMesh(std::move(mesh));
            hasAnyMesh = true;
        }

        if (hasAnyMesh)
            model.SetSourceAsInline();

        return hasAnyMesh;
    }

    bool BuildBuiltinModelFromXml(const XMLElement* modelElem, KH_Model& outModel)
    {
        KH_BuiltinModelType builtinType = KH_BuiltinModelType::None;
        if (!StringToBuiltinType(modelElem->Attribute("builtinType"), builtinType))
            return false;

        const float size = modelElem->FloatAttribute("size", 1.0f);
        outModel = KH_Model::CreateBuiltin(builtinType, size);
        return true;
    }

    void WriteModel(XMLDocument& doc, XMLElement* objectsElem, const KH_Model& model)
    {
        XMLElement* modelElem = doc.NewElement("Model");

        switch (model.GetSourceType())
        {
        case KH_ModelSourceType::Asset:
            modelElem->SetAttribute("sourceType", "Asset");
            modelElem->SetAttribute("path", model.GetSourcePath().c_str());
            break;

        case KH_ModelSourceType::Builtin:
        {
            const std::string builtinTypeName = std::string(magic_enum::enum_name(model.GetBuiltinType()));
            modelElem->SetAttribute("sourceType", "Builtin");
            modelElem->SetAttribute("builtinType", builtinTypeName.c_str());
            modelElem->SetAttribute("size", model.GetBuiltinSize());
            break;
        }

        case KH_ModelSourceType::Inline:
            modelElem->SetAttribute("sourceType", "Inline");
            WriteInlineMeshes(doc, modelElem, model);
            break;

        default:
            return;
        }

        WriteMeshMaterialSlots(doc, modelElem, model);
        WriteTransform(doc, modelElem, model);

        objectsElem->InsertEndChild(modelElem);
    }

    bool ReadModel(const XMLElement* modelElem, KH_SceneBase& scene)
    {
        if (modelElem == nullptr)
            return false;

        const char* sourceType = modelElem->Attribute("sourceType");
        if (sourceType == nullptr)
            return false;

        KH_Model* createdModel = nullptr;

        if (std::strcmp(sourceType, "Asset") == 0)
        {
            const char* path = modelElem->Attribute("path");
            if (path == nullptr || path[0] == '\0')
                return false;

            KH_Model& modelRef = scene.AddModel(KH_MATERIAL_UNDEFINED_SLOT, path);
            createdModel = &modelRef;
        }
        else if (std::strcmp(sourceType, "Builtin") == 0)
        {
            KH_Model model;
            if (!BuildBuiltinModelFromXml(modelElem, model))
                return false;

            KH_Model& modelRef = scene.AddModel(KH_MATERIAL_UNDEFINED_SLOT, std::move(model));
            createdModel = &modelRef;
        }
        else if (std::strcmp(sourceType, "Inline") == 0)
        {
            KH_Model model;
            if (!ReadInlineMeshes(modelElem, model))
                return false;

            KH_Model& modelRef = scene.AddModel(KH_MATERIAL_UNDEFINED_SLOT, std::move(model));
            createdModel = &modelRef;
        }
        else
        {
            return false;
        }

        ReadTransform(modelElem->FirstChildElement("Transform"), *createdModel);

        if (!ReadMeshMaterialSlots(modelElem, *createdModel, scene))
            return false;

        return true;
    }

    void WriteObjects(XMLDocument& doc, XMLElement* root, const KH_SceneBase& scene)
    {
        XMLElement* objectsElem = doc.NewElement("Objects");
        root->InsertEndChild(objectsElem);

        for (const auto& sceneObject : scene.GetObjects())
        {
            const KH_Model* model = dynamic_cast<const KH_Model*>(sceneObject.get());
            if (model == nullptr)
                continue;

            WriteModel(doc, objectsElem, *model);
        }
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
        }

        return true;
    }
}

namespace KH_SceneXmlSerializer
{
    bool SaveScene(const KH_SceneBase& scene, const std::string& filePath)
    {
        tinyxml2::XMLDocument doc;

        tinyxml2::XMLDeclaration* decl =
            doc.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")");
        doc.InsertFirstChild(decl);

        tinyxml2::XMLElement* root = doc.NewElement("Scene");
        root->SetAttribute("version", 4);
        doc.InsertEndChild(root);

        WriteSceneSettings(doc, root, scene);
        WriteShaderFeatures(doc, root, scene);
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

        KH_ShaderFeatureType activeType = KH_ShaderFeatureType::DisneyBRDF;
        if (!ReadSceneSettings(root, activeType))
            return false;

        scene.Clear();

        if (!ReadShaderFeatures(root, scene))
            return false;

        if (!ReadObjects(root, scene))
            return false;

        if (!scene.SetActiveShaderFeature(activeType))
            return false;

        return true;
    }
}
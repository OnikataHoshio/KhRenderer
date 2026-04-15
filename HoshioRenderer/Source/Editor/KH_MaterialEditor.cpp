#include "KH_MaterialEditor.h"
#include "KH_Editor.h"

namespace
{
    const char* ShaderFeatureDisplayName(KH_ShaderFeatureType type)
    {
        switch (type)
        {
        case KH_ShaderFeatureType::DisneyBRDF: return "Disney BRDF";
        case KH_ShaderFeatureType::BSSRDF:     return "BSSRDF";
        default:                               return "Unknown";
        }
    }
}

void KH_MaterialEditor::Render()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::Begin("Feature Materials");

    SyncSelectedMaterialFromEditorSelection();

    KH_Editor& Editor = KH_Editor::Instance();
    KH_GpuLBVHScene& Scene = Editor.Scene;

    KH_ShaderFeatureBase* ActiveFeature = Scene.GetActiveShaderFeature();
    const KH_ShaderFeatureType ActiveType = Scene.GetActiveShaderFeatureType();

    ImGui::TextDisabled("Active Shader Feature: %s", ShaderFeatureDisplayName(ActiveType));
    ImGui::Separator();

    if (ActiveFeature == nullptr)
    {
        ImGui::TextDisabled("No active shader feature");
    }
    else if (auto* Disney = Scene.GetShaderFeatureAs<KH_DisneyBRDF>(ActiveType))
    {
        RenderDisneyBRDFEditor(*Disney, Editor);
    }
    else if (auto* BSSRDF = Scene.GetShaderFeatureAs<KH_BSSRDF>(ActiveType))
    {
        RenderBSSRDFEditor(*BSSRDF, Editor);
    }
    else
    {
        ImGui::TextDisabled("No material editor available for this shader feature");
    }

    bIsFocused = ImGui::IsWindowFocused();
    bIsHovered = ImGui::IsWindowHovered();

    ImGui::End();
    ImGui::PopStyleVar();
}

void KH_MaterialEditor::RenderDisneyBRDFEditor(KH_DisneyBRDF& Feature, KH_Editor& Editor)
{
    KH_GpuLBVHScene& Scene = Editor.Scene;
    auto& Materials = Feature.GetMaterials();

    if (ImGui::Button("New Material"))
    {
        KH_BRDFMaterial mat{};
        mat.BaseColor = glm::vec3(0.8f);
        mat.Emissive = glm::vec3(0.0f);
        mat.Subsurface = 0.0f;
        mat.Metallic = 0.0f;
        mat.Specular = 0.5f;
        mat.SpecularTint = 0.0f;
        mat.Roughness = 0.5f;
        mat.Anisotropic = 0.0f;
        mat.Sheen = 0.0f;
        mat.SheenTint = 0.0f;
        mat.Clearcoat = 0.0f;
        mat.ClearcoatGloss = 0.0f;
        mat.IOR = 1.5f;
        mat.Transmission = 0.0f;

        SelectedMaterialID = Feature.AddMaterial(mat);
        Scene.UpdateMaterialSSBO();
        Editor.RequestFrameReset();
    }

    ImGui::SameLine();

    const bool canDelete =
        SelectedMaterialID >= 0 &&
        SelectedMaterialID < static_cast<int>(Materials.size());

    ImGui::BeginDisabled(!canDelete);
    if (ImGui::Button("Delete Material"))
    {
        if (Scene.DeleteMaterial(Scene.GetActiveShaderFeatureType(), SelectedMaterialID))
        {
            const int count = Feature.GetMaterialCount();
            SelectedMaterialID = (count <= 0) ? -1 : std::clamp(SelectedMaterialID, 0, count - 1);

            Scene.UpdatePrimitiveSSBO();
            Editor.RequestFrameReset();
        }
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    if (Materials.empty())
    {
        ImGui::TextDisabled("No materials");
        return;
    }

    SelectedMaterialID = std::clamp(
        SelectedMaterialID < 0 ? 0 : SelectedMaterialID,
        0,
        static_cast<int>(Materials.size()) - 1
    );

    std::vector<std::string> labels;
    std::vector<const char*> items;
    labels.reserve(Materials.size());
    items.reserve(Materials.size());

    for (int i = 0; i < static_cast<int>(Materials.size()); ++i)
    {
        labels.push_back("Material " + std::to_string(i));
    }

    for (auto& s : labels)
    {
        items.push_back(s.c_str());
    }

    ImGui::Text("Selected Material");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::Combo("##MaterialSelect", &SelectedMaterialID, items.data(), static_cast<int>(items.size()));

    ImGui::Separator();

    KH_BRDFMaterial& Mat = Materials[SelectedMaterialID];
    bool materialChanged = false;

    materialChanged |= ImGui::ColorEdit3(
        "BaseColor",
        &Mat.BaseColor.x,
        ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR
    );

    materialChanged |= ImGui::ColorEdit3(
        "Emissive",
        &Mat.Emissive.x,
        ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR
    );

    materialChanged |= ImGui::DragFloat("Subsurface", &Mat.Subsurface, 0.01f, 0.0f, 1.0f);
    materialChanged |= ImGui::DragFloat("Metallic", &Mat.Metallic, 0.01f, 0.0f, 1.0f);
    materialChanged |= ImGui::DragFloat("Specular", &Mat.Specular, 0.01f, 0.0f, 1.0f);
    materialChanged |= ImGui::DragFloat("SpecularTint", &Mat.SpecularTint, 0.01f, 0.0f, 1.0f);

    materialChanged |= ImGui::DragFloat("Roughness", &Mat.Roughness, 0.01f, 0.0f, 1.0f);
    materialChanged |= ImGui::DragFloat("Anisotropic", &Mat.Anisotropic, 0.01f, 0.0f, 1.0f);
    materialChanged |= ImGui::DragFloat("Sheen", &Mat.Sheen, 0.01f, 0.0f, 1.0f);
    materialChanged |= ImGui::DragFloat("SheenTint", &Mat.SheenTint, 0.01f, 0.0f, 1.0f);

    materialChanged |= ImGui::DragFloat("Clearcoat", &Mat.Clearcoat, 0.01f, 0.0f, 1.0f);
    materialChanged |= ImGui::DragFloat("ClearcoatGloss", &Mat.ClearcoatGloss, 0.01f, 0.0f, 1.0f);
    materialChanged |= ImGui::DragFloat("IOR", &Mat.IOR, 0.01f, 1.0f, 3.0f);
    materialChanged |= ImGui::DragFloat("Transmission", &Mat.Transmission, 0.01f, 0.0f, 1.0f);

    if (materialChanged)
    {
        Scene.UpdateMaterialSSBO();
        Editor.RequestFrameReset();
    }
}

void KH_MaterialEditor::RenderBSSRDFEditor(KH_BSSRDF& Feature, KH_Editor& Editor)
{
    KH_GpuLBVHScene& Scene = Editor.Scene;
    auto& Materials = Feature.GetMaterials();

    if (ImGui::Button("New Material"))
    {
        KH_BSSRDFMaterial mat{};
        mat.Emissive = glm::vec3(0.0f);
        mat.BaseColor = glm::vec3(0.8f);
        mat.Radius = glm::vec3(1.0f, 0.2f, 0.1f);
        mat.Eta = 1.3f;
        mat.Scale = 0.05f;
        
        SelectedMaterialID = Feature.AddMaterial(mat);
        Scene.UpdateMaterialSSBO();
        Editor.RequestFrameReset();
    }

    ImGui::SameLine();

    const bool canDelete =
        SelectedMaterialID >= 0 &&
        SelectedMaterialID < static_cast<int>(Materials.size());

    ImGui::BeginDisabled(!canDelete);
    if (ImGui::Button("Delete Material"))
    {
        if (Scene.DeleteMaterial(Scene.GetActiveShaderFeatureType(), SelectedMaterialID))
        {
            const int count = Feature.GetMaterialCount();
            SelectedMaterialID = (count <= 0) ? -1 : std::clamp(SelectedMaterialID, 0, count - 1);

            Scene.UpdatePrimitiveSSBO();
            Editor.RequestFrameReset();
        }
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    if (Materials.empty())
    {
        ImGui::TextDisabled("No materials");
        return;
    }

    SelectedMaterialID = std::clamp(
        SelectedMaterialID < 0 ? 0 : SelectedMaterialID,
        0,
        static_cast<int>(Materials.size()) - 1
    );

    std::vector<std::string> labels;
    std::vector<const char*> items;
    labels.reserve(Materials.size());
    items.reserve(Materials.size());

    for (int i = 0; i < static_cast<int>(Materials.size()); ++i)
    {
        labels.push_back("Material " + std::to_string(i));
    }

    for (auto& s : labels)
    {
        items.push_back(s.c_str());
    }

    ImGui::Text("Selected Material");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::Combo("##MaterialSelect", &SelectedMaterialID, items.data(), static_cast<int>(items.size()));

    ImGui::Separator();

    KH_BSSRDFMaterial& Mat = Materials[SelectedMaterialID];
    bool materialChanged = false;


    materialChanged |= ImGui::ColorEdit3(
        "Emissive",
        &Mat.Emissive.x,
        ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR
    );

    materialChanged |= ImGui::ColorEdit3(
        "BaseColor",
        &Mat.BaseColor.x,
        ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR
    );

    materialChanged |= ImGui::DragFloat("Scale", &Mat.Scale, 0.001f, 0.0f, 1000.0f);

    materialChanged |= ImGui::DragFloat3("Radius", &Mat.Radius.x, 0.001f, 0.0f, 100.0f);

    materialChanged |= ImGui::DragFloat("Eta", &Mat.Eta, 0.001f, 1.0f, 3.0f);


    if (materialChanged)
    {
        Scene.UpdateMaterialSSBO();
        Editor.RequestFrameReset();
    }
}

void KH_MaterialEditor::SyncSelectedMaterialFromEditorSelection()
{
    KH_Editor& Editor = KH_Editor::Instance();
    KH_GpuLBVHScene& Scene = Editor.Scene;

    const int objectID = Editor.GetSelectedObjectID();
    const int meshID = Editor.GetSelectedObjectMeshID();
    const KH_ShaderFeatureType activeType = Scene.GetActiveShaderFeatureType();
    const int activeTypeInt = static_cast<int>(activeType);

    if (objectID == LastSyncedObjectID &&
        meshID == LastSyncedMeshID &&
        activeTypeInt == LastSyncedFeatureType)
    {
        return;
    }

    LastSyncedObjectID = objectID;
    LastSyncedMeshID = meshID;
    LastSyncedFeatureType = activeTypeInt;

    KH_ShaderFeatureBase* ActiveFeature = Scene.GetActiveShaderFeature();
    const int MaterialCount = ActiveFeature ? ActiveFeature->GetMaterialCount() : 0;
    if (MaterialCount <= 0)
    {
        SelectedMaterialID = -1;
        return;
    }

    if (SelectedMaterialID < 0)
        SelectedMaterialID = 0;

    SelectedMaterialID = std::clamp(SelectedMaterialID, 0, MaterialCount - 1);

    auto& objects = Scene.GetObjects();
    if (objectID < 0 || objectID >= static_cast<int>(objects.size()))
        return;

    KH_Object* object = objects[objectID].get();
    KH_Model* model = dynamic_cast<KH_Model*>(object);
    if (model == nullptr)
        return;

    const auto& meshes = model->GetMeshes();
    if (meshID < 0 || meshID >= static_cast<int>(meshes.size()))
        return;

    SelectedMaterialID = std::clamp(
        meshes[meshID].GetMaterialSlotID(activeType),
        0,
        MaterialCount - 1
    );
}

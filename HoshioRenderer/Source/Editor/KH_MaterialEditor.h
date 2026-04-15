#pragma once

#include "KH_Panel.h"

class KH_Editor;
class KH_DisneyBRDF;
class KH_BSSRDF;

class KH_MaterialEditor : public KH_Panel
{
public:
    KH_MaterialEditor() = default;
    ~KH_MaterialEditor() override = default;

    void Render() override;

private:
    int SelectedMaterialID = -1;

    int LastSyncedObjectID = -2;
    int LastSyncedMeshID = -2;
    int LastSyncedFeatureType = -1;

    void SyncSelectedMaterialFromEditorSelection();

    void RenderDisneyBRDFEditor(KH_DisneyBRDF& Feature, KH_Editor& Editor);
    void RenderBSSRDFEditor(KH_BSSRDF& Feature, KH_Editor& Editor);
};


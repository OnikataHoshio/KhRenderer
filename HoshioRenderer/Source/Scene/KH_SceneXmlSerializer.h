#pragma once
#include <KH_Common.h>

class KH_SceneBase;

namespace KH_SceneXmlSerializer
{
    bool SaveScene(const KH_SceneBase& scene, const std::string& filePath);
    bool LoadScene(KH_SceneBase& scene, const std::string& filePath);
}
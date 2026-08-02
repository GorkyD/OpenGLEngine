#include "Editor/AssetLibrary.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

void AssetLibrary::Refresh()
{
    modelPaths.clear();

    std::error_code ec;
    if (!std::filesystem::exists(modelsRoot, ec))
        return;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(modelsRoot, ec))
    {
        if (!entry.is_regular_file())
            continue;

        std::string extension = entry.path().extension().string();
        for (auto& c : extension)
            c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));

        if (extension != ".obj" && extension != ".fbx" && extension != ".glb" && extension != ".gltf")
            continue;

        std::string path = entry.path().generic_string();
        const auto pos = path.find(modelsRoot);
        if (pos != std::string::npos)
            path = path.substr(pos);

        modelPaths.push_back(path);
    }

    std::sort(modelPaths.begin(), modelPaths.end());
}

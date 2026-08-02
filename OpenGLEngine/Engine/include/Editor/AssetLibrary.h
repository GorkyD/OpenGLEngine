#pragma once

#include <string>
#include <vector>

class AssetLibrary
{
public:
    void Refresh();

    const std::vector<std::string>& GetModelPaths() const
    {
        return modelPaths;
    }

    bool IsEmpty() const
    {
        return modelPaths.empty();
    }

private:
    static constexpr const char* modelsRoot = "Assets/Models";

    std::vector<std::string> modelPaths;
};

#pragma once
#include <array>
#include <string>
#include "Extension/Extension.h"

class Texture
{
public:
    static TexturePtr LoadFromFile(const std::string& path);

    // Order: +X (right), -X (left), +Y (top), -Y (bottom), +Z (front), -Z (back)
    static TexturePtr LoadCubemap(const std::array<std::string, 6>& facePaths);

    static TexturePtr CreateFromPixels(const unsigned char* pixels, int width, int height, int channels);

    ~Texture();

    unsigned int GetId() const
    {
        return id;
    }

    unsigned int GetTarget() const
    {
        return target;
    }

private:
    Texture() = default;
    unsigned int id = 0;
    unsigned int target = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
};

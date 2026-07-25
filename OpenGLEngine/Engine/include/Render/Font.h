#pragma once
#include "Extension/Extension.h"
#include <string>
#include <vector>

struct FontGlyphVertex
{
    float x, y;
    float u, v;
};

class Font
{
public:
    static FontPtr LoadFromFile(const std::string& path, float pixelHeight = 48.0f);

    std::vector<FontGlyphVertex> BuildGeometry(const std::string& text, float x, float y, float scale) const;

    float MeasureWidth(const std::string& text, float scale) const;
    float GetLineHeight(float scale) const;

    TexturePtr GetAtlasTexture() const
    {
        return atlasTexture;
    }

private:
    Font() = default;

    static constexpr int FirstChar = 32;
    static constexpr int NumChars = 96;

    TexturePtr atlasTexture;
    std::vector<unsigned char> bakedChars;
    int atlasWidth = 0;
    int atlasHeight = 0;
    float pixelHeight = 48.0f;
    float ascent = 0.0f;
};

#pragma once

#include "Extension/Extension.h"
#include "Math/Vector4.h"
#include <string>
#include <vector>

struct FontGlyphVertex
{
    float x, y;
    float u, v;
    float r, g, b, a;
};

class Font
{
public:
    static FontPtr LoadFromFile(const std::string& path, float pixelHeight = 48.0f);

    std::vector<FontGlyphVertex> BuildGeometry(const std::string& text, float x, float y, float scale, const Vector4& color) const;

    float MeasureWidth(const std::string& text, float scale) const;
    float GetLineHeight(float scale) const;

    void DrawToPixels(const std::string& text, unsigned char* rgba, int width, int height, float x, float y, float scale, const Vector4& color) const;

    float GetPixelHeight() const
    {
        return pixelHeight;
    }

    TexturePtr GetAtlasTexture() const
    {
        return atlasTexture;
    }

private:
    Font() = default;

    struct GlyphRange
    {
        int first = 0;
        int count = 0;
        int offset = 0;
    };

    int GlyphIndex(unsigned int codepoint) const;

    TexturePtr atlasTexture;

    std::vector<unsigned char> packedChars;
    std::vector<unsigned char> atlasCoverage;
    std::vector<GlyphRange> ranges;

    float pixelHeight = 48.0f;
    float ascent = 0.0f;

    int atlasWidth = 0;
    int atlasHeight = 0;
};

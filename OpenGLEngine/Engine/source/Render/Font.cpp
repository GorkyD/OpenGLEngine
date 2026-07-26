#include "Render/Font.h"
#include "Render/Texture.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

#include <algorithm>
#include <fstream>

FontPtr Font::LoadFromFile(const std::string& path, float pixelHeight)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        OGL_WARNING("Font | Failed to open: " << path)
        return nullptr;
    }

    const std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> fontData(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(fontData.data()), fileSize))
    {
        OGL_WARNING("Font | Failed to read: " << path)
        return nullptr;
    }

    const int atlasWidth = 512;
    const int atlasHeight = 512;
    std::vector<unsigned char> atlasPixels(atlasWidth * atlasHeight);
    std::vector<unsigned char> bakedChars(NumChars * sizeof(stbtt_bakedchar));

    const int result = stbtt_BakeFontBitmap(fontData.data(), 0, pixelHeight, atlasPixels.data(), atlasWidth, atlasHeight, FirstChar, NumChars, reinterpret_cast<stbtt_bakedchar*>(bakedChars.data()));
    if (result <= 0)
    {
        OGL_WARNING("Font | Atlas too small or bake failed for: " << path)
        return nullptr;
    }

    stbtt_fontinfo fontInfo;
    stbtt_InitFont(&fontInfo, fontData.data(), 0);
    int ascent = 0;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, nullptr, nullptr);
    const float scaleForPixelHeight = stbtt_ScaleForPixelHeight(&fontInfo, pixelHeight);

    auto font = FontPtr(new Font());
    font->atlasTexture = Texture::CreateFromPixels(atlasPixels.data(), atlasWidth, atlasHeight, 1);
    font->bakedChars = std::move(bakedChars);
    font->atlasWidth = atlasWidth;
    font->atlasHeight = atlasHeight;
    font->pixelHeight = pixelHeight;
    font->ascent = static_cast<float>(ascent) * scaleForPixelHeight;

    OGL_INFO("Font | Loaded: " << path << " (" << pixelHeight << "px)")
    return font;
}

std::vector<FontGlyphVertex> Font::BuildGeometry(const std::string& text, float x, float y, float scale, const Vector4& color) const
{
    std::vector<FontGlyphVertex> vertices;
    if (!atlasTexture)
        return vertices;

    vertices.reserve(text.size() * 6);

    const auto* chars = reinterpret_cast<const stbtt_bakedchar*>(bakedChars.data());

    float cursorX = x;
    float cursorY = y + ascent * scale;

    for (const unsigned char c : text)
    {
        if (c == '\n')
        {
            cursorX = x;
            cursorY += GetLineHeight(scale);
            continue;
        }

        if (c < FirstChar || c >= FirstChar + NumChars)
            continue;

        stbtt_aligned_quad quad;
        float unscaledX = 0.0f;
        float unscaledY = 0.0f;
        stbtt_GetBakedQuad(chars, atlasWidth, atlasHeight, c - FirstChar, &unscaledX, &unscaledY, &quad, 1);

        const float x0 = cursorX + quad.x0 * scale;
        const float x1 = cursorX + quad.x1 * scale;
        const float y0 = cursorY + quad.y0 * scale;
        const float y1 = cursorY + quad.y1 * scale;

        vertices.push_back({x0, y0, quad.s0, quad.t0, color.x, color.y, color.z, color.w});
        vertices.push_back({x1, y0, quad.s1, quad.t0, color.x, color.y, color.z, color.w});
        vertices.push_back({x1, y1, quad.s1, quad.t1, color.x, color.y, color.z, color.w});

        vertices.push_back({x0, y0, quad.s0, quad.t0, color.x, color.y, color.z, color.w});
        vertices.push_back({x1, y1, quad.s1, quad.t1, color.x, color.y, color.z, color.w});
        vertices.push_back({x0, y1, quad.s0, quad.t1, color.x, color.y, color.z, color.w});

        cursorX += unscaledX * scale;
    }

    return vertices;
}

float Font::MeasureWidth(const std::string& text, float scale) const
{
    if (!atlasTexture)
        return 0.0f;

    const auto* chars = reinterpret_cast<const stbtt_bakedchar*>(bakedChars.data());

    float width = 0.0f;
    float lineWidth = 0.0f;
    for (const unsigned char c : text)
    {
        if (c == '\n')
        {
            width = std::max(width, lineWidth);
            lineWidth = 0.0f;
            continue;
        }

        if (c < FirstChar || c >= FirstChar + NumChars)
            continue;

        lineWidth += chars[c - FirstChar].xadvance * scale;
    }

    return std::max(width, lineWidth);
}

float Font::GetLineHeight(float scale) const
{
    return pixelHeight * scale * 1.2f;
}

#include "Render/Font.h"
#include "Render/Texture.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

#include <algorithm>
#include <cmath>
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

    static const int rangeStarts[] = {32, 0xA0, 0x400, 0x2010};
    static const int rangeCounts[] = {95, 96, 96, 0x30};
    constexpr int rangeCount = 4;

    int totalGlyphs = 0;
    for (const int count : rangeCounts)
        totalGlyphs += count;

    const float cell = pixelHeight * 0.75f + 2.0f;
    const float needed = static_cast<float>(totalGlyphs) * cell * cell * 1.4f;
    int atlasSize = 256;
    while (static_cast<float>(atlasSize) * static_cast<float>(atlasSize) < needed && atlasSize < 4096)
        atlasSize *= 2;
    const int atlasWidth = atlasSize;
    const int atlasHeight = atlasSize;

    std::vector<unsigned char> atlasPixels(static_cast<size_t>(atlasWidth) * atlasHeight);
    std::vector<unsigned char> packedChars(static_cast<size_t>(totalGlyphs) * sizeof(stbtt_packedchar));
    auto* packed = reinterpret_cast<stbtt_packedchar*>(packedChars.data());

    std::vector<stbtt_pack_range> packRanges(rangeCount);
    std::vector<GlyphRange> glyphRanges(rangeCount);
    int offset = 0;
    for (int i = 0; i < rangeCount; i++)
    {
        packRanges[i] = {};
        packRanges[i].font_size = pixelHeight;
        packRanges[i].first_unicode_codepoint_in_range = rangeStarts[i];
        packRanges[i].num_chars = rangeCounts[i];
        packRanges[i].chardata_for_range = packed + offset;
        glyphRanges[i] = {rangeStarts[i], rangeCounts[i], offset};
        offset += rangeCounts[i];
    }

    stbtt_pack_context context;
    if (!stbtt_PackBegin(&context, atlasPixels.data(), atlasWidth, atlasHeight, 0, 1, nullptr))
    {
        OGL_WARNING("Font | Pack begin failed for: " << path)
        return nullptr;
    }
    stbtt_PackSetOversampling(&context, 1, 1);
    const int packed_ok = stbtt_PackFontRanges(&context, fontData.data(), 0, packRanges.data(), rangeCount);
    stbtt_PackEnd(&context);
    if (!packed_ok)
        OGL_WARNING("Font | Some glyphs did not fit or are missing in: " << path)

    stbtt_fontinfo fontInfo;
    stbtt_InitFont(&fontInfo, fontData.data(), 0);
    int ascent = 0;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, nullptr, nullptr);
    const float scaleForPixelHeight = stbtt_ScaleForPixelHeight(&fontInfo, pixelHeight);

    auto font = FontPtr(new Font());
    font->atlasTexture = Texture::CreateFromPixels(atlasPixels.data(), atlasWidth, atlasHeight, 1);
    font->atlasCoverage = atlasPixels;
    font->packedChars = std::move(packedChars);
    font->ranges = std::move(glyphRanges);
    font->atlasWidth = atlasWidth;
    font->atlasHeight = atlasHeight;
    font->pixelHeight = pixelHeight;
    font->ascent = static_cast<float>(ascent) * scaleForPixelHeight;

    OGL_INFO("Font | Loaded: " << path << " (" << pixelHeight << "px)")
    return font;
}

namespace
{
unsigned int NextCodepoint(const std::string& text, size_t& index)
{
    const unsigned char lead = static_cast<unsigned char>(text[index++]);
    if (lead < 0x80)
        return lead;
    int extra = 0;
    unsigned int codepoint = 0;
    if ((lead & 0xE0) == 0xC0)
    {
        extra = 1;
        codepoint = lead & 0x1F;
    }
    else if ((lead & 0xF0) == 0xE0)
    {
        extra = 2;
        codepoint = lead & 0x0F;
    }
    else if ((lead & 0xF8) == 0xF0)
    {
        extra = 3;
        codepoint = lead & 0x07;
    }
    else
    {
        return 0xFFFD;
    }
    for (int i = 0; i < extra; i++)
    {
        if (index >= text.size())
            return 0xFFFD;
        const unsigned char next = static_cast<unsigned char>(text[index]);
        if ((next & 0xC0) != 0x80)
            return 0xFFFD;
        codepoint = (codepoint << 6) | (next & 0x3F);
        index++;
    }
    return codepoint;
}
}

int Font::GlyphIndex(unsigned int codepoint) const
{
    for (const GlyphRange& range : ranges)
    {
        if (codepoint >= static_cast<unsigned int>(range.first) && codepoint < static_cast<unsigned int>(range.first + range.count))
            return range.offset + static_cast<int>(codepoint - range.first);
    }
    return -1;
}

std::vector<FontGlyphVertex> Font::BuildGeometry(const std::string& text, float x, float y, float scale, const Vector4& color) const
{
    std::vector<FontGlyphVertex> vertices;
    if (!atlasTexture)
        return vertices;

    vertices.reserve(text.size() * 6);

    const auto* chars = reinterpret_cast<const stbtt_packedchar*>(packedChars.data());

    float cursorX = x;
    float cursorY = y + ascent * scale;

    size_t index = 0;
    while (index < text.size())
    {
        const unsigned int c = NextCodepoint(text, index);
        if (c == '\n')
        {
            cursorX = x;
            cursorY += GetLineHeight(scale);
            continue;
        }

        const int glyph = GlyphIndex(c);
        if (glyph < 0)
            continue;

        stbtt_aligned_quad quad;
        float unscaledX = 0.0f;
        float unscaledY = 0.0f;
        stbtt_GetPackedQuad(chars, atlasWidth, atlasHeight, glyph, &unscaledX, &unscaledY, &quad, 1);

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

    const auto* chars = reinterpret_cast<const stbtt_packedchar*>(packedChars.data());

    float width = 0.0f;
    float lineWidth = 0.0f;
    size_t index = 0;
    while (index < text.size())
    {
        const unsigned int c = NextCodepoint(text, index);
        if (c == '\n')
        {
            width = std::max(width, lineWidth);
            lineWidth = 0.0f;
            continue;
        }

        const int glyph = GlyphIndex(c);
        if (glyph < 0)
            continue;

        lineWidth += chars[glyph].xadvance * scale;
    }

    return std::max(width, lineWidth);
}

float Font::GetLineHeight(float scale) const
{
    return pixelHeight * scale * 1.2f;
}

void Font::DrawToPixels(const std::string& text, unsigned char* rgba, int width, int height, float x, float y, float scale, const Vector4& color) const
{
    if (atlasCoverage.empty() || !rgba)
        return;

    const auto* chars = reinterpret_cast<const stbtt_packedchar*>(packedChars.data());
    float cursorX = x;
    float cursorY = y + ascent * scale;

    size_t index = 0;
    while (index < text.size())
    {
        const unsigned int c = NextCodepoint(text, index);
        if (c == '\n')
        {
            cursorX = x;
            cursorY += GetLineHeight(scale);
            continue;
        }

        const int glyph = GlyphIndex(c);
        if (glyph < 0)
            continue;

        const stbtt_packedchar& packed = chars[glyph];
        const float x0 = cursorX + packed.xoff * scale;
        const float y0 = cursorY + packed.yoff * scale;
        const float glyphWidth = static_cast<float>(packed.x1 - packed.x0);
        const float glyphHeight = static_cast<float>(packed.y1 - packed.y0);
        const int startX = static_cast<int>(std::floor(x0));
        const int startY = static_cast<int>(std::floor(y0));
        const float offsetX = x0 - static_cast<float>(startX);
        const float offsetY = y0 - static_cast<float>(startY);
        const int targetWidth = static_cast<int>(std::ceil(glyphWidth * scale + offsetX)) + 1;
        const int targetHeight = static_cast<int>(std::ceil(glyphHeight * scale + offsetY)) + 1;

        auto sampleAtlas = [&](float gx, float gy) -> float
        {
            gx -= 0.5f;
            gy -= 0.5f;
            const int ix = static_cast<int>(std::floor(gx));
            const int iy = static_cast<int>(std::floor(gy));
            const float fx = gx - static_cast<float>(ix);
            const float fy = gy - static_cast<float>(iy);
            auto texel = [&](int cx, int cy) -> float
            {
                if (cx < 0 || cy < 0 || cx >= static_cast<int>(glyphWidth) || cy >= static_cast<int>(glyphHeight))
                    return 0.0f;
                return atlasCoverage[static_cast<size_t>(packed.y0 + cy) * atlasWidth + packed.x0 + cx] / 255.0f;
            };
            const float top = texel(ix, iy) + (texel(ix + 1, iy) - texel(ix, iy)) * fx;
            const float bottom = texel(ix, iy + 1) + (texel(ix + 1, iy + 1) - texel(ix, iy + 1)) * fx;
            return top + (bottom - top) * fy;
        };

        const int taps = scale < 0.75f ? 3 : 2;
        const float tapWeight = 1.0f / static_cast<float>(taps * taps);

        for (int ty = 0; ty < targetHeight; ty++)
        {
            const int py = startY + ty;
            if (py < 0 || py >= height)
                continue;
            for (int tx = 0; tx < targetWidth; tx++)
            {
                const int px = startX + tx;
                if (px < 0 || px >= width)
                    continue;
                float sum = 0.0f;
                for (int sy = 0; sy < taps; sy++)
                {
                    for (int sx = 0; sx < taps; sx++)
                    {
                        const float subX = static_cast<float>(tx) + (static_cast<float>(sx) + 0.5f) / static_cast<float>(taps) - offsetX;
                        const float subY = static_cast<float>(ty) + (static_cast<float>(sy) + 0.5f) / static_cast<float>(taps) - offsetY;
                        sum += sampleAtlas(subX / scale, subY / scale);
                    }
                }
                const float coverage = (std::min)(1.0f, sum * tapWeight * 1.15f) * color.w;
                if (coverage <= 0.0f)
                    continue;
                unsigned char* pixel = rgba + (static_cast<size_t>(py) * width + px) * 4;
                pixel[0] = static_cast<unsigned char>(pixel[0] + (color.x * 255.0f - pixel[0]) * coverage);
                pixel[1] = static_cast<unsigned char>(pixel[1] + (color.y * 255.0f - pixel[1]) * coverage);
                pixel[2] = static_cast<unsigned char>(pixel[2] + (color.z * 255.0f - pixel[2]) * coverage);
            }
        }

        cursorX += packed.xadvance * scale;
    }
}

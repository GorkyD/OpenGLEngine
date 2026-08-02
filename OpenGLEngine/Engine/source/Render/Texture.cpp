#include "Render/Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <glad/glad.h>
#include <unordered_map>

static std::unordered_map<std::string, std::weak_ptr<Texture>> textureCache;

TexturePtr Texture::LoadFromFile(const std::string& path)
{
    const auto cached = textureCache.find(path);
    if (cached != textureCache.end())
    {
        if (TexturePtr existing = cached->second.lock())
            return existing;
    }

    stbi_set_flip_vertically_on_load(true);

    int w, h, ch;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &ch, 0);
    if (!pixels)
    {
        OGL_WARNING("Texture | Failed to load: " << path)
        return nullptr;
    }

    GLenum format = GL_RGB;
    if (ch == 1)
        format = GL_RED;
    else if (ch == 3)
        format = GL_RGB;
    else if (ch == 4)
        format = GL_RGBA;

    unsigned int texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);

    auto texture = std::shared_ptr<Texture>(new Texture());
    texture->id = texId;
    texture->target = GL_TEXTURE_2D;
    texture->width = w;
    texture->height = h;
    texture->channels = ch;

    textureCache[path] = texture;

    OGL_INFO("Texture | Loaded: " << path << " (" << w << "x" << h << ", " << ch << "ch)")
    return texture;
}

TexturePtr Texture::LoadFromMemory(const unsigned char* data, int size)
{
    if (!data || size <= 0)
        return nullptr;

    stbi_set_flip_vertically_on_load(true);

    int w, h, ch;
    unsigned char* pixels = stbi_load_from_memory(data, size, &w, &h, &ch, 4);
    if (!pixels)
    {
        OGL_WARNING("Texture | Failed to decode embedded texture")
        return nullptr;
    }

    TexturePtr texture = CreateFromPixels(pixels, w, h, 4);
    stbi_image_free(pixels);

    if (texture)
        OGL_INFO("Texture | Loaded embedded (" << w << "x" << h << ", " << ch << "ch source)")

    return texture;
}

TexturePtr Texture::LoadCubemap(const std::array<std::string, 6>& facePaths)
{
    stbi_set_flip_vertically_on_load(false);

    unsigned int texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texId);

    int w = 0, h = 0, ch = 0;
    for (unsigned int i = 0; i < facePaths.size(); i++)
    {
        int faceW, faceH, faceCh;
        unsigned char* pixels = stbi_load(facePaths[i].c_str(), &faceW, &faceH, &faceCh, 0);
        if (!pixels)
        {
            OGL_WARNING("Texture | Failed to load cubemap face: " << facePaths[i])
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            glDeleteTextures(1, &texId);
            return nullptr;
        }

        GLenum format = GL_RGB;
        if (faceCh == 1)
            format = GL_RED;
        else if (faceCh == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, faceW, faceH, 0, format, GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);

        w = faceW;
        h = faceH;
        ch = faceCh;
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    auto texture = std::shared_ptr<Texture>(new Texture());
    texture->id = texId;
    texture->target = GL_TEXTURE_CUBE_MAP;
    texture->width = w;
    texture->height = h;
    texture->channels = ch;

    OGL_INFO("Texture | Loaded cubemap (" << w << "x" << h << ")")
    return texture;
}

TexturePtr Texture::CreateFromPixels(const unsigned char* pixels, int width, int height, int channels)
{
    GLenum format = channels == 1 ? GL_RED : GL_RGBA;

    unsigned int texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    auto texture = std::shared_ptr<Texture>(new Texture());
    texture->id = texId;
    texture->target = GL_TEXTURE_2D;
    texture->width = width;
    texture->height = height;
    texture->channels = channels;

    return texture;
}

Texture::~Texture()
{
    if (id)
        glDeleteTextures(1, &id);
}

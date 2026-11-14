#include "Render/ShadowMap.h"
#include <glad/glad.h>
#include "Extension/Extension.h"

ShadowMap::ShadowMap(int resolution) : resolution(resolution)
{
    Create();
}

void ShadowMap::Resize(int newResolution)
{
    if (newResolution == resolution || newResolution <= 0)
        return;
    Destroy();
    resolution = newResolution;
    Create();
}

void ShadowMap::Destroy()
{
    glDeleteFramebuffers(1, &framebufferId);
    glDeleteTextures(1, &depthTextureId);
    framebufferId = 0;
    depthTextureId = 0;
}

void ShadowMap::Create()
{
    glGenTextures(1, &depthTextureId);
    glBindTexture(GL_TEXTURE_2D, depthTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glGenFramebuffers(1, &framebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTextureId, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        OGL_ERROR("ShadowMap | Framebuffer incomplete")

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

ShadowMap::~ShadowMap()
{
    Destroy();
}

void ShadowMap::BeginWrite() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    glViewport(0, 0, resolution, resolution);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::EndWrite() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

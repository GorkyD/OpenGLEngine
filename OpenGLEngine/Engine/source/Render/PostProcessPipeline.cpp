#include "Render/PostProcessPipeline.h"
#include <glad/glad.h>
#include <utility>
#include "Extension/Extension.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/VertexArrayObject.h"

PostProcessPipeline::PostProcessPipeline(RenderEngine* renderEngine, ShaderProgramPtr shader) : renderEngine(renderEngine), shader(std::move(shader))
{
    static const float dummyVertices[3] = {0.0f, 0.0f, 0.0f};
    VertexAttributes attrs[] = {{1}};
    fullscreenVao = renderEngine->CreateVertexArrayObject({(void*)dummyVertices, sizeof(float), 3, attrs, 1});
}

PostProcessPipeline::~PostProcessPipeline()
{
    DestroyTargets();
}

void PostProcessPipeline::DestroyTargets()
{
    if (framebufferId)
        glDeleteFramebuffers(1, &framebufferId);
    if (colorTextureId)
        glDeleteTextures(1, &colorTextureId);
    if (depthRenderbufferId)
        glDeleteRenderbuffers(1, &depthRenderbufferId);

    framebufferId = 0;
    colorTextureId = 0;
    depthRenderbufferId = 0;
}

void PostProcessPipeline::EnsureTargets(int newWidth, int newHeight)
{
    if (newWidth == width && newHeight == height && framebufferId != 0)
        return;

    DestroyTargets();

    width = newWidth;
    height = newHeight;
    if (width <= 0 || height <= 0)
        return;

    glGenTextures(1, &colorTextureId);
    glBindTexture(GL_TEXTURE_2D, colorTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenRenderbuffers(1, &depthRenderbufferId);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbufferId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

    glGenFramebuffers(1, &framebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTextureId, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbufferId);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        OGL_ERROR("PostProcessPipeline | Framebuffer incomplete")

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void PostProcessPipeline::BeginScene(int newWidth, int newHeight)
{
    EnsureTargets(newWidth, newHeight);
    if (!framebufferId)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
}

void PostProcessPipeline::Resolve(float exposure, float vignetteStrength, float vignetteRadius, float vignetteSoftness)
{
    if (!framebufferId || !shader)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    renderEngine->SetShaderProgram(shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTextureId);
    glUniform1i(shader->GetUniformLocation("sceneColor"), 0);
    glUniform1f(shader->GetUniformLocation("exposure"), exposure);
    glUniform1f(shader->GetUniformLocation("vignetteStrength"), vignetteStrength);
    glUniform1f(shader->GetUniformLocation("vignetteRadius"), vignetteRadius);
    glUniform1f(shader->GetUniformLocation("vignetteSoftness"), vignetteSoftness);

    renderEngine->SetVertexArrayObject(fullscreenVao);
    renderEngine->DrawTriangles(List, 3, 0);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

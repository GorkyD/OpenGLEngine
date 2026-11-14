#include "Render/PostProcessPipeline.h"
#include <glad/glad.h>
#include <algorithm>
#include <utility>
#include "Extension/Extension.h"
#include "Ecs/Components/PostProcessComponent.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/VertexArrayObject.h"

namespace
{
unsigned int SolidTexture(unsigned char value)
{
    unsigned int id = 0;
    const unsigned char pixel[4] = {value, value, value, 255};
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}
}

PostProcessPipeline::PostProcessPipeline(RenderEngine* renderEngine, const PostProcessShaders& shaders) : renderEngine(renderEngine), shaders(shaders)
{
    static const float dummyVertices[3] = {0.0f, 0.0f, 0.0f};
    VertexAttributes attrs[] = {{1}};
    fullscreenVao = renderEngine->CreateVertexArrayObject({(void*)dummyVertices, sizeof(float), 3, attrs, 1});
    whiteTextureId = SolidTexture(255);
    blackTextureId = SolidTexture(0);
}

PostProcessPipeline::~PostProcessPipeline()
{
    DestroyTargets();
    glDeleteTextures(1, &whiteTextureId);
    glDeleteTextures(1, &blackTextureId);
}

void PostProcessPipeline::CreateTarget(Target& target, int targetWidth, int targetHeight, bool hdr)
{
    target.width = (std::max)(1, targetWidth);
    target.height = (std::max)(1, targetHeight);
    glGenTextures(1, &target.texture);
    glBindTexture(GL_TEXTURE_2D, target.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, hdr ? GL_RGBA16F : GL_RGBA8, target.width, target.height, 0, GL_RGBA, hdr ? GL_FLOAT : GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &target.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        OGL_ERROR("PostProcessPipeline | Effect target incomplete")
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void PostProcessPipeline::DestroyTarget(Target& target)
{
    if (target.framebuffer)
        glDeleteFramebuffers(1, &target.framebuffer);
    if (target.texture)
        glDeleteTextures(1, &target.texture);
    target = Target();
}

void PostProcessPipeline::DestroyTargets()
{
    if (framebufferId)
        glDeleteFramebuffers(1, &framebufferId);
    if (colorTextureId)
        glDeleteTextures(1, &colorTextureId);
    if (depthTextureId)
        glDeleteTextures(1, &depthTextureId);
    framebufferId = 0;
    colorTextureId = 0;
    depthTextureId = 0;
    for (Target* target : {&aoA, &aoB, &halfA, &halfB, &volumeA, &volumeB, &blurA, &blurB})
        DestroyTarget(*target);
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &depthTextureId);
    glBindTexture(GL_TEXTURE_2D, depthTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &framebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTextureId, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTextureId, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        OGL_ERROR("PostProcessPipeline | Framebuffer incomplete")

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    CreateTarget(aoA, width, height, false);
    CreateTarget(aoB, width, height, false);
    CreateTarget(halfA, width / 2, height / 2, true);
    CreateTarget(halfB, width / 2, height / 2, true);
    CreateTarget(volumeA, width / 2, height / 2, true);
    CreateTarget(volumeB, width / 2, height / 2, true);
    CreateTarget(blurA, width / 2, height / 2, true);
    CreateTarget(blurB, width / 2, height / 2, true);
}

void PostProcessPipeline::BeginScene(int newWidth, int newHeight, int renderHeight)
{
    if (renderHeight > 0 && renderHeight < newHeight && newHeight > 0)
    {
        newWidth = (std::max)(1, static_cast<int>(static_cast<float>(newWidth) * renderHeight / newHeight + 0.5f));
        newHeight = renderHeight;
    }
    EnsureTargets(newWidth, newHeight);
    if (!framebufferId)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    glViewport(0, 0, width, height);
}

void PostProcessPipeline::BeginPass(const Target& target, const ShaderProgramPtr& pass)
{
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    glViewport(0, 0, target.width, target.height);
    renderEngine->SetShaderProgram(pass);
}

void PostProcessPipeline::Bind(const ShaderProgramPtr& pass, const char* name, int unit, unsigned int texture)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(pass->GetUniformLocation(name), unit);
}

void PostProcessPipeline::Draw()
{
    renderEngine->SetVertexArrayObject(fullscreenVao);
    renderEngine->DrawTriangles(List, 3, 0);
}

void PostProcessPipeline::Blur(const ShaderProgramPtr& pass, Target& a, Target& b, bool depthAware, float spread, int iterations)
{
    for (int i = 0; i < iterations; i++)
    {
        BeginPass(b, pass);
        Bind(pass, "source", 0, a.texture);
        Bind(pass, "sceneDepth", 1, depthTextureId);
        glUniform2f(pass->GetUniformLocation("direction"), spread / a.width, 0.0f);
        glUniform1i(pass->GetUniformLocation("depthAware"), depthAware ? 1 : 0);
        glUniform1i(pass->GetUniformLocation("brightPass"), 0);
        Draw();
        BeginPass(a, pass);
        Bind(pass, "source", 0, b.texture);
        Bind(pass, "sceneDepth", 1, depthTextureId);
        glUniform2f(pass->GetUniformLocation("direction"), 0.0f, spread / a.height);
        glUniform1i(pass->GetUniformLocation("depthAware"), depthAware ? 1 : 0);
        glUniform1i(pass->GetUniformLocation("brightPass"), 0);
        Draw();
    }
}

void PostProcessPipeline::Resolve(const PostProcessComponent& settings, const PostFrameData& frame)
{
    if (!framebufferId || !shaders.composite)
        return;

    GLint viewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, viewport);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    frameCounter++;

    const bool ao = settings.aoEnabled && frame.cameraValid && shaders.ambientOcclusion && shaders.blur;
    const bool volume = settings.volumetricEnabled && frame.cameraValid && frame.shadowValid && shaders.volume;
    const bool bloom = settings.bloomEnabled && shaders.blur;
    const bool dof = settings.dofEnabled && frame.cameraValid && shaders.blur;
    const Matrix4 invProjection = frame.projection.Inverse();
    const Matrix4 invView = frame.view.Inverse();

    if (ao)
    {
        const ShaderProgramPtr& pass = shaders.ambientOcclusion;
        BeginPass(aoA, pass);
        Bind(pass, "sceneDepth", 0, depthTextureId);
        glUniformMatrix4fv(pass->GetUniformLocation("invProjection"), 1, GL_FALSE, &invProjection.matrix[0][0]);
        glUniformMatrix4fv(pass->GetUniformLocation("projection"), 1, GL_FALSE, &frame.projection.matrix[0][0]);
        glUniform2f(pass->GetUniformLocation("texelSize"), 1.0f / width, 1.0f / height);
        glUniform1f(pass->GetUniformLocation("radius"), settings.aoRadius);
        glUniform1f(pass->GetUniformLocation("intensity"), settings.aoIntensity);
        glUniform1f(pass->GetUniformLocation("frame"), static_cast<float>(frameCounter % 64));
        Draw();
        Blur(shaders.blur, aoA, aoB, true, 1.0f, 1);
    }

    if (volume)
    {
        const ShaderProgramPtr& pass = shaders.volume;
        BeginPass(volumeA, pass);
        Bind(pass, "sceneDepth", 0, depthTextureId);
        Bind(pass, "shadowMap", 1, frame.shadowTexture);
        glUniformMatrix4fv(pass->GetUniformLocation("invProjection"), 1, GL_FALSE, &invProjection.matrix[0][0]);
        glUniformMatrix4fv(pass->GetUniformLocation("invView"), 1, GL_FALSE, &invView.matrix[0][0]);
        glUniformMatrix4fv(pass->GetUniformLocation("lightSpaceMatrix"), 1, GL_FALSE, &frame.lightSpace.matrix[0][0]);
        glUniform3f(pass->GetUniformLocation("cameraPosition"), frame.cameraPosition.x, frame.cameraPosition.y, frame.cameraPosition.z);
        glUniform3f(pass->GetUniformLocation("sunDirection"), frame.sunDirection.x, frame.sunDirection.y, frame.sunDirection.z);
        glUniform3f(pass->GetUniformLocation("sunColor"), frame.sunColor.x, frame.sunColor.y, frame.sunColor.z);
        glUniform1f(pass->GetUniformLocation("density"), settings.volumetricDensity);
        glUniform1f(pass->GetUniformLocation("maxDistance"), settings.volumetricDistance);
        glUniform1i(pass->GetUniformLocation("steps"), (std::max)(4, settings.volumetricSteps));
        glUniform1f(pass->GetUniformLocation("time"), frame.time);
        Draw();
        Blur(shaders.blur, volumeA, volumeB, false, 1.0f, 1);
    }

    if (bloom)
    {
        const ShaderProgramPtr& pass = shaders.blur;
        BeginPass(halfA, pass);
        Bind(pass, "source", 0, colorTextureId);
        Bind(pass, "sceneDepth", 1, depthTextureId);
        glUniform2f(pass->GetUniformLocation("direction"), 1.0f / width, 0.0f);
        glUniform1i(pass->GetUniformLocation("depthAware"), 0);
        glUniform1i(pass->GetUniformLocation("brightPass"), 1);
        glUniform1f(pass->GetUniformLocation("threshold"), settings.bloomThreshold);
        Draw();
        Blur(pass, halfA, halfB, false, 2.0f, 2);
    }

    if (dof)
    {
        const ShaderProgramPtr& pass = shaders.blur;
        BeginPass(blurA, pass);
        Bind(pass, "source", 0, colorTextureId);
        Bind(pass, "sceneDepth", 1, depthTextureId);
        glUniform2f(pass->GetUniformLocation("direction"), 0.0f, 0.0f);
        glUniform1i(pass->GetUniformLocation("depthAware"), 0);
        glUniform1i(pass->GetUniformLocation("brightPass"), 0);
        Draw();
        Blur(pass, blurA, blurB, false, 1.5f, 1);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    const ShaderProgramPtr& shader = shaders.composite;
    renderEngine->SetShaderProgram(shader);

    Bind(shader, "sceneColor", 0, colorTextureId);
    Bind(shader, "aoTexture", 1, ao ? aoA.texture : whiteTextureId);
    Bind(shader, "volumeTexture", 2, volume ? volumeA.texture : blackTextureId);
    Bind(shader, "bloomTexture", 3, bloom ? halfA.texture : blackTextureId);
    Bind(shader, "blurTexture", 4, dof ? blurA.texture : colorTextureId);
    Bind(shader, "sceneDepth", 5, depthTextureId);
    glUniform1i(shader->GetUniformLocation("aoEnabled"), ao ? 1 : 0);
    glUniform1f(shader->GetUniformLocation("aoStrength"), settings.aoStrength);
    glUniform1i(shader->GetUniformLocation("volumeEnabled"), volume ? 1 : 0);
    glUniform1i(shader->GetUniformLocation("bloomEnabled"), bloom ? 1 : 0);
    glUniform1f(shader->GetUniformLocation("bloomIntensity"), settings.bloomIntensity);
    glUniform1i(shader->GetUniformLocation("dofEnabled"), dof ? 1 : 0);
    glUniform1f(shader->GetUniformLocation("dofFocus"), settings.dofFocus);
    glUniform1f(shader->GetUniformLocation("dofRange"), settings.dofRange);
    glUniform1f(shader->GetUniformLocation("nearPlane"), frame.nearPlane);
    glUniform1f(shader->GetUniformLocation("farPlane"), frame.farPlane);
    glUniform1f(shader->GetUniformLocation("exposure"), settings.exposure);
    glUniform1f(shader->GetUniformLocation("vignetteStrength"), settings.vignetteStrength);
    glUniform1f(shader->GetUniformLocation("vignetteRadius"), settings.vignetteRadius);
    glUniform1f(shader->GetUniformLocation("vignetteSoftness"), settings.vignetteSoftness);

    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    const float virtualHeight = settings.psxEnabled ? settings.psxResolutionHeight : 0.0f;
    glUniform2f(shader->GetUniformLocation("psxResolution"), virtualHeight * aspect, virtualHeight);
    glUniform1f(shader->GetUniformLocation("psxColorLevels"), settings.psxEnabled ? settings.psxColorLevels : 0.0f);
    glUniform1f(shader->GetUniformLocation("psxDitherStrength"), settings.psxDitherStrength);

    Draw();
    glActiveTexture(GL_TEXTURE0);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

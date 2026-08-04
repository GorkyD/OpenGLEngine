#pragma once

#include "Extension/Extension.h"

class RenderEngine;

class PostProcessPipeline
{
public:
    PostProcessPipeline(RenderEngine* renderEngine, ShaderProgramPtr shader);
    ~PostProcessPipeline();

    PostProcessPipeline(const PostProcessPipeline&) = delete;
    PostProcessPipeline& operator=(const PostProcessPipeline&) = delete;

    void BeginScene(int width, int height);
    void Resolve(float exposure, float vignetteStrength, float vignetteRadius, float vignetteSoftness);

private:
    void EnsureTargets(int newWidth, int newHeight);
    void DestroyTargets();

    RenderEngine* renderEngine;
    ShaderProgramPtr shader;
    VertexArrayObjectPtr fullscreenVao;

    unsigned int framebufferId = 0;
    unsigned int colorTextureId = 0;
    unsigned int depthRenderbufferId = 0;

    int width = 0;
    int height = 0;
};

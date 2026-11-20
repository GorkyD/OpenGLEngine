#pragma once

#include "Extension/Extension.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"

class RenderEngine;
struct PostProcessComponent;

struct PostProcessShaders
{
    ShaderProgramPtr composite;
    ShaderProgramPtr ambientOcclusion;
    ShaderProgramPtr blur;
    ShaderProgramPtr volume;
};

struct PostFrameData
{
    Matrix4 view;
    Matrix4 projection;
    Matrix4 lightSpace;
    Vector3 cameraPosition = {0, 0, 0};
    Vector3 sunDirection = {0, 1, 0};
    Vector3 sunColor = {1, 1, 1};
    unsigned int shadowTexture = 0;
    bool shadowValid = false;
    bool cameraValid = false;
    float nearPlane = 0.01f;
    float farPlane = 100.0f;
    float time = 0.0f;
};

class PostProcessPipeline
{
public:
    PostProcessPipeline(RenderEngine* renderEngine, const PostProcessShaders& shaders);
    ~PostProcessPipeline();

    PostProcessPipeline(const PostProcessPipeline&) = delete;
    PostProcessPipeline& operator=(const PostProcessPipeline&) = delete;

    void BeginScene(int width, int height, int renderHeight = 0);
    void Resolve(const PostProcessComponent& settings, const PostFrameData& frame);
    unsigned int DepthTexture() const
    {
        return depthTextureId;
    }

private:
    struct Target
    {
        unsigned int framebuffer = 0;
        unsigned int texture = 0;
        int width = 0;
        int height = 0;
    };

    void EnsureTargets(int newWidth, int newHeight);
    void DestroyTargets();
    void CreateTarget(Target& target, int targetWidth, int targetHeight, bool hdr);
    void DestroyTarget(Target& target);
    void BeginPass(const Target& target, const ShaderProgramPtr& pass);
    void Bind(const ShaderProgramPtr& pass, const char* name, int unit, unsigned int texture);
    void Draw();
    void Blur(const ShaderProgramPtr& pass, Target& a, Target& b, bool depthAware, float spread, int iterations);

    RenderEngine* renderEngine;
    PostProcessShaders shaders;
    VertexArrayObjectPtr fullscreenVao;

    unsigned int framebufferId = 0;
    unsigned int colorTextureId = 0;
    unsigned int depthTextureId = 0;
    unsigned int whiteTextureId = 0;
    unsigned int blackTextureId = 0;

    Target aoA;
    Target aoB;
    Target halfA;
    Target halfB;
    Target volumeA;
    Target volumeB;
    Target blurA;
    Target blurB;

    int width = 0;
    int height = 0;
    int frameCounter = 0;
};

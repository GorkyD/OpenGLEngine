#pragma once

class ShadowMap
{
public:
    explicit ShadowMap(int resolution);
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    void Resize(int newResolution);
    void BeginWrite() const;
    void EndWrite() const;

    unsigned int GetDepthTextureId() const
    {
        return depthTextureId;
    }
    int GetResolution() const
    {
        return resolution;
    }

private:
    void Create();
    void Destroy();
    unsigned int framebufferId = 0;
    unsigned int depthTextureId = 0;
    int resolution = 0;
};

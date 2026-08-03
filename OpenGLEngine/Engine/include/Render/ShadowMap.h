#pragma once

class ShadowMap
{
public:
    explicit ShadowMap(int resolution);
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

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
    unsigned int framebufferId = 0;
    unsigned int depthTextureId = 0;
    int resolution = 0;
};

#pragma once

struct PostProcessComponent
{
    float exposure = 1.0f;
    float vignetteStrength = 0.35f;
    float vignetteRadius = 0.75f;
    float vignetteSoftness = 0.45f;

    bool psxEnabled = false;
    float psxResolutionHeight = 240.0f;
    float psxColorLevels = 32.0f;
    float psxDitherStrength = 1.0f;
    bool psxVertexSnap = true;
    float psxVertexSnapHeight = 160.0f;
};

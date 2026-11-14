#pragma once

struct PostProcessComponent
{
    float exposure = 1.0f;
    float vignetteStrength = 0.35f;
    float vignetteRadius = 0.75f;
    float vignetteSoftness = 0.45f;

    bool psxEnabled = false;
    float psxResolutionHeight = 240.0f;
    bool psxRenderLowRes = false;
    bool aoEnabled = false;
    float aoRadius = 0.25f;
    float aoIntensity = 1.0f;
    float aoStrength = 0.85f;
    bool volumetricEnabled = false;
    float volumetricDensity = 0.12f;
    float volumetricDistance = 6.0f;
    int volumetricSteps = 24;
    bool bloomEnabled = false;
    float bloomThreshold = 1.2f;
    float bloomIntensity = 0.6f;
    bool dofEnabled = false;
    float dofFocus = 0.5f;
    float dofRange = 0.25f;
    float psxColorLevels = 32.0f;
    float psxDitherStrength = 1.0f;
    bool psxVertexSnap = true;
    float psxVertexSnapHeight = 160.0f;
    bool overlayAfterPostProcess = false;
};

#pragma once

#include <memory>
#include <vector>

struct PluckParams
{
    float frequency = 110.0f;
    float volume = 0.5f;
    float brightness = 0.6f;
    float sustain = 0.996f;
    float buzz = 0.0f;
    float pan = 0.0f;
    float drive = 0.0f;
    bool rig = false;
    float pickPosition = 0.2f;
    float pickupTaps[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    int pickupTapCount = 0;
};

enum class RigPickup
{
    SingleCoil,
    Humbucker,
    Piezo
};

enum class RigPedal
{
    TrebleBooster,
    Overdrive,
    Distortion,
    BigFuzz,
    RoughFuzz
};

enum class RigAmp
{
    Direct,
    TransistorCombo,
    CleanTube,
    ChimeTube,
    HighGainTube
};

struct RigPedalSlot
{
    RigPedal type = RigPedal::Overdrive;
    bool on = false;
    float drive = 0.6f;
    float tone = 0.5f;
    float level = 0.6f;
};

struct RigSettings
{
    RigPickup pickup = RigPickup::Humbucker;
    bool customPickup = false;
    float pickupGain = 1.35f;
    float pickupLowCut = 60.0f;
    float pickupResonance = 2300.0f;
    float pickupResonanceDb = 4.0f;
    float pickupQ = 1.2f;
    float pickupCutoff = 4600.0f;
    std::vector<RigPedalSlot> pedals;
    RigAmp amp = RigAmp::CleanTube;
    float gain = 0.5f;
    float bass = 0.5f;
    float middle = 0.5f;
    float treble = 0.5f;
    float volume = 0.6f;
    float reverb = 0.2f;
};

class SynthAudio
{
public:
    static constexpr int SampleRate = 48000;

    SynthAudio();
    ~SynthAudio();

    bool Init();
    void Shutdown();
    bool IsRunning() const;

    void Pluck(const PluckParams& params);
    void Tone(float frequency, float seconds, float volume);
    void StopAll();
    void SetMasterVolume(float volume);
    void SetRig(const RigSettings& settings);

    void RenderOffline(std::vector<float>& outStereo, int frames);

private:
    struct State;
    std::unique_ptr<State> state;
};

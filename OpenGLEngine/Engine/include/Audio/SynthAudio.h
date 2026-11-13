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

    void RenderOffline(std::vector<float>& outStereo, int frames);

private:
    struct State;
    std::unique_ptr<State> state;
};

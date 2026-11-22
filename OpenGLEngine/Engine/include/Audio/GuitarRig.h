#pragma once

#include <array>
#include <vector>
#include "Audio/SynthAudio.h"

class Biquad
{
public:
    void LowPass(float frequency, float q, float sampleRate);
    void HighPass(float frequency, float q, float sampleRate);
    void Peak(float frequency, float q, float gainDb, float sampleRate);
    void LowShelf(float frequency, float gainDb, float sampleRate);
    void HighShelf(float frequency, float gainDb, float sampleRate);
    void HighPassFirst(float frequency, float sampleRate);
    void LowPassFirst(float frequency, float sampleRate);
    void Bypass();
    float Process(float x);
    void Reset();

private:
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float z1 = 0.0f;
    float z2 = 0.0f;
};

class GuitarRig
{
public:
    void Configure(const RigSettings& settings, float sampleRate);
    float Process(float x);
    void Process(float x, float& left, float& right);
    void Reset();

private:
    struct PedalState
    {
        RigPedalSlot slot;
        Biquad pre;
        Biquad shape;
        Biquad tone;
        Biquad post;
        float previous = 0.0f;
        float envelope = 0.0f;
    };

    struct Comb
    {
        std::vector<float> buffer;
        int index = 0;
        float store = 0.0f;
    };

    struct AllPass
    {
        std::vector<float> buffer;
        int index = 0;
    };

    struct Echo
    {
        std::vector<float> left;
        std::vector<float> right;
        int index = 0;
        float time = 0.4f;
        float feedback = 0.4f;
        float mix = 0.0f;
        float wow = 0.0f;
        Biquad toneLeft;
        Biquad toneRight;
        Biquad lowLeft;
        Biquad lowRight;
    };

    struct Room
    {
        std::array<Comb, 8> left;
        std::array<Comb, 8> right;
        std::array<AllPass, 4> passLeft;
        std::array<AllPass, 4> passRight;
        Biquad inputCut;
        Biquad inputTop;
        float feedback = 0.84f;
        float damping = 0.25f;
        float mix = 0.0f;
        float spring = 0.0f;
    };

    float Pedal(PedalState& pedal, float x);
    float Drive(float x);
    float PreAmp(float x);
    float PowerAmp(float x);
    void Delay(float x, float& left, float& right);
    void Reverb(Room& room, float x, float& left, float& right);
    float Shape(float x, float gain, float bias) const;
    static bool TimeBased(RigPedal type);

    RigSettings settings;
    float sampleRate = 48000.0f;
    float overRate = 192000.0f;
    std::array<Biquad, 3> pickup;
    float pickupGain = 1.0f;
    std::vector<PedalState> pedals;
    std::array<Biquad, 4> pre;
    std::array<Biquad, 4> stack;
    std::array<Biquad, 6> cabinet;
    std::array<Biquad, 2> antiAlias;
    std::array<Biquad, 2> inputGuard;
    float preGain = 1.0f;
    float powerGain = 1.0f;
    float outputGain = 1.0f;
    float bias = 0.0f;
    int stages = 1;
    float sag = 0.0f;
    float sagAmount = 0.0f;
    float previousInput = 0.0f;
    float previousSample = 0.0f;
    float ampReverb = 0.0f;
    Room room;
    Room spring;
    Echo echo;
    bool echoOn = false;
    bool springOn = false;
    float humPhase = 0.0f;
    float humLevel = 0.0f;
    float wowPhase = 0.0f;
};

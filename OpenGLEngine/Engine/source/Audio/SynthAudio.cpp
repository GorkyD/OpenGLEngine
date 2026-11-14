#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#include <miniaudio.h>

#include "Audio/SynthAudio.h"
#include "Audio/GuitarRig.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <mutex>
#include <random>

namespace
{
constexpr int MaxVoices = 32;
constexpr float Pi = 3.14159265358979f;

struct Voice
{
    enum class Kind
    {
        Pluck,
        Tone
    };

    Kind kind = Kind::Pluck;
    std::vector<float> line;
    float delay = 0.0f;
    int writeIndex = 0;
    float sustain = 0.996f;
    float brightness = 0.6f;
    float lastSample = 0.0f;
    float buzz = 0.0f;
    float drive = 0.0f;
    float volume = 0.5f;
    float panLeft = 0.7f;
    float panRight = 0.7f;
    float frequency = 440.0f;
    float phase = 0.0f;
    float age = 0.0f;
    float duration = 3.0f;
    float energy = 1.0f;
    float rattle = 0.0f;
    bool finished = false;
    bool rig = false;
    unsigned int noise = 22222u;

    float NextNoise()
    {
        noise = noise * 1664525u + 1013904223u;
        return static_cast<float>((noise >> 8) & 0xFFFF) / 32768.0f - 1.0f;
    }

    float Read(float delaySamples) const
    {
        const int size = static_cast<int>(line.size());
        float readPosition = static_cast<float>(writeIndex) - delaySamples;
        while (readPosition < 0.0f)
            readPosition += static_cast<float>(size);
        const int i0 = static_cast<int>(readPosition) % size;
        const int i1 = (i0 + 1) % size;
        const float fraction = readPosition - std::floor(readPosition);
        return line[i0] + (line[i1] - line[i0]) * fraction;
    }

    float Next()
    {
        const float dt = 1.0f / SynthAudio::SampleRate;
        age += dt;

        if (kind == Kind::Tone)
        {
            phase += 2.0f * Pi * frequency * dt;
            if (phase > 2.0f * Pi)
                phase -= 2.0f * Pi;
            const float envelope = std::exp(-age * 3.2f / duration) * (std::min)(1.0f, age * 400.0f);
            const float strike = std::exp(-age * 40.0f) * 0.4f;
            const float sample = (std::sin(phase) + strike * std::sin(phase * 6.27f)) * envelope * volume;
            if (age > duration)
                finished = true;
            return sample;
        }

        const float delayed = Read(delay);
        const float filtered = sustain * (brightness * delayed + (1.0f - brightness) * lastSample);
        lastSample = filtered;
        line[writeIndex] = filtered;
        writeIndex = (writeIndex + 1) % static_cast<int>(line.size());

        float sample = filtered;
        if (buzz > 0.0f)
        {
            const float level = std::fabs(filtered);
            const float contact = level > 0.06f ? (level - 0.06f) : 0.0f;
            rattle += (NextNoise() * contact - rattle) * 0.18f;
            sample += rattle * buzz * 1.4f * std::exp(-age * 3.0f);
        }

        if (drive > 0.0f)
        {
            const float gain = 1.0f + drive * 14.0f;
            sample = std::tanh(sample * gain) / std::tanh(gain) * 0.55f;
        }
        energy = energy * 0.999f + std::fabs(filtered) * 0.001f;
        if (age > 0.3f && energy < 0.0004f)
            finished = true;
        return sample * volume;
    }
};

Voice MakePluck(const PluckParams& params, unsigned int seed)
{
    Voice voice;
    voice.kind = Voice::Kind::Pluck;
    voice.noise = seed;
    const float frequency = std::clamp(params.frequency, 20.0f, 4000.0f);
    voice.sustain = std::clamp(params.sustain, 0.9f, 0.9999f);
    voice.brightness = std::clamp(params.brightness, 0.5f, 1.0f);
    const float omega = 2.0f * Pi * frequency / static_cast<float>(SynthAudio::SampleRate);
    const float feedback = 1.0f - voice.brightness;
    const float filterDelay = std::atan2(feedback * std::sin(omega), 1.0f - feedback * std::cos(omega)) / omega;
    voice.delay = static_cast<float>(SynthAudio::SampleRate) / frequency - filterDelay;
    voice.line.assign(static_cast<size_t>(voice.delay) + 4, 0.0f);
    voice.buzz = (std::max)(0.0f, params.buzz);
    voice.drive = (std::max)(0.0f, params.drive);
    voice.volume = params.volume;
    voice.frequency = frequency;
    voice.rig = params.rig;

    const int period = (std::max)(2, static_cast<int>(voice.delay));
    const float pickPoint = std::clamp(params.pickPosition, 0.05f, 0.5f);
    float smooth = 0.0f;
    float mean = 0.0f;
    for (int i = 0; i < period; i++)
    {
        const float phase = static_cast<float>(i) / static_cast<float>(period);
        const float shape = phase < pickPoint ? phase / pickPoint : (1.0f - phase) / (1.0f - pickPoint);
        const float raw = shape * 1.6f - 0.8f + voice.NextNoise() * 0.15f;
        smooth = smooth + (raw - smooth) * (0.2f + 0.35f * voice.brightness);
        voice.line[i] = smooth;
        mean += smooth;
    }
    mean /= static_cast<float>(period);
    for (int i = 0; i < period; i++)
        voice.line[i] -= mean;
    voice.writeIndex = period;

    const float pan = std::clamp(params.pan, -1.0f, 1.0f);
    voice.panLeft = std::cos((pan + 1.0f) * Pi * 0.25f);
    voice.panRight = std::sin((pan + 1.0f) * Pi * 0.25f);
    return voice;
}
}

struct SynthAudio::State
{
    ma_device device{};
    bool deviceStarted = false;
    std::mutex mutex;
    std::vector<Voice> voices;
    std::vector<Voice> pending;
    float masterVolume = 0.8f;
    float lowpass[2] = {0.0f, 0.0f};
    unsigned int seed = 12345u;
    bool stopRequested = false;
    GuitarRig rig;
    RigSettings pendingRig;
    bool rigDirty = true;

    void Mix(float* out, unsigned int frames)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (stopRequested)
            {
                voices.clear();
                stopRequested = false;
            }
            for (Voice& voice : pending)
            {
                if (voices.size() >= MaxVoices)
                    voices.erase(voices.begin());
                voices.push_back(std::move(voice));
            }
            pending.clear();
            if (rigDirty)
            {
                rig.Configure(pendingRig, static_cast<float>(SampleRate));
                rigDirty = false;
            }
        }

        for (unsigned int frame = 0; frame < frames; frame++)
        {
            float left = 0.0f;
            float right = 0.0f;
            float bus = 0.0f;
            for (Voice& voice : voices)
            {
                if (voice.finished)
                    continue;
                const float sample = voice.Next();
                if (voice.rig)
                {
                    bus += sample;
                    continue;
                }
                left += sample * voice.panLeft;
                right += sample * voice.panRight;
            }
            const float amp = rig.Process(bus);
            left += amp * 0.7f;
            right += amp * 0.7f;
            lowpass[0] += (left - lowpass[0]) * 0.45f;
            lowpass[1] += (right - lowpass[1]) * 0.45f;
            out[frame * 2 + 0] = std::tanh(lowpass[0] * masterVolume * 0.75f);
            out[frame * 2 + 1] = std::tanh(lowpass[1] * masterVolume * 0.75f);
        }

        voices.erase(std::remove_if(voices.begin(), voices.end(), [](const Voice& voice) { return voice.finished; }), voices.end());
    }

    static void Callback(ma_device* device, void* output, const void* input, ma_uint32 frames)
    {
        (void)input;
        auto* self = static_cast<State*>(device->pUserData);
        self->Mix(static_cast<float*>(output), frames);
    }
};

SynthAudio::SynthAudio() : state(std::make_unique<State>()) {}

SynthAudio::~SynthAudio()
{
    Shutdown();
}

bool SynthAudio::Init()
{
    if (state->deviceStarted)
        return true;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = SampleRate;
    config.dataCallback = &State::Callback;
    config.pUserData = state.get();

    if (ma_device_init(nullptr, &config, &state->device) != MA_SUCCESS)
    {
        std::clog << "OGL_WARNING: SynthAudio | no playback device, running silent" << std::endl;
        return false;
    }
    if (ma_device_start(&state->device) != MA_SUCCESS)
    {
        ma_device_uninit(&state->device);
        std::clog << "OGL_WARNING: SynthAudio | device failed to start, running silent" << std::endl;
        return false;
    }
    state->deviceStarted = true;
    return true;
}

void SynthAudio::Shutdown()
{
    if (!state || !state->deviceStarted)
        return;
    ma_device_uninit(&state->device);
    state->deviceStarted = false;
}

bool SynthAudio::IsRunning() const
{
    return state->deviceStarted;
}

void SynthAudio::Pluck(const PluckParams& params)
{
    std::lock_guard<std::mutex> lock(state->mutex);
    state->seed = state->seed * 747796405u + 2891336453u;
    state->pending.push_back(MakePluck(params, state->seed));
}

void SynthAudio::Tone(float frequency, float seconds, float volume)
{
    Voice voice;
    voice.kind = Voice::Kind::Tone;
    voice.frequency = frequency;
    voice.duration = (std::max)(0.1f, seconds);
    voice.volume = volume;
    std::lock_guard<std::mutex> lock(state->mutex);
    state->pending.push_back(std::move(voice));
}

void SynthAudio::StopAll()
{
    std::lock_guard<std::mutex> lock(state->mutex);
    state->pending.clear();
    state->stopRequested = true;
}

void SynthAudio::SetMasterVolume(float volume)
{
    state->masterVolume = std::clamp(volume, 0.0f, 2.0f);
}

void SynthAudio::SetRig(const RigSettings& settings)
{
    std::lock_guard<std::mutex> lock(state->mutex);
    state->pendingRig = settings;
    state->rigDirty = true;
}

void SynthAudio::RenderOffline(std::vector<float>& outStereo, int frames)
{
    outStereo.assign(static_cast<size_t>(frames) * 2, 0.0f);
    state->Mix(outStereo.data(), static_cast<unsigned int>(frames));
}

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#include <miniaudio.h>

#include "Audio/SynthAudio.h"
#include "Audio/GuitarRig.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
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
    int id = 0;
    float targetDelay = 0.0f;
    float taps[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    int tapCount = 0;
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

        if (targetDelay > 0.0f && std::fabs(targetDelay - delay) > 0.001f)
            delay += (targetDelay - delay) * 0.0015f;
        const float delayed = Read(delay);
        const float filtered = sustain * (brightness * delayed + (1.0f - brightness) * lastSample);
        lastSample = filtered;
        line[writeIndex] = filtered;
        writeIndex = (writeIndex + 1) % static_cast<int>(line.size());

        float sample = filtered;
        if (tapCount > 0)
        {
            float picked = 0.0f;
            for (int i = 0; i < tapCount; i++)
                picked += filtered - Read(1.0f + 2.0f * taps[i] * delay);
            sample = picked / static_cast<float>(tapCount);
        }
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
    voice.targetDelay = voice.delay;
    voice.line.assign(static_cast<size_t>(static_cast<float>(SynthAudio::SampleRate) / (std::max)(20.0f, frequency * 0.2f)) + 8, 0.0f);
    voice.buzz = (std::max)(0.0f, params.buzz);
    voice.drive = (std::max)(0.0f, params.drive);
    voice.volume = params.volume;
    voice.frequency = frequency;
    voice.rig = params.rig;
    voice.tapCount = std::clamp(params.pickupTapCount, 0, 4);
    for (int i = 0; i < voice.tapCount; i++)
        voice.taps[i] = std::clamp(params.pickupTaps[i], 0.01f, 0.45f);

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
    int nextId = 1;
    std::vector<std::pair<int, float>> bends;
    std::vector<std::vector<float>> samples;
    int loopSample = -1;
    float loopTarget = 0.0f;
    float loopGain = 0.0f;
    size_t loopPosition = 0;

    static float DelayFor(const Voice& voice, float frequency)
    {
        const float f = std::clamp(frequency, 20.0f, 4000.0f);
        const float omega = 2.0f * Pi * f / static_cast<float>(SampleRate);
        const float feedback = 1.0f - voice.brightness;
        const float filterDelay = std::atan2(feedback * std::sin(omega), 1.0f - feedback * std::cos(omega)) / omega;
        const float delay = static_cast<float>(SampleRate) / f - filterDelay;
        return std::clamp(delay, 2.0f, static_cast<float>(voice.line.size()) - 4.0f);
    }

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
            for (const auto& bend : bends)
            {
                for (std::vector<Voice>* list : {&voices, &pending})
                {
                    for (Voice& voice : *list)
                    {
                        if (voice.id == bend.first && voice.kind == Voice::Kind::Pluck)
                            voice.targetDelay = DelayFor(voice, bend.second);
                    }
                }
            }
            bends.clear();
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
            if (loopSample >= 0 && loopSample < static_cast<int>(samples.size()) && !samples[loopSample].empty())
            {
                loopGain += (loopTarget - loopGain) * 0.0004f;
                if (loopGain > 0.0005f)
                {
                    const std::vector<float>& data = samples[loopSample];
                    const float value = data[loopPosition % data.size()] * loopGain;
                    left += value;
                    right += value;
                }
                loopPosition = (loopPosition + 1) % samples[loopSample].size();
            }
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

    state->samples.reserve(32);
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

int SynthAudio::Pluck(const PluckParams& params)
{
    std::lock_guard<std::mutex> lock(state->mutex);
    state->seed = state->seed * 747796405u + 2891336453u;
    state->pending.push_back(MakePluck(params, state->seed));
    state->pending.back().id = state->nextId++;
    return state->pending.back().id;
}

void SynthAudio::Bend(int voice, float frequency)
{
    if (voice <= 0)
        return;
    std::lock_guard<std::mutex> lock(state->mutex);
    state->bends.emplace_back(voice, frequency);
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

int SynthAudio::LoadWav(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return -1;
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0)
        return -1;
    auto read16 = [&](size_t at) { return static_cast<uint32_t>(bytes[at]) | (static_cast<uint32_t>(bytes[at + 1]) << 8); };
    auto read32 = [&](size_t at) { return read16(at) | (read16(at + 2) << 16); };
    uint32_t format = 0;
    uint32_t channels = 0;
    uint32_t rate = 0;
    uint32_t bits = 0;
    size_t dataAt = 0;
    size_t dataSize = 0;
    size_t at = 12;
    while (at + 8 <= bytes.size())
    {
        const uint32_t size = read32(at + 4);
        if (std::memcmp(bytes.data() + at, "fmt ", 4) == 0 && at + 24 <= bytes.size())
        {
            format = read16(at + 8);
            channels = read16(at + 10);
            rate = read32(at + 12);
            bits = read16(at + 22);
            if (format == 0xFFFE && at + 34 <= bytes.size())
                format = read16(at + 32);
        }
        else if (std::memcmp(bytes.data() + at, "data", 4) == 0)
        {
            dataAt = at + 8;
            dataSize = (std::min)(static_cast<size_t>(size), bytes.size() - dataAt);
            break;
        }
        at += 8 + size + (size & 1u);
    }
    if (dataAt == 0 || channels == 0 || rate == 0 || (bits != 16 && bits != 24 && bits != 32))
        return -1;
    const size_t frameBytes = static_cast<size_t>(bits / 8) * channels;
    const size_t frames = dataSize / frameBytes;
    std::vector<float> mono(frames);
    for (size_t i = 0; i < frames; i++)
    {
        float sum = 0.0f;
        for (uint32_t c = 0; c < channels; c++)
        {
            const size_t p = dataAt + i * frameBytes + c * (bits / 8);
            float value = 0.0f;
            if (bits == 16)
                value = static_cast<int16_t>(read16(p)) / 32768.0f;
            else if (bits == 24)
                value = static_cast<float>(static_cast<int32_t>((read16(p) << 8) | (static_cast<uint32_t>(bytes[p + 2]) << 24)) >> 8) / 8388608.0f;
            else if (format == 3)
            {
                const uint32_t raw = read32(p);
                std::memcpy(&value, &raw, sizeof(float));
            }
            else
                value = static_cast<float>(static_cast<int32_t>(read32(p))) / 2147483648.0f;
            sum += value;
        }
        mono[i] = sum / static_cast<float>(channels);
    }
    std::vector<float> resampled;
    const double step = static_cast<double>(rate) / SampleRate;
    const size_t outFrames = static_cast<size_t>(frames / step);
    resampled.reserve(outFrames);
    for (size_t i = 0; i < outFrames; i++)
    {
        const double source = i * step;
        const size_t i0 = static_cast<size_t>(source);
        const size_t i1 = (std::min)(i0 + 1, frames - 1);
        const float t = static_cast<float>(source - static_cast<double>(i0));
        resampled.push_back(mono[i0] + (mono[i1] - mono[i0]) * t);
    }
    const size_t fade = (std::min)(resampled.size() / 4, static_cast<size_t>(SampleRate / 5));
    for (size_t i = 0; i < fade; i++)
    {
        const float t = static_cast<float>(i) / static_cast<float>(fade);
        const size_t tail = resampled.size() - fade + i;
        resampled[i] = resampled[i] * t + resampled[tail] * (1.0f - t);
    }
    resampled.resize(resampled.size() - fade);
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->samples.size() >= state->samples.capacity())
        return -1;
    state->samples.push_back(std::move(resampled));
    return static_cast<int>(state->samples.size()) - 1;
}

void SynthAudio::SetLoop(int sample, float volume)
{
    std::lock_guard<std::mutex> lock(state->mutex);
    if (sample != state->loopSample && state->loopGain < 0.001f)
    {
        state->loopSample = sample;
        state->loopPosition = 0;
    }
    state->loopTarget = (std::max)(0.0f, volume);
}

void SynthAudio::RenderOffline(std::vector<float>& outStereo, int frames)
{
    outStereo.assign(static_cast<size_t>(frames) * 2, 0.0f);
    state->Mix(outStereo.data(), static_cast<unsigned int>(frames));
}

#include "Audio/GuitarRig.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr float Pi = 3.14159265358979f;

float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

float Knob(float value, float range)
{
    return (std::clamp(value, 0.0f, 1.0f) - 0.5f) * range;
}
}

void Biquad::LowPass(float frequency, float q, float sampleRate)
{
    const float w = 2.0f * Pi * std::clamp(frequency, 10.0f, sampleRate * 0.45f) / sampleRate;
    const float alpha = std::sin(w) / (2.0f * q);
    const float c = std::cos(w);
    const float a0 = 1.0f + alpha;
    b0 = (1.0f - c) * 0.5f / a0;
    b1 = (1.0f - c) / a0;
    b2 = b0;
    a1 = -2.0f * c / a0;
    a2 = (1.0f - alpha) / a0;
}

void Biquad::HighPass(float frequency, float q, float sampleRate)
{
    const float w = 2.0f * Pi * std::clamp(frequency, 10.0f, sampleRate * 0.45f) / sampleRate;
    const float alpha = std::sin(w) / (2.0f * q);
    const float c = std::cos(w);
    const float a0 = 1.0f + alpha;
    b0 = (1.0f + c) * 0.5f / a0;
    b1 = -(1.0f + c) / a0;
    b2 = b0;
    a1 = -2.0f * c / a0;
    a2 = (1.0f - alpha) / a0;
}

void Biquad::Peak(float frequency, float q, float gainDb, float sampleRate)
{
    const float a = std::pow(10.0f, gainDb / 40.0f);
    const float w = 2.0f * Pi * std::clamp(frequency, 10.0f, sampleRate * 0.45f) / sampleRate;
    const float alpha = std::sin(w) / (2.0f * q);
    const float c = std::cos(w);
    const float a0 = 1.0f + alpha / a;
    b0 = (1.0f + alpha * a) / a0;
    b1 = -2.0f * c / a0;
    b2 = (1.0f - alpha * a) / a0;
    a1 = -2.0f * c / a0;
    a2 = (1.0f - alpha / a) / a0;
}

void Biquad::LowShelf(float frequency, float gainDb, float sampleRate)
{
    const float a = std::pow(10.0f, gainDb / 40.0f);
    const float w = 2.0f * Pi * std::clamp(frequency, 10.0f, sampleRate * 0.45f) / sampleRate;
    const float c = std::cos(w);
    const float alpha = std::sin(w) / 2.0f * std::sqrt(2.0f);
    const float root = 2.0f * std::sqrt(a) * alpha;
    const float a0 = (a + 1.0f) + (a - 1.0f) * c + root;
    b0 = a * ((a + 1.0f) - (a - 1.0f) * c + root) / a0;
    b1 = 2.0f * a * ((a - 1.0f) - (a + 1.0f) * c) / a0;
    b2 = a * ((a + 1.0f) - (a - 1.0f) * c - root) / a0;
    a1 = -2.0f * ((a - 1.0f) + (a + 1.0f) * c) / a0;
    a2 = ((a + 1.0f) + (a - 1.0f) * c - root) / a0;
}

void Biquad::HighShelf(float frequency, float gainDb, float sampleRate)
{
    const float a = std::pow(10.0f, gainDb / 40.0f);
    const float w = 2.0f * Pi * std::clamp(frequency, 10.0f, sampleRate * 0.45f) / sampleRate;
    const float c = std::cos(w);
    const float alpha = std::sin(w) / 2.0f * std::sqrt(2.0f);
    const float root = 2.0f * std::sqrt(a) * alpha;
    const float a0 = (a + 1.0f) - (a - 1.0f) * c + root;
    b0 = a * ((a + 1.0f) + (a - 1.0f) * c + root) / a0;
    b1 = -2.0f * a * ((a - 1.0f) + (a + 1.0f) * c) / a0;
    b2 = a * ((a + 1.0f) + (a - 1.0f) * c - root) / a0;
    a1 = 2.0f * ((a - 1.0f) - (a + 1.0f) * c) / a0;
    a2 = ((a + 1.0f) - (a - 1.0f) * c - root) / a0;
}

void Biquad::Bypass()
{
    b0 = 1.0f;
    b1 = b2 = a1 = a2 = 0.0f;
}

float Biquad::Process(float x)
{
    const float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    if (std::fabs(z1) < 1e-20f)
        z1 = 0.0f;
    if (std::fabs(z2) < 1e-20f)
        z2 = 0.0f;
    return y;
}

void Biquad::Reset()
{
    z1 = z2 = 0.0f;
}

float GuitarRig::Shape(float x, float gain, float shapeBias) const
{
    return std::tanh(x * gain + shapeBias) - std::tanh(shapeBias);
}

void GuitarRig::Configure(const RigSettings& newSettings, float rate)
{
    settings = newSettings;
    sampleRate = rate;
    const float sr = sampleRate;

    switch (settings.pickup)
    {
    case RigPickup::SingleCoil:
        pickup[0].HighPass(80.0f, 0.7f, sr);
        pickup[1].Peak(4200.0f, 1.6f, 5.0f, sr);
        pickup[2].LowPass(7500.0f, 0.7f, sr);
        pickupGain = 0.85f;
        break;
    case RigPickup::Humbucker:
        pickup[0].HighPass(60.0f, 0.7f, sr);
        pickup[1].Peak(2300.0f, 1.2f, 4.0f, sr);
        pickup[2].LowPass(4600.0f, 0.75f, sr);
        pickupGain = 1.35f;
        break;
    case RigPickup::Piezo:
        pickup[0].HighPass(90.0f, 0.7f, sr);
        pickup[1].Peak(1000.0f, 0.8f, -3.0f, sr);
        pickup[2].LowPass(10000.0f, 0.7f, sr);
        pickupGain = 1.0f;
        break;
    }

    std::vector<PedalState> previous = std::move(pedals);
    pedals.clear();
    for (size_t i = 0; i < settings.pedals.size(); i++)
    {
        PedalState state;
        if (i < previous.size())
            state = previous[i];
        state.slot = settings.pedals[i];
        const float d = std::clamp(state.slot.drive, 0.0f, 1.0f);
        const float t = std::clamp(state.slot.tone, 0.0f, 1.0f);
        switch (state.slot.type)
        {
        case RigPedal::TrebleBooster:
            state.pre.HighShelf(1200.0f, Lerp(5.0f, 13.0f, t), sr);
            state.shape.HighPass(250.0f, 0.7f, sr);
            state.tone.LowPass(9000.0f, 0.7f, sr);
            state.post.Bypass();
            break;
        case RigPedal::Overdrive:
            state.pre.HighPass(720.0f, 0.7f, sr);
            state.shape.Peak(700.0f, 0.7f, 3.0f + d * 2.0f, sr);
            state.tone.LowPass(Lerp(900.0f, 5200.0f, t), 0.7f, sr);
            state.post.HighPass(60.0f, 0.7f, sr);
            break;
        case RigPedal::Distortion:
            state.pre.HighPass(120.0f, 0.7f, sr);
            state.shape.Peak(850.0f, 0.8f, -6.0f, sr);
            state.tone.LowPass(Lerp(1500.0f, 6500.0f, t), 0.7f, sr);
            state.post.HighPass(70.0f, 0.7f, sr);
            break;
        case RigPedal::BigFuzz:
            state.pre.HighPass(100.0f, 0.7f, sr);
            state.shape.Peak(1000.0f, 0.6f, -9.0f, sr);
            state.tone.LowPass(Lerp(1200.0f, 5000.0f, t), 0.7f, sr);
            state.post.HighPass(100.0f, 0.7f, sr);
            break;
        case RigPedal::RoughFuzz:
            state.pre.LowPass(6000.0f, 0.7f, sr);
            state.shape.HighPass(90.0f, 0.7f, sr);
            state.tone.LowPass(Lerp(2500.0f, 6000.0f, t), 0.7f, sr);
            state.post.Bypass();
            break;
        }
        pedals.push_back(state);
    }

    for (Biquad& filter : pre)
        filter.Bypass();
    for (Biquad& filter : stack)
        filter.Bypass();
    for (Biquad& filter : cabinet)
        filter.Bypass();

    const float g = std::clamp(settings.gain, 0.0f, 1.0f);
    const float bassDb = Knob(settings.bass, 14.0f);
    const float middleDb = Knob(settings.middle, 12.0f);
    const float trebleDb = Knob(settings.treble, 14.0f);
    reverbMix = 0.0f;
    humLevel = 0.0f;
    sagAmount = 0.0f;
    bias = 0.0f;
    stages = 1;

    switch (settings.amp)
    {
    case RigAmp::Direct:
        preGain = 1.0f;
        powerGain = 0.8f;
        stack[0].LowShelf(120.0f, bassDb, sr);
        stack[1].Peak(800.0f, 0.7f, middleDb, sr);
        stack[2].HighShelf(3000.0f, trebleDb, sr);
        cabinet[0].LowPass(9000.0f, 0.7f, sr);
        outputGain = 0.75f;
        break;
    case RigAmp::TransistorCombo:
        preGain = Lerp(2.5f, 22.0f, g * g);
        bias = 0.12f;
        stages = 2;
        powerGain = 1.6f;
        pre[0].HighPass(160.0f, 0.7f, sr);
        pre[1].LowPass(3200.0f, 0.7f, sr);
        stack[0].Peak(1000.0f, 0.8f, 6.0f + middleDb * 0.5f, sr);
        stack[1].LowShelf(150.0f, bassDb * 0.6f, sr);
        stack[2].HighShelf(2500.0f, trebleDb * 0.6f, sr);
        cabinet[0].HighPass(120.0f, 0.7f, sr);
        cabinet[1].Peak(420.0f, 1.0f, 3.0f, sr);
        cabinet[2].LowPass(3500.0f, 0.8f, sr);
        cabinet[3].LowPass(3500.0f, 1.0f, sr);
        humLevel = 0.004f;
        outputGain = 0.42f;
        break;
    case RigAmp::CleanTube:
        preGain = Lerp(1.0f, 7.0f, g * g);
        bias = 0.05f;
        powerGain = 1.1f;
        sagAmount = 0.3f;
        pre[0].HighPass(70.0f, 0.7f, sr);
        stack[0].LowShelf(120.0f, 2.0f + bassDb, sr);
        stack[1].Peak(500.0f, 0.7f, -4.0f + middleDb, sr);
        stack[2].HighShelf(2500.0f, 3.0f + trebleDb, sr);
        cabinet[0].HighPass(70.0f, 0.7f, sr);
        cabinet[1].Peak(110.0f, 1.2f, 3.0f, sr);
        cabinet[2].Peak(2500.0f, 1.5f, 3.0f, sr);
        cabinet[3].LowPass(5500.0f, 0.7f, sr);
        cabinet[4].LowPass(5500.0f, 1.0f, sr);
        reverbMix = std::clamp(settings.reverb, 0.0f, 1.0f) * 0.55f;
        outputGain = 0.55f;
        break;
    case RigAmp::ChimeTube:
        preGain = Lerp(1.8f, 18.0f, g * g);
        bias = 0.18f;
        powerGain = 1.6f;
        sagAmount = 0.5f;
        pre[0].HighPass(90.0f, 0.7f, sr);
        stack[0].HighShelf(2200.0f, 4.0f + trebleDb, sr);
        stack[1].LowShelf(150.0f, -1.0f + bassDb, sr);
        stack[2].Peak(1200.0f, 0.8f, 2.0f + middleDb * 0.7f, sr);
        cabinet[0].HighPass(85.0f, 0.7f, sr);
        cabinet[1].Peak(150.0f, 1.0f, 2.0f, sr);
        cabinet[2].Peak(2800.0f, 1.5f, 5.0f, sr);
        cabinet[3].LowPass(6000.0f, 0.7f, sr);
        cabinet[4].LowPass(6000.0f, 0.9f, sr);
        reverbMix = std::clamp(settings.reverb, 0.0f, 1.0f) * 0.25f;
        outputGain = 0.5f;
        break;
    case RigAmp::HighGainTube:
        preGain = Lerp(3.0f, 60.0f, g * g);
        bias = 0.22f;
        stages = 3;
        powerGain = 1.4f;
        sagAmount = 0.3f;
        pre[0].HighPass(150.0f, 0.7f, sr);
        pre[1].HighPass(300.0f, 0.7f, sr);
        pre[2].LowPass(7000.0f, 0.7f, sr);
        pre[3].LowPass(6500.0f, 0.7f, sr);
        stack[0].Peak(750.0f, 0.9f, 4.0f + middleDb, sr);
        stack[1].LowShelf(120.0f, bassDb, sr);
        stack[2].HighShelf(3000.0f, 2.0f + trebleDb, sr);
        cabinet[0].HighPass(75.0f, 0.7f, sr);
        cabinet[1].Peak(100.0f, 1.4f, 5.0f, sr);
        cabinet[2].Peak(1300.0f, 1.2f, -4.0f, sr);
        cabinet[3].Peak(2600.0f, 1.6f, 5.0f, sr);
        cabinet[4].LowPass(4800.0f, 0.8f, sr);
        cabinet[5].LowPass(4800.0f, 1.1f, sr);
        reverbMix = std::clamp(settings.reverb, 0.0f, 1.0f) * 0.2f;
        outputGain = 0.42f;
        break;
    }

    static const int combLengths[4] = {1557, 1617, 1491, 1422};
    static const int allPassLengths[2] = {556, 441};
    for (int i = 0; i < 4; i++)
    {
        const size_t length = static_cast<size_t>(combLengths[i] * sampleRate / 44100.0f);
        if (combs[i].buffer.size() != length)
        {
            combs[i].buffer.assign(length, 0.0f);
            combs[i].index = 0;
        }
    }
    for (int i = 0; i < 2; i++)
    {
        const size_t length = static_cast<size_t>(allPassLengths[i] * sampleRate / 44100.0f);
        if (allPasses[i].buffer.size() != length)
        {
            allPasses[i].buffer.assign(length, 0.0f);
            allPasses[i].index = 0;
        }
    }
    reverbTone.HighPass(350.0f, 0.7f, sr);
}

float GuitarRig::Pedal(PedalState& pedal, float x)
{
    const RigPedalSlot& slot = pedal.slot;
    const float d = std::clamp(slot.drive, 0.0f, 1.0f);
    const float level = std::clamp(slot.level, 0.0f, 1.0f) * 2.0f;
    float y = 0.0f;
    switch (slot.type)
    {
    case RigPedal::TrebleBooster:
    {
        const float gain = Lerp(2.0f, 9.0f, d);
        const float boosted = pedal.pre.Process(x);
        y = std::tanh(pedal.shape.Process(boosted) * gain + 0.15f) - std::tanh(0.15f);
        y = pedal.tone.Process(y) * 0.8f;
        break;
    }
    case RigPedal::Overdrive:
    {
        const float gain = Lerp(6.0f, 130.0f, d * d);
        const float high = pedal.pre.Process(x);
        const float clipped = std::tanh(high * gain) * 0.55f;
        y = pedal.post.Process(pedal.tone.Process(pedal.shape.Process(x * 0.6f + clipped)));
        break;
    }
    case RigPedal::Distortion:
    {
        const float gain = Lerp(20.0f, 400.0f, d * d);
        const float input = pedal.pre.Process(x);
        const float hard = std::clamp(input * gain, -1.0f, 1.0f);
        y = std::tanh(hard * 1.6f) * 0.7f;
        y = pedal.post.Process(pedal.tone.Process(pedal.shape.Process(y)));
        break;
    }
    case RigPedal::BigFuzz:
    {
        const float gain = Lerp(12.0f, 80.0f, d);
        const float input = pedal.pre.Process(x);
        const float halfway = (pedal.previous + input) * 0.5f;
        pedal.previous = input;
        const float first = (std::tanh(halfway * gain) + std::tanh(input * gain)) * 0.5f;
        const float second = std::tanh(pedal.post.Process(first) * 8.0f) * 0.75f;
        y = pedal.tone.Process(pedal.shape.Process(second));
        break;
    }
    case RigPedal::RoughFuzz:
    {
        const float gain = Lerp(30.0f, 180.0f, d);
        const float input = pedal.pre.Process(x);
        pedal.envelope += (std::fabs(input) - pedal.envelope) * 0.002f;
        const float gate = std::clamp((pedal.envelope - 0.002f) / 0.02f, 0.0f, 1.0f);
        y = Shape(input, gain, 0.45f) * 0.7f * (0.35f + 0.65f * gate);
        y = pedal.tone.Process(pedal.shape.Process(y));
        break;
    }
    }
    return y * level;
}

float GuitarRig::Amp(float x)
{
    float y = pre[0].Process(x);
    const float stageGain = std::pow(preGain, 1.0f / static_cast<float>(stages));
    for (int i = 0; i < stages; i++)
    {
        const float middle = (previousInput + y) * 0.5f;
        if (i == 0)
            previousInput = y;
        const float shaped = i == 0 ? (Shape(middle, stageGain, bias) + Shape(y, stageGain, bias)) * 0.5f : Shape(y, stageGain, bias);
        y = shaped * (stages > 1 ? 1.2f : 1.0f);
        if (i + 1 < static_cast<int>(pre.size()))
            y = pre[i + 1].Process(y);
    }
    if (stages == 1 && preGain < 1.5f)
        y = Lerp(y, x, 0.5f);
    for (Biquad& filter : stack)
        y = filter.Process(y);
    sag += (std::fabs(y) - sag) * 0.0008f;
    const float power = powerGain / (1.0f + sagAmount * sag * 3.0f);
    y = std::tanh(y * power) / (std::max)(0.6f, std::tanh(power));
    for (Biquad& filter : cabinet)
        y = filter.Process(y);
    return y;
}

float GuitarRig::Reverb(float x)
{
    const float input = reverbTone.Process(x) * 0.25f;
    float out = 0.0f;
    for (Comb& comb : combs)
    {
        const float value = comb.buffer[comb.index];
        comb.store = value * 0.7f + comb.store * 0.3f;
        comb.buffer[comb.index] = input + comb.store * 0.82f;
        comb.index = (comb.index + 1) % static_cast<int>(comb.buffer.size());
        out += value;
    }
    for (AllPass& pass : allPasses)
    {
        const float buffered = pass.buffer[pass.index];
        const float result = buffered - out;
        pass.buffer[pass.index] = out + buffered * 0.5f;
        pass.index = (pass.index + 1) % static_cast<int>(pass.buffer.size());
        out = result;
    }
    return out;
}

float GuitarRig::Process(float x)
{
    float y = x * pickupGain;
    for (Biquad& filter : pickup)
        y = filter.Process(y);
    for (PedalState& pedal : pedals)
    {
        if (pedal.slot.on)
            y = Pedal(pedal, y);
    }
    y = Amp(y) * outputGain * std::clamp(settings.volume, 0.0f, 1.0f) * 2.0f;
    if (humLevel > 0.0f)
    {
        humPhase += 2.0f * Pi * 50.0f / sampleRate;
        if (humPhase > 2.0f * Pi)
            humPhase -= 2.0f * Pi;
        y += (std::sin(humPhase) + 0.4f * std::sin(humPhase * 3.0f)) * humLevel;
    }
    if (reverbMix > 0.0f)
        y += Reverb(y) * reverbMix;
    return y;
}

void GuitarRig::Reset()
{
    for (Biquad& filter : pickup)
        filter.Reset();
    for (PedalState& pedal : pedals)
    {
        pedal.pre.Reset();
        pedal.shape.Reset();
        pedal.tone.Reset();
        pedal.post.Reset();
        pedal.previous = 0.0f;
        pedal.envelope = 0.0f;
    }
    for (Biquad& filter : pre)
        filter.Reset();
    for (Biquad& filter : stack)
        filter.Reset();
    for (Biquad& filter : cabinet)
        filter.Reset();
    for (Comb& comb : combs)
        std::fill(comb.buffer.begin(), comb.buffer.end(), 0.0f);
    for (AllPass& pass : allPasses)
        std::fill(pass.buffer.begin(), pass.buffer.end(), 0.0f);
    sag = 0.0f;
}

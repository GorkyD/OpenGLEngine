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

void Biquad::HighPassFirst(float frequency, float sampleRate)
{
    const float k = std::tan(Pi * std::clamp(frequency, 10.0f, sampleRate * 0.45f) / sampleRate);
    const float norm = 1.0f / (1.0f + k);
    b0 = norm;
    b1 = -norm;
    b2 = 0.0f;
    a1 = (k - 1.0f) * norm;
    a2 = 0.0f;
}

void Biquad::LowPassFirst(float frequency, float sampleRate)
{
    const float k = std::tan(Pi * std::clamp(frequency, 10.0f, sampleRate * 0.45f) / sampleRate);
    const float norm = 1.0f / (1.0f + k);
    b0 = k * norm;
    b1 = k * norm;
    b2 = 0.0f;
    a1 = (k - 1.0f) * norm;
    a2 = 0.0f;
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

bool GuitarRig::TimeBased(RigPedal type)
{
    return type == RigPedal::Delay || type == RigPedal::SpringReverb;
}

void GuitarRig::Configure(const RigSettings& newSettings, float rate)
{
    settings = newSettings;
    sampleRate = rate;
    overRate = rate * 4.0f;
    const float sr = sampleRate;
    const float os = overRate;

    switch (settings.pickup)
    {
    case RigPickup::SingleCoil:
        pickup[0].HighPass(80.0f, 0.7f, os);
        pickup[1].Peak(4200.0f, 1.4f, 4.0f, os);
        pickup[2].LowPass(7500.0f, 0.7f, os);
        pickupGain = 0.85f;
        break;
    case RigPickup::Humbucker:
        pickup[0].HighPass(60.0f, 0.7f, os);
        pickup[1].Peak(2300.0f, 1.1f, 3.5f, os);
        pickup[2].LowPass(5600.0f, 1.0f, os);
        pickupGain = 1.25f;
        break;
    case RigPickup::Piezo:
        pickup[0].HighPass(90.0f, 0.7f, os);
        pickup[1].Peak(1000.0f, 0.8f, -3.0f, os);
        pickup[2].LowPass(10000.0f, 0.7f, os);
        pickupGain = 1.0f;
        break;
    }
    if (settings.customPickup)
    {
        pickup[0].HighPass(settings.pickupLowCut, 0.7f, os);
        pickup[1].Peak(settings.pickupResonance, settings.pickupQ, (std::min)(settings.pickupResonanceDb, 4.0f), os);
        pickup[2].LowPass(settings.pickupCutoff * 1.15f, 0.95f, os);
        pickupGain = settings.pickupGain;
    }

    std::vector<PedalState> previous = std::move(pedals);
    pedals.clear();
    echoOn = false;
    springOn = false;
    for (size_t i = 0; i < settings.pedals.size(); i++)
    {
        PedalState state;
        if (i < previous.size())
            state = previous[i];
        state.slot = settings.pedals[i];
        const float d = std::clamp(state.slot.drive, 0.0f, 1.0f);
        const float t = std::clamp(state.slot.tone, 0.0f, 1.0f);
        const float l = std::clamp(state.slot.level, 0.0f, 1.0f);
        switch (state.slot.type)
        {
        case RigPedal::TrebleBooster:
            state.pre.HighShelf(1200.0f, Lerp(4.0f, 10.0f, t), os);
            state.shape.HighPass(220.0f, 0.7f, os);
            state.tone.LowPass(8000.0f, 0.7f, os);
            state.post.LowPass(9000.0f, 0.7f, os);
            break;
        case RigPedal::Overdrive:
            state.pre.HighPassFirst(650.0f, os);
            state.shape.Peak(720.0f, 0.7f, 3.0f, os);
            state.tone.LowPass(Lerp(1600.0f, 5200.0f, t), 0.7f, os);
            state.post.HighPassFirst(70.0f, os);
            break;
        case RigPedal::Distortion:
            state.pre.HighPass(140.0f, 0.7f, os);
            state.shape.Peak(850.0f, 0.8f, -2.5f, os);
            state.tone.LowPass(Lerp(2400.0f, 6500.0f, t), 0.7f, os);
            state.post.LowPass(7000.0f, 0.7f, os);
            break;
        case RigPedal::BigFuzz:
            state.pre.HighPassFirst(90.0f, os);
            state.shape.Peak(1000.0f, 0.8f, -4.0f, os);
            state.tone.LowPass(Lerp(2200.0f, 6000.0f, t), 0.7f, os);
            state.post.HighPassFirst(70.0f, os);
            break;
        case RigPedal::RoughFuzz:
            state.pre.LowPass(5500.0f, 0.7f, os);
            state.shape.HighPass(90.0f, 0.7f, os);
            state.tone.LowPass(Lerp(2500.0f, 5500.0f, t), 0.7f, os);
            state.post.Bypass();
            break;
        case RigPedal::Delay:
            if (state.slot.on)
            {
                echoOn = true;
                echo.time = Lerp(0.22f, 0.52f, d);
                echo.feedback = Lerp(0.22f, 0.58f, t);
                echo.mix = Lerp(0.2f, 0.55f, l);
                echo.toneLeft.LowPass(Lerp(2400.0f, 4200.0f, t), 0.7f, sr);
                echo.toneRight.LowPass(Lerp(2400.0f, 4200.0f, t), 0.7f, sr);
                echo.lowLeft.HighPass(160.0f, 0.7f, sr);
                echo.lowRight.HighPass(160.0f, 0.7f, sr);
            }
            break;
        case RigPedal::SpringReverb:
            if (state.slot.on)
            {
                springOn = true;
                spring.feedback = Lerp(0.8f, 0.9f, d);
                spring.damping = Lerp(0.45f, 0.18f, t);
                spring.mix = Lerp(0.25f, 0.6f, l);
                spring.spring = 1.0f;
                spring.inputCut.HighPass(320.0f, 0.7f, sr);
                spring.inputTop.LowPass(4200.0f, 0.7f, sr);
            }
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
    antiAlias[0].LowPass(17000.0f, 0.54f, os);
    antiAlias[1].LowPass(17000.0f, 1.31f, os);
    inputGuard[0].LowPass(18000.0f, 0.54f, os);
    inputGuard[1].LowPass(18000.0f, 1.31f, os);

    const float g = std::clamp(settings.gain, 0.0f, 1.0f);
    const float bassDb = Knob(settings.bass, 14.0f);
    const float middleDb = Knob(settings.middle, 12.0f);
    const float trebleDb = Knob(settings.treble, 14.0f);
    ampReverb = 0.0f;
    humLevel = 0.0f;
    sagAmount = 0.0f;
    bias = 0.0f;
    stages = 1;

    switch (settings.amp)
    {
    case RigAmp::Direct:
        preGain = 1.0f;
        powerGain = 0.8f;
        stack[0].LowShelf(120.0f, bassDb, os);
        stack[1].Peak(800.0f, 0.7f, middleDb, os);
        stack[2].HighShelf(3000.0f, trebleDb, os);
        cabinet[0].LowPass(9000.0f, 0.7f, sr);
        outputGain = 0.75f;
        break;
    case RigAmp::TransistorCombo:
        preGain = Lerp(2.0f, 14.0f, g * g);
        bias = 0.1f;
        stages = 2;
        powerGain = 1.4f;
        pre[0].HighPass(150.0f, 0.7f, os);
        pre[1].LowPass(3800.0f, 0.7f, os);
        stack[0].Peak(1000.0f, 0.8f, 4.0f + middleDb * 0.5f, os);
        stack[1].LowShelf(150.0f, bassDb * 0.6f, os);
        stack[2].HighShelf(2500.0f, trebleDb * 0.6f, os);
        cabinet[0].HighPass(110.0f, 0.7f, sr);
        cabinet[1].Peak(420.0f, 1.0f, 2.5f, sr);
        cabinet[2].LowPass(3800.0f, 0.8f, sr);
        cabinet[3].LowPass(4200.0f, 1.0f, sr);
        humLevel = 0.002f;
        ampReverb = std::clamp(settings.reverb, 0.0f, 1.0f) * 0.25f;
        outputGain = 0.5f;
        break;
    case RigAmp::CleanTube:
        preGain = Lerp(1.0f, 5.0f, g * g);
        bias = 0.04f;
        powerGain = 1.0f;
        sagAmount = 0.25f;
        pre[0].HighPass(70.0f, 0.7f, os);
        stack[0].LowShelf(120.0f, 2.0f + bassDb, os);
        stack[1].Peak(500.0f, 0.7f, -4.0f + middleDb, os);
        stack[2].HighShelf(2500.0f, 3.0f + trebleDb, os);
        stack[3].HighShelf(3800.0f, 5.0f, os);
        cabinet[0].HighPass(90.0f, 0.7f, sr);
        cabinet[1].Peak(120.0f, 1.2f, 1.0f, sr);
        cabinet[2].Peak(2400.0f, 1.2f, 3.5f, sr);
        cabinet[3].Peak(4500.0f, 2.0f, -1.5f, sr);
        cabinet[4].LowPass(7500.0f, 0.7f, sr);
        cabinet[5].LowPass(8000.0f, 0.9f, sr);
        ampReverb = std::clamp(settings.reverb, 0.0f, 1.0f) * 0.6f;
        outputGain = 0.6f;
        break;
    case RigAmp::ChimeTube:
        preGain = Lerp(1.6f, 12.0f, g * g);
        bias = 0.14f;
        powerGain = 1.4f;
        sagAmount = 0.4f;
        pre[0].HighPass(90.0f, 0.7f, os);
        stack[0].HighShelf(2200.0f, 3.0f + trebleDb, os);
        stack[1].LowShelf(150.0f, -1.0f + bassDb, os);
        stack[2].Peak(1200.0f, 0.8f, 1.5f + middleDb * 0.7f, os);
        cabinet[0].HighPass(85.0f, 0.7f, sr);
        cabinet[1].Peak(150.0f, 1.0f, 2.0f, sr);
        cabinet[2].Peak(2800.0f, 1.4f, 3.5f, sr);
        cabinet[3].Peak(4800.0f, 2.0f, -2.5f, sr);
        cabinet[4].LowPass(7000.0f, 0.7f, sr);
        cabinet[5].LowPass(7400.0f, 0.9f, sr);
        ampReverb = std::clamp(settings.reverb, 0.0f, 1.0f) * 0.35f;
        outputGain = 0.55f;
        break;
    case RigAmp::HighGainTube:
        preGain = Lerp(2.5f, 32.0f, g * g);
        bias = 0.18f;
        stages = 3;
        powerGain = 1.3f;
        sagAmount = 0.25f;
        pre[0].HighPass(130.0f, 0.7f, os);
        pre[1].HighPass(220.0f, 0.7f, os);
        pre[2].LowPass(6500.0f, 0.7f, os);
        pre[3].LowPass(6000.0f, 0.7f, os);
        stack[0].Peak(750.0f, 0.9f, 3.0f + middleDb, os);
        stack[1].LowShelf(120.0f, bassDb, os);
        stack[2].HighShelf(3000.0f, 1.0f + trebleDb, os);
        cabinet[0].HighPass(80.0f, 0.7f, sr);
        cabinet[1].Peak(110.0f, 1.4f, 4.0f, sr);
        cabinet[2].Peak(1300.0f, 1.2f, -3.0f, sr);
        cabinet[3].Peak(2600.0f, 1.6f, 3.5f, sr);
        cabinet[4].LowPass(5000.0f, 0.8f, sr);
        cabinet[5].LowPass(5400.0f, 1.1f, sr);
        ampReverb = std::clamp(settings.reverb, 0.0f, 1.0f) * 0.25f;
        outputGain = 0.45f;
        break;
    }

    static const int combLengths[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    static const int passLengths[4] = {556, 441, 341, 225};
    auto size = [&](Room& target, float scale, int spread) {
        for (int i = 0; i < 8; i++)
        {
            const size_t left = static_cast<size_t>((combLengths[i] * scale) * sampleRate / 44100.0f);
            const size_t right = static_cast<size_t>((combLengths[i] * scale + spread) * sampleRate / 44100.0f);
            if (target.left[i].buffer.size() != left)
                target.left[i].buffer.assign(left, 0.0f), target.left[i].index = 0;
            if (target.right[i].buffer.size() != right)
                target.right[i].buffer.assign(right, 0.0f), target.right[i].index = 0;
        }
        for (int i = 0; i < 4; i++)
        {
            const size_t left = static_cast<size_t>(passLengths[i] * sampleRate / 44100.0f);
            const size_t right = static_cast<size_t>((passLengths[i] + spread) * sampleRate / 44100.0f);
            if (target.passLeft[i].buffer.size() != left)
                target.passLeft[i].buffer.assign(left, 0.0f), target.passLeft[i].index = 0;
            if (target.passRight[i].buffer.size() != right)
                target.passRight[i].buffer.assign(right, 0.0f), target.passRight[i].index = 0;
        }
    };
    size(room, 1.0f, 23);
    size(spring, 0.62f, 31);
    room.feedback = 0.84f;
    room.damping = 0.3f;
    room.mix = ampReverb;
    room.spring = 0.0f;
    room.inputCut.HighPass(250.0f, 0.7f, sr);
    room.inputTop.LowPass(6000.0f, 0.7f, sr);

    const size_t echoLength = static_cast<size_t>(sampleRate * 0.7f);
    if (echo.left.size() != echoLength)
    {
        echo.left.assign(echoLength, 0.0f);
        echo.right.assign(echoLength, 0.0f);
        echo.index = 0;
    }
}

float GuitarRig::Pedal(PedalState& pedal, float x)
{
    const RigPedalSlot& slot = pedal.slot;
    const float d = std::clamp(slot.drive, 0.0f, 1.0f);
    const float level = std::clamp(slot.level, 0.0f, 1.0f) * 0.9f;
    float y = 0.0f;
    switch (slot.type)
    {
    case RigPedal::TrebleBooster:
    {
        const float gain = Lerp(1.6f, 6.0f, d);
        const float boosted = pedal.pre.Process(x);
        y = std::tanh(pedal.shape.Process(boosted) * gain + 0.12f) - std::tanh(0.12f);
        y = pedal.post.Process(pedal.tone.Process(y)) * 1.4f;
        break;
    }
    case RigPedal::Overdrive:
    {
        const float gain = Lerp(4.0f, 55.0f, d * d);
        const float high = pedal.pre.Process(x) * gain;
        const float clipped = high / (1.0f + std::fabs(high)) * (high > 0.0f ? 1.0f : 0.92f);
        y = pedal.post.Process(pedal.tone.Process(pedal.shape.Process(x * 0.5f + clipped * 0.8f)));
        break;
    }
    case RigPedal::Distortion:
    {
        const float gain = Lerp(10.0f, 110.0f, d * d);
        const float input = pedal.pre.Process(x) * gain;
        const float soft = input / std::sqrt(1.0f + input * input);
        y = std::tanh(soft * 1.8f) * 0.65f;
        y = pedal.post.Process(pedal.tone.Process(pedal.shape.Process(y)));
        break;
    }
    case RigPedal::BigFuzz:
    {
        const float gain = Lerp(8.0f, 45.0f, d);
        const float input = pedal.pre.Process(x);
        const float first = std::tanh(input * gain);
        const float second = std::tanh(pedal.post.Process(first) * 5.0f) * 0.7f;
        y = pedal.tone.Process(pedal.shape.Process(second));
        break;
    }
    case RigPedal::RoughFuzz:
    {
        const float gain = Lerp(20.0f, 110.0f, d);
        const float input = pedal.pre.Process(x);
        pedal.envelope += (std::fabs(input) - pedal.envelope) * 0.0005f;
        const float gate = std::clamp((pedal.envelope - 0.002f) / 0.02f, 0.0f, 1.0f);
        y = Shape(input, gain, 0.4f) * 0.65f * (0.35f + 0.65f * gate);
        y = pedal.tone.Process(pedal.shape.Process(y));
        break;
    }
    default:
        return x;
    }
    return y * level;
}

float GuitarRig::PreAmp(float x)
{
    float y = pre[0].Process(x);
    const float stageGain = std::pow(preGain, 1.0f / static_cast<float>(stages));
    for (int i = 0; i < stages; i++)
    {
        y = Shape(y, stageGain, bias) * (stages > 1 ? 1.15f : 1.0f);
        if (i + 1 < static_cast<int>(pre.size()))
            y = pre[i + 1].Process(y);
    }
    if (stages == 1 && preGain < 1.5f)
        y = Lerp(y, x, 0.5f);
    for (Biquad& filter : stack)
        y = filter.Process(y);
    return y;
}

float GuitarRig::PowerAmp(float x)
{
    sag += (std::fabs(x) - sag) * 0.0002f;
    const float power = powerGain / (1.0f + sagAmount * sag * 3.0f);
    return std::tanh(x * power) / (std::max)(0.6f, std::tanh(power));
}

float GuitarRig::Drive(float x)
{
    float y = x * pickupGain;
    for (Biquad& filter : pickup)
        y = filter.Process(y);
    for (PedalState& pedal : pedals)
    {
        if (pedal.slot.on && !TimeBased(pedal.slot.type))
            y = Pedal(pedal, y);
    }
    return PowerAmp(PreAmp(y));
}

void GuitarRig::Delay(float x, float& left, float& right)
{
    const int size = static_cast<int>(echo.left.size());
    wowPhase += 2.0f * Pi * 0.6f / sampleRate;
    if (wowPhase > 2.0f * Pi)
        wowPhase -= 2.0f * Pi;
    const float wobble = 1.0f + 0.0025f * std::sin(wowPhase);
    auto read = [&](const std::vector<float>& line, float seconds) {
        float position = static_cast<float>(echo.index) - seconds * sampleRate * wobble;
        while (position < 0.0f)
            position += static_cast<float>(size);
        const int i0 = static_cast<int>(position) % size;
        const int i1 = (i0 + 1) % size;
        const float fraction = position - std::floor(position);
        return line[i0] + (line[i1] - line[i0]) * fraction;
    };
    const float tapLeft = read(echo.left, echo.time);
    const float tapRight = read(echo.right, echo.time * 1.5f);
    const float wetLeft = echo.lowLeft.Process(echo.toneLeft.Process(tapLeft));
    const float wetRight = echo.lowRight.Process(echo.toneRight.Process(tapRight));
    echo.left[echo.index] = x + std::tanh(wetRight * echo.feedback * 1.1f);
    echo.right[echo.index] = std::tanh(wetLeft * echo.feedback * 1.1f);
    echo.index = (echo.index + 1) % size;
    left += wetLeft * echo.mix;
    right += wetRight * echo.mix;
}

void GuitarRig::Reverb(Room& target, float x, float& left, float& right)
{
    const float input = target.inputTop.Process(target.inputCut.Process(x)) * 0.08f;
    float outLeft = 0.0f;
    float outRight = 0.0f;
    auto comb = [&](Comb& c, float in) {
        const float value = c.buffer[c.index];
        c.store = value * (1.0f - target.damping) + c.store * target.damping;
        c.buffer[c.index] = in + c.store * target.feedback;
        c.index = (c.index + 1) % static_cast<int>(c.buffer.size());
        return value;
    };
    for (int i = 0; i < 8; i++)
    {
        outLeft += comb(target.left[i], input);
        outRight += comb(target.right[i], input);
    }
    auto pass = [](AllPass& p, float in) {
        const float buffered = p.buffer[p.index];
        const float result = buffered - in;
        p.buffer[p.index] = in + buffered * 0.5f;
        p.index = (p.index + 1) % static_cast<int>(p.buffer.size());
        return result;
    };
    for (int i = 0; i < 4; i++)
    {
        outLeft = pass(target.passLeft[i], outLeft);
        outRight = pass(target.passRight[i], outRight);
    }
    if (target.spring > 0.0f)
    {
        const float flutter = std::sin(wowPhase * 37.0f) * 0.06f;
        outLeft *= 1.0f + flutter;
        outRight *= 1.0f - flutter;
    }
    left += outLeft * target.mix;
    right += outRight * target.mix;
}

void GuitarRig::Process(float x, float& left, float& right)
{
    const float step = 1.0f / 4.0f;
    float driven = 0.0f;
    for (int k = 1; k <= 4; k++)
    {
        float sub = previousSample + (x - previousSample) * (static_cast<float>(k) * step);
        for (Biquad& filter : inputGuard)
            sub = filter.Process(sub);
        float out = Drive(sub);
        for (Biquad& filter : antiAlias)
            out = filter.Process(out);
        driven = out;
    }
    previousSample = x;

    float y = driven;
    for (Biquad& filter : cabinet)
        y = filter.Process(y);
    y *= outputGain * std::clamp(settings.volume, 0.0f, 1.0f) * 2.0f;
    if (humLevel > 0.0f)
    {
        humPhase += 2.0f * Pi * 50.0f / sampleRate;
        if (humPhase > 2.0f * Pi)
            humPhase -= 2.0f * Pi;
        y += (std::sin(humPhase) + 0.4f * std::sin(humPhase * 3.0f)) * humLevel;
    }
    left = y;
    right = y;
    if (echoOn)
        Delay(y, left, right);
    else
        wowPhase += 2.0f * Pi * 0.6f / sampleRate;
    if (springOn)
        Reverb(spring, (left + right) * 0.5f, left, right);
    if (room.mix > 0.0f)
        Reverb(room, (left + right) * 0.5f, left, right);
}

float GuitarRig::Process(float x)
{
    float left = 0.0f;
    float right = 0.0f;
    Process(x, left, right);
    return (left + right) * 0.5f;
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
    for (Room* target : {&room, &spring})
    {
        for (Comb& comb : target->left)
            std::fill(comb.buffer.begin(), comb.buffer.end(), 0.0f);
        for (Comb& comb : target->right)
            std::fill(comb.buffer.begin(), comb.buffer.end(), 0.0f);
        for (AllPass& pass : target->passLeft)
            std::fill(pass.buffer.begin(), pass.buffer.end(), 0.0f);
        for (AllPass& pass : target->passRight)
            std::fill(pass.buffer.begin(), pass.buffer.end(), 0.0f);
    }
    std::fill(echo.left.begin(), echo.left.end(), 0.0f);
    std::fill(echo.right.begin(), echo.right.end(), 0.0f);
    sag = 0.0f;
}

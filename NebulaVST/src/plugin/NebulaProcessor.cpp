#include "NebulaProcessor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"
#include <cmath>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Nebula {

NebulaProcessor::NebulaProcessor() {
    setControllerClass(FUID(0xE3B12002, 0xF94B4E8C, 0x9C5A1B07, 0xA1C5F100));
    for (uint32_t i = 0; i < kNumParams; ++i) {
        const auto& info = paramInfo(i);
        params_[i].store(norm(info, info.def));
    }
}

tresult PLUGIN_API NebulaProcessor::initialize(FUnknown* ctx) {
    tresult r = AudioEffect::initialize(ctx);
    if (r != kResultOk) return r;
    addEventInput(STR16("MIDI In"), 1);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    return kResultOk;
}

tresult PLUGIN_API NebulaProcessor::setBusArrangements(SpeakerArrangement* in, int32 ni,
                                                       SpeakerArrangement* out, int32 no) {
    if (no == 1 && out[0] == SpeakerArr::kStereo)
        return AudioEffect::setBusArrangements(in, ni, out, no);
    return kResultFalse;
}

tresult PLUGIN_API NebulaProcessor::canProcessSampleSize(int32 sym) {
    return sym == kSample32 ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API NebulaProcessor::setActive(TBool state) {
    if (state) {
        sampleRate_ = processSetup.sampleRate;
        for (auto& v : voices_) v.setSampleRate(sampleRate_);
        lfo_.setSampleRate(sampleRate_);
        chorus_.setSampleRate(sampleRate_);
        delay_.setSampleRate(sampleRate_);
        reverb_.setSampleRate(sampleRate_);
        rebuildVoiceParams();
    }
    return AudioEffect::setActive(state);
}

void NebulaProcessor::handleParamChange(uint32_t id, double n) {
    if (id >= kNumParams) return;
    params_[id].store(n);
}

void NebulaProcessor::handleNoteOn(int midi, int vel) {
    int idx = allocateVoice(midi);
    voices_[idx].noteOn(midi, vel);
    voices_[idx].updateParams(cachedVoiceParams_);
}

void NebulaProcessor::handleNoteOff(int midi) {
    for (auto& v : voices_)
        if (v.isActive() && v.midiNote() == midi) v.noteOff();
}

int NebulaProcessor::allocateVoice(int midi) {
    for (int i = 0; i < kVoices; ++i)
        if (!voices_[i].isActive()) return i;
    for (int i = 0; i < kVoices; ++i)
        if (voices_[i].midiNote() == midi) return i;
    return 0;
}

void NebulaProcessor::rebuildVoiceParams() {
    auto V = [&](uint32_t id){ return denorm(paramInfo(id), params_[id].load()); };
    VoiceParams& p = cachedVoiceParams_;
    p.osc1Wave = (int)V(kOsc1Wave); p.osc2Wave = (int)V(kOsc2Wave);
    p.osc1Oct  = V(kOsc1Octave);    p.osc2Oct  = V(kOsc2Octave);
    p.osc1Semi = V(kOsc1Semi);      p.osc2Semi = V(kOsc2Semi);
    p.osc1Fine = V(kOsc1Fine);      p.osc2Fine = V(kOsc2Fine);
    p.osc1Pw   = V(kOsc1Pw);        p.osc2Pw   = V(kOsc2Pw);
    p.mixO1    = V(kMixOsc1);       p.mixO2    = V(kMixOsc2);
    p.mixSub   = V(kMixSub);        p.mixNoise = V(kMixNoise);
    p.filterType = (int)V(kFilterType);
    p.cutoff   = V(kFilterCutoff);  p.q       = V(kFilterQ);
    p.envAmt   = V(kFilterEnvAmt);  p.kbd     = V(kFilterKbd);
    p.drive    = V(kFilterDrive);
    p.fA = V(kFEnvA); p.fD = V(kFEnvD); p.fS = V(kFEnvS); p.fR = V(kFEnvR);
    p.aA = V(kAEnvA); p.aD = V(kAEnvD); p.aS = V(kAEnvS); p.aR = V(kAEnvR);

    lfo_.setWave((Lfo::Wave)(int)V(kLfoWave));
    lfo_.setRate(V(kLfoRate));
    lfoAmount_ = V(kLfoAmount);
    lfoDest_   = (int)V(kLfoDest);

    chorus_.setMode((int)V(kFxChorusMode));
    chorus_.setRate(V(kFxChorusRate));
    chorus_.setDepth(V(kFxChorusDepth));
    chorus_.setMix(V(kFxChorus));

    delay_.setMode((int)V(kFxDelayMode));
    delay_.set(V(kFxDelayTime), V(kFxDelayFb), V(kFxDelay));

    reverb_.setMix(V(kFxReverb));

    masterGain_ = V(kMaster);
}

// Мягкий лимитер, который начинает сжимать только выше ±0.85,
// не убивая динамику.
static inline float softLimit(double x) {
    const double t = 0.85;
    if (x >  t) return (float)( t + (1.0 - t) * std::tanh((x - t) / (1.0 - t)) );
    if (x < -t) return (float)(-t - (1.0 - t) * std::tanh((-x - t) / (1.0 - t)) );
    return (float)x;
}

tresult PLUGIN_API NebulaProcessor::process(ProcessData& data) {
    if (data.inputParameterChanges) {
        int32 n = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < n; ++i) {
            IParamValueQueue* q = data.inputParameterChanges->getParameterData(i);
            if (!q) continue;
            int32 points = q->getPointCount();
            if (points <= 0) continue;
            int32 sampleOffset; ParamValue value;
            if (q->getPoint(points - 1, sampleOffset, value) == kResultOk)
                handleParamChange(q->getParameterId(), value);
        }
        rebuildVoiceParams();
        for (auto& v : voices_) if (v.isActive()) v.updateParams(cachedVoiceParams_);
    }

    if (data.inputEvents) {
        int32 n = data.inputEvents->getEventCount();
        for (int32 i = 0; i < n; ++i) {
            Event e;
            if (data.inputEvents->getEvent(i, e) != kResultOk) continue;
            switch (e.type) {
                case Event::kNoteOnEvent:
                    handleNoteOn(e.noteOn.pitch, (int)(e.noteOn.velocity * 127.0f));
                    break;
                case Event::kNoteOffEvent:
                    handleNoteOff(e.noteOff.pitch);
                    break;
                default: break;
            }
        }
    }

    if (data.numSamples == 0 || data.numOutputs < 1) return kResultOk;
    float* outL = data.outputs[0].channelBuffers32[0];
    float* outR = data.outputs[0].channelBuffers32[1];

    // Динамический скейлинг по числу активных голосов: 1 голос = 0 dB,
    // 16 голосов = -12 dB. Без этого моно-нота слишком тихая.
    int active = 0;
    for (auto& v : voices_) if (v.isActive()) ++active;
    double voiceScale = 1.0;
    if (active > 1) {
        // sqrt-нормировка — компромисс между «каждый голос громкий»
        // и «не клиппает на 16 голосах»
        voiceScale = 1.0 / std::sqrt((double)active);
    }

    for (int32 s = 0; s < data.numSamples; ++s) {
        double l = lfo_.process() * lfoAmount_;
        double pitchMod = 0, filtMod = 0, ampMod = 1.0, pwMod = 0;
        switch (lfoDest_) {
            case 0: pitchMod = l * 1.0; break;
            case 1: filtMod  = l * 4000.0; break;
            case 2: ampMod   = 1.0 - lfoAmount_ * 0.5 + 0.5 * l * lfoAmount_; break;
            case 3: pwMod    = l * 0.45; break;
        }

        double mono = 0.0;
        for (auto& v : voices_)
            if (v.isActive())
                mono += v.renderSample(pitchMod, filtMod, pwMod);
        mono *= voiceScale * ampMod;

        // Стерео-цепочка FX
        float chL, chR;
        chorus_.process((float)mono, (float)mono, chL, chR);
        float dlL, dlR;
        delay_.process(chL, chR, dlL, dlR);
        float rvL, rvR;
        reverb_.process(0.5f * (dlL + dlR), rvL, rvR);

        outL[s] = softLimit(rvL * masterGain_);
        outR[s] = softLimit(rvR * masterGain_);
    }

    return kResultOk;
}

tresult PLUGIN_API NebulaProcessor::setState(IBStream* state) {
    IBStreamer s(state, kLittleEndian);
    for (uint32_t i = 0; i < kNumParams; ++i) {
        double v = 0.0;
        if (!s.readDouble(v)) return kResultOk;
        params_[i].store(v);
    }
    return kResultOk;
}

tresult PLUGIN_API NebulaProcessor::getState(IBStream* state) {
    IBStreamer s(state, kLittleEndian);
    for (uint32_t i = 0; i < kNumParams; ++i)
        s.writeDouble(params_[i].load());
    return kResultOk;
}

} // namespace

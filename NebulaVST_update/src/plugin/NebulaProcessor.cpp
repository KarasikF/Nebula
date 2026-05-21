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

NebulaProcessor* NebulaProcessor::s_current = nullptr;

NebulaProcessor::NebulaProcessor() {
    setControllerClass(FUID(0xE3B12002, 0xF94B4E8C, 0x9C5A1B07, 0xA1C5F100));
    for (uint32_t i = 0; i < kNumParams; ++i) {
        const auto& info = paramInfo(i);
        params_[i].store(norm(info, info.def));
    }
    for (int i=0;i<kQ;i++) uiQueue_[i].packed.store(0);
    s_current = this;
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
    Voice& v = voices_[idx];

    bool legato = (notesHeld_ > 0);
    if (cachedVoiceParams_.glideMode == 1 && !legato) {
        v.setLastFreq(440.0 * std::pow(2.0, (midi - 69) / 12.0));
    } else if (lastFreq_ > 0) {
        v.setLastFreq(lastFreq_);
    }

    panRng_ = panRng_ * 1664525u + 1013904223u;
    double r = ((int32_t)panRng_) * (1.0 / 2147483648.0);
    v.pan = r * cachedVoiceParams_.spread;

    v.noteOn(midi, vel, legato);
    v.updateParams(cachedVoiceParams_);
    notesHeld_++;
    lastFreq_ = 440.0 * std::pow(2.0, (midi - 69) / 12.0);
}

void NebulaProcessor::handleNoteOff(int midi) {
    for (auto& v : voices_)
        if (v.isActive() && v.midiNote() == midi) {
            v.noteOff();
            if (notesHeld_ > 0) notesHeld_--;
        }
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
    p.osc1On   = V(kOsc1On) > 0.5;   p.osc2On = V(kOsc2On) > 0.5;
    p.osc1Oct  = V(kOsc1Octave);    p.osc2Oct  = V(kOsc2Octave);
    p.osc1Semi = V(kOsc1Semi);      p.osc2Semi = V(kOsc2Semi);
    p.osc1Fine = V(kOsc1Fine);      p.osc2Fine = V(kOsc2Fine);
    p.osc1Pw   = V(kOsc1Pw);        p.osc2Pw   = V(kOsc2Pw);
    p.mixO1    = V(kMixOsc1);       p.mixO2    = V(kMixOsc2);
    p.mixSub   = V(kMixSub);        p.mixNoise = V(kMixNoise);
    p.noiseColor = (int)V(kNoiseColor);

    p.glide     = V(kGlide);
    p.glideMode = (int)V(kGlideMode);
    p.spread    = V(kSpread);
    p.drift     = V(kAnalogDrift);

    p.filterOn   = V(kFilterOn) > 0.5;
    p.filterType = (int)V(kFilterType);
    p.cutoff   = V(kFilterCutoff);  p.q       = V(kFilterQ);
    p.envAmt   = V(kFilterEnvAmt);  p.kbd     = V(kFilterKbd);
    p.driveType = (int)V(kDriveType);
    p.driveAmt  = V(kDriveAmount);

    p.fA = V(kFEnvA); p.fD = V(kFEnvD); p.fS = V(kFEnvS); p.fR = V(kFEnvR);
    p.aA = V(kAEnvA); p.aD = V(kAEnvD); p.aS = V(kAEnvS); p.aR = V(kAEnvR);

    lfo_.setWave((Lfo::Wave)(int)V(kLfoWave));
    lfo_.setRate(V(kLfoRate));
    lfoAmount_ = V(kLfoAmount);
    lfoDest_   = (int)V(kLfoDest);

    chorusOn_ = V(kFxChorusOn) > 0.5;
    chorus_.setMode((int)V(kFxChorusMode));
    chorus_.setRate(V(kFxChorusRate));
    chorus_.setDepth(V(kFxChorusDepth));
    chorus_.setMix(V(kFxChorus));

    delayOn_ = V(kFxDelayOn) > 0.5;
    delay_.setMode((int)V(kFxDelayMode));
    delay_.set(V(kFxDelayTime), V(kFxDelayFb), V(kFxDelay));

    reverbOn_ = V(kFxReverbOn) > 0.5;
    reverb_.setMix(V(kFxReverb));
    reverb_.setSize(V(kFxReverbSize));
    reverb_.setDamp(V(kFxReverbDamp));

    masterGain_ = V(kMaster);
}

static inline float softLimit(double x) {
    const double t = 0.85;
    if (x >  t) return (float)( t + (1.0 - t) * std::tanh((x - t) / (1.0 - t)) );
    if (x < -t) return (float)(-t - (1.0 - t) * std::tanh((-x - t) / (1.0 - t)) );
    return (float)x;
}

void NebulaProcessor::postUiNote(int midi, int vel, bool on) {
    int t = uiTail_.load(std::memory_order_relaxed);
    int next = (t + 1) % kQ;
    if (next == uiHead_.load(std::memory_order_acquire)) return; // полный
    uint32_t pack = (uint32_t)(midi & 0x7F) | ((uint32_t)(vel & 0x7F) << 7) | (on ? 0x80000000u : 0);
    uiQueue_[t].packed.store(pack, std::memory_order_release);
    uiTail_.store(next, std::memory_order_release);
}

void NebulaProcessor::drainUiQueue() {
    int h = uiHead_.load(std::memory_order_relaxed);
    int t = uiTail_.load(std::memory_order_acquire);
    while (h != t) {
        uint32_t pack = uiQueue_[h].packed.load(std::memory_order_acquire);
        int midi = pack & 0x7F;
        int vel  = (pack >> 7) & 0x7F;
        bool on  = (pack & 0x80000000u) != 0;
        if (on) handleNoteOn(midi, vel);
        else    handleNoteOff(midi);
        h = (h + 1) % kQ;
    }
    uiHead_.store(h, std::memory_order_release);
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

    drainUiQueue();

    if (data.numSamples == 0 || data.numOutputs < 1) return kResultOk;
    float* outL = data.outputs[0].channelBuffers32[0];
    float* outR = data.outputs[0].channelBuffers32[1];

    const double voiceScale = 1.0 / 3.5;

    for (int32 s = 0; s < data.numSamples; ++s) {
        double l = lfo_.process() * lfoAmount_;
        double pitchMod = 0, filtMod = 0, ampMod = 1.0, pwMod = 0;
        switch (lfoDest_) {
            case 0: pitchMod = l * 1.0; break;
            case 1: filtMod  = l * 4000.0; break;
            case 2: ampMod   = 1.0 - lfoAmount_ * 0.5 + 0.5 * l * lfoAmount_; break;
            case 3: pwMod    = l * 0.45; break;
        }

        float sumL = 0, sumR = 0;
        for (auto& v : voices_) {
            if (v.isActive()) {
                float vL, vR;
                v.renderSample(pitchMod, filtMod, pwMod, vL, vR);
                sumL += vL; sumR += vR;
            }
        }
        sumL *= (float)(voiceScale * ampMod);
        sumR *= (float)(voiceScale * ampMod);

        float wL = sumL, wR = sumR;
        if (chorusOn_) { float a,b; chorus_.process(wL, wR, a, b); wL = a; wR = b; }
        if (delayOn_)  { float a,b; delay_.process (wL, wR, a, b); wL = a; wR = b; }
        if (reverbOn_) { float a,b; reverb_.process(wL, wR, a, b); wL = a; wR = b; }

        outL[s] = softLimit(wL * masterGain_);
        outR[s] = softLimit(wR * masterGain_);
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

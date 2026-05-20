#pragma once
#include "PolyBlepOsc.h"
#include "SvfFilter.h"
#include "Envelope.h"
#include <cstdint>

namespace Nebula {

struct VoiceParams {
    int    osc1Wave=2, osc2Wave=2;
    double osc1Oct=0, osc1Semi=0, osc1Fine=0, osc1Pw=0.5;
    double osc2Oct=0, osc2Semi=-12, osc2Fine=7, osc2Pw=0.5;
    double mixO1=0.7, mixO2=0.5, mixSub=0.2, mixNoise=0.0;
    int    filterType=0;
    double cutoff=2000, q=1.0, envAmt=0.5, kbd=0.3, drive=1.2;
    double fA=0.01, fD=0.4, fS=0.3, fR=0.4;
    double aA=0.01, aD=0.3, aS=0.8, aR=0.4;
};

class Voice {
public:
    void setSampleRate(double sr);
    void noteOn(int midi, int velocity);
    void noteOff();
    bool isActive() const { return aEnv_.isActive(); }
    int  midiNote() const { return midi_; }

    // Pull current parameters from shared snapshot before each render block.
    void updateParams(const VoiceParams& p);

    // Generate one sample (mono). LFO modulation is added by Processor through
    // direct field access to keep API tight.
    float renderSample(double pitchModSemi, double filterModHz, double pwMod);

private:
    double sr_ = 48000.0;
    int midi_ = 60;
    double vel_ = 1.0;
    double baseFreq_ = 440.0;

    PolyBlepOsc osc1_, osc2_, sub_;
    SvfFilter   filt_;
    AdsrEnv     fEnv_, aEnv_;
    VoiceParams p_;

    // simple LCG noise
    uint32_t rngState_ = 0x1234567u;
    inline float noise(){
        rngState_ = rngState_ * 1664525u + 1013904223u;
        return ((int32_t)rngState_) * (1.0f / 2147483648.0f);
    }
};

} // namespace

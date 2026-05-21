#pragma once
#include "PolyBlepOsc.h"
#include "SvfFilter.h"
#include "Envelope.h"
#include "Noise.h"
#include "Drive.h"
#include <cstdint>

namespace Nebula {

struct VoiceParams {
    int    osc1Wave=2, osc2Wave=2;
    bool   osc1On=true, osc2On=false;
    double osc1Oct=0, osc1Semi=0, osc1Fine=0, osc1Pw=0.5;
    double osc2Oct=0, osc2Semi=0, osc2Fine=0, osc2Pw=0.5;
    double mixO1=0.7, mixO2=0.5, mixSub=0.0, mixNoise=0.0;
    int    noiseColor = 0;
    bool   filterOn = true;
    int    filterType=0;
    double cutoff=20000, q=0.7, envAmt=0.0, kbd=0.0;
    int    driveType = 0;
    double driveAmt  = 0.0;
    double fA=0.001, fD=0.1, fS=1.0, fR=0.1;
    double aA=0.001, aD=0.1, aS=1.0, aR=0.01;
    double glide = 0.0;
    int    glideMode = 0;   // 0 = always, 1 = legato
    double spread = 0.3;
    double drift  = 0.25;
};

class Voice {
public:
    void setSampleRate(double sr);
    void noteOn(int midi, int velocity, bool legatoFrom);
    void noteOff();
    bool isActive() const { return aEnv_.isActive(); }
    int  midiNote() const { return midi_; }

    void updateParams(const VoiceParams& p);
    // pan ∈ [-1..1]; stereo out
    void renderSample(double pitchModSemi, double filterModHz, double pwMod,
                      float& outL, float& outR);

    // воспроизведение чужой частоты (для glide) — это глобальная история нот
    void setLastFreq(double f){ portaCur_ = f; }
    double currentFreq() const { return portaCur_; }

    double pan = 0.0;

private:
    double sr_ = 48000.0;
    int midi_ = 60;
    double vel_ = 1.0;
    double targetFreq_ = 440.0;
    double portaCur_   = 440.0;  // плавно идёт к targetFreq_
    double portaCoef_  = 0.0;

    PolyBlepOsc osc1_, osc2_, sub_;
    SvfFilter   filt_;
    AdsrEnv     fEnv_, aEnv_;
    ColoredNoise noise_;
    Drive       drive_;
    VoiceParams p_;

    // дрейф «аналогового» осциллятора — медленный LFO ~0.5 Hz
    double driftPhase_ = 0.0, driftRate_ = 0.5;
    uint32_t driftRng_ = 0xC0FFEEu;
};

} // namespace

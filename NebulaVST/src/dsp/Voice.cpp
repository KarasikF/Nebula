#include "Voice.h"
#include <cmath>

namespace Nebula {

static inline double midiToFreq(int n){ return 440.0 * std::pow(2.0, (n - 69) / 12.0); }
static inline double clamp(double v, double a, double b){ return v<a?a:(v>b?b:v); }

void Voice::setSampleRate(double sr){
    sr_ = sr;
    osc1_.setSampleRate(sr);
    osc2_.setSampleRate(sr);
    sub_.setSampleRate(sr);
    filt_.setSampleRate(sr);
    fEnv_.setSampleRate(sr);
    aEnv_.setSampleRate(sr);
}

void Voice::noteOn(int midi, int velocity){
    midi_ = midi;
    vel_  = velocity / 127.0;
    baseFreq_ = midiToFreq(midi);
    osc1_.reset(); osc2_.reset(0.13); sub_.reset(0.27);
    filt_.reset();
    fEnv_.noteOn();
    aEnv_.noteOn();
}

void Voice::noteOff(){
    fEnv_.noteOff();
    aEnv_.noteOff();
}

void Voice::updateParams(const VoiceParams& p){
    p_ = p;
    osc1_.setWave((PolyBlepOsc::Wave)p.osc1Wave);
    osc2_.setWave((PolyBlepOsc::Wave)p.osc2Wave);
    sub_.setWave(PolyBlepOsc::Triangle);
    osc1_.setPulseWidth(p.osc1Pw);
    osc2_.setPulseWidth(p.osc2Pw);

    double f1 = baseFreq_ * std::pow(2.0, p.osc1Oct + p.osc1Semi/12.0 + p.osc1Fine/1200.0);
    double f2 = baseFreq_ * std::pow(2.0, p.osc2Oct + p.osc2Semi/12.0 + p.osc2Fine/1200.0);
    osc1_.setFrequency(f1);
    osc2_.setFrequency(f2);
    sub_.setFrequency(baseFreq_ * 0.5);

    filt_.setType((SvfFilter::Type)p.filterType);
    fEnv_.set(p.fA, p.fD, p.fS, p.fR);
    aEnv_.set(p.aA, p.aD, p.aS, p.aR);
}

float Voice::renderSample(double pitchModSemi, double filterModHz, double pwMod){
    if (pitchModSemi != 0.0) {
        double r = std::pow(2.0, pitchModSemi/12.0);
        osc1_.setFrequency(baseFreq_ * std::pow(2.0, p_.osc1Oct + p_.osc1Semi/12.0 + p_.osc1Fine/1200.0) * r);
        osc2_.setFrequency(baseFreq_ * std::pow(2.0, p_.osc2Oct + p_.osc2Semi/12.0 + p_.osc2Fine/1200.0) * r);
    }
    if (pwMod != 0.0){
        osc1_.setPulseWidth(clamp(p_.osc1Pw + pwMod, 0.05, 0.95));
        osc2_.setPulseWidth(clamp(p_.osc2Pw + pwMod, 0.05, 0.95));
    }

    // Уровни осцилляторов: на максимуме каждый даёт около ±1.
    // Sub чуть громче, чтобы был слышен (как у Moog).
    double o1 = osc1_.process() * p_.mixO1;
    double o2 = osc2_.process() * p_.mixO2;
    double sb = sub_.process()  * p_.mixSub * 1.2;
    double ns = noise()         * p_.mixNoise;
    double mixed = o1 + o2 + sb + ns;

    // Drive: компенсируем потерю громкости. Без драйва (=1.0) — bypass.
    if (p_.drive > 1.01) {
        // softклип tanh(k*x)/tanh(k) ≈ сохраняет пик ±1
        double k = p_.drive;
        mixed = std::tanh(k * mixed) / std::tanh(k);
    }

    double fenv = fEnv_.process();
    double kbdSemi = (midi_ - 60) * p_.kbd;
    double cutoff = p_.cutoff * std::pow(2.0, kbdSemi/12.0);
    cutoff += p_.envAmt * 8000.0 * fenv + filterModHz;
    cutoff = clamp(cutoff, 20.0, sr_ * 0.45);
    filt_.setParams(cutoff, p_.q);
    double y = filt_.process(mixed);

    double aenv = aEnv_.process();
    // Полный выход голоса. Скейлинг по числу голосов сделаем выше.
    return (float)(y * aenv * vel_);
}

} // namespace

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
    driftRate_ = 0.3 + (driftRng_ & 0xFF) / 256.0; // лёгкий разброс
}

void Voice::noteOn(int midi, int velocity, bool legatoFrom){
    midi_ = midi;
    vel_  = velocity / 127.0;
    targetFreq_ = midiToFreq(midi);

    // Если portaCur_ ещё «играет» от предыдущей ноты — начнём glide.
    // Иначе сразу прыгаем в новую частоту.
    if (!aEnv_.isActive() || !legatoFrom) {
        // не legato — старт с targetFreq, либо с portaCur если он есть
        // если portaCur_=0 (первый запуск), приравниваем
        if (portaCur_ < 1.0) portaCur_ = targetFreq_;
    }
    // обновим glide-коэффициент по текущему p_.glide ниже в updateParams

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

    filt_.setType((SvfFilter::Type)p.filterType);
    fEnv_.set(p.fA, p.fD, p.fS, p.fR);
    aEnv_.set(p.aA, p.aD, p.aS, p.aR);
    noise_.setColor(p.noiseColor);
    drive_.setType(p.driveType);
    drive_.setAmount(p.driveAmt);

    // portamento: коэффициент one-pole filter. glide=0 -> мгновенно.
    if (p.glide <= 0.001) portaCoef_ = 1.0;
    else portaCoef_ = 1.0 - std::exp(-1.0 / (p.glide * sr_));
}

void Voice::renderSample(double pitchModSemi, double filterModHz, double pwMod,
                         float& outL, float& outR){
    // 1) portamento step
    portaCur_ += (targetFreq_ - portaCur_) * portaCoef_;

    // 2) analog drift (медленный sin)
    double driftN = 0.0;
    if (p_.drift > 0.001) {
        driftPhase_ += driftRate_ / sr_;
        if (driftPhase_ >= 1.0) driftPhase_ -= 1.0;
        driftN = std::sin(2.0 * 3.14159265 * driftPhase_) * p_.drift * 0.015; // ±1.5 cents max
    }

    // 3) частоты с детюном/модуляцией
    double base = portaCur_ * std::pow(2.0, driftN);
    double pitchMul = std::pow(2.0, pitchModSemi / 12.0);
    double f1 = base * std::pow(2.0, p_.osc1Oct + p_.osc1Semi/12.0 + p_.osc1Fine/1200.0) * pitchMul;
    double f2 = base * std::pow(2.0, p_.osc2Oct + p_.osc2Semi/12.0 + p_.osc2Fine/1200.0) * pitchMul;
    osc1_.setFrequency(f1);
    osc2_.setFrequency(f2);
    sub_.setFrequency(base * 0.5);

    // 4) PW mod — только для square (для saw тоже эффект ноль)
    if (pwMod != 0.0){
        if (p_.osc1Wave == PolyBlepOsc::Square)
            osc1_.setPulseWidth(clamp(p_.osc1Pw + pwMod, 0.05, 0.95));
        if (p_.osc2Wave == PolyBlepOsc::Square)
            osc2_.setPulseWidth(clamp(p_.osc2Pw + pwMod, 0.05, 0.95));
    }

    // 5) сэмплируем источники
    double o1 = p_.osc1On ? osc1_.process() * p_.mixO1 : 0.0;
    double o2 = p_.osc2On ? osc2_.process() * p_.mixO2 : 0.0;
    double sb = sub_.process()   * p_.mixSub * 1.2;
    double ns = noise_.process() * p_.mixNoise;
    double mixed = o1 + o2 + sb + ns;

    // 6) drive (новый, с типами)
    mixed = drive_.process(mixed);

    // 7) фильтр
    double y = mixed;
    if (p_.filterOn) {
        double fenv = fEnv_.process();
        double kbdSemi = (midi_ - 60) * p_.kbd;
        double cutoff = p_.cutoff * std::pow(2.0, kbdSemi/12.0);
        cutoff += p_.envAmt * 8000.0 * fenv + filterModHz;
        cutoff = clamp(cutoff, 20.0, sr_ * 0.45);
        filt_.setParams(cutoff, p_.q);
        y = filt_.process(mixed);
    } else {
        fEnv_.process(); // всё равно тикаем энв (чтобы не было застойного состояния)
    }

    // 8) amp env + velocity
    double aenv = aEnv_.process();
    double mono = y * aenv * vel_;

    // 9) stereo панорама — equal-power
    double pp = 0.5 * (pan + 1.0);          // [0..1]
    double lG = std::sqrt(1.0 - pp);
    double rG = std::sqrt(pp);
    outL = (float)(mono * lG);
    outR = (float)(mono * rG);
}

} // namespace

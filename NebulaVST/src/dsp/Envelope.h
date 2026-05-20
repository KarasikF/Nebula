#pragma once
#include <cmath>

namespace Nebula {

// Экспоненциальный ADSR по схеме Nigel Redmon (earlevel.com).
// Времена A/D/R задаются в секундах. Атака идёт от текущего value к 1.0,
// Decay — от 1.0 к Sustain, Release — от текущего value к 0.0.
class AdsrEnv {
public:
    enum Stage { Idle, Attack, Decay, Sustain, Release };

    void setSampleRate(double sr) {
        sr_ = sr;
        set(a_, d_, s_, r_);
    }

    void set(double a, double d, double s, double r) {
        a_ = a; d_ = d; s_ = s; r_ = r;
        sustainLevel_ = s;
        attackRate_  = a * sr_;
        decayRate_   = d * sr_;
        releaseRate_ = r * sr_;
        attackCoef_  = calcCoef(attackRate_,  targetRatioA_);
        decayCoef_   = calcCoef(decayRate_,   targetRatioDR_);
        releaseCoef_ = calcCoef(releaseRate_, targetRatioDR_);
        attackBase_  = (1.0 + targetRatioA_)  * (1.0 - attackCoef_);
        decayBase_   = (sustainLevel_ - targetRatioDR_) * (1.0 - decayCoef_);
        releaseBase_ = -targetRatioDR_        * (1.0 - releaseCoef_);
    }

    void noteOn()  { stage_ = Attack; }
    void noteOff() { if (stage_ != Idle) stage_ = Release; }
    bool isActive() const { return stage_ != Idle; }

    inline double process() {
        switch (stage_) {
            case Idle: return 0.0;
            case Attack:
                value_ = attackBase_ + value_ * attackCoef_;
                if (value_ >= 1.0) { value_ = 1.0; stage_ = Decay; }
                break;
            case Decay:
                value_ = decayBase_ + value_ * decayCoef_;
                if (value_ <= sustainLevel_) { value_ = sustainLevel_; stage_ = Sustain; }
                break;
            case Sustain:
                value_ = sustainLevel_;
                break;
            case Release:
                value_ = releaseBase_ + value_ * releaseCoef_;
                if (value_ <= 0.0) { value_ = 0.0; stage_ = Idle; }
                break;
        }
        return value_;
    }

    double value() const { return value_; }

private:
    static double calcCoef(double rateSamples, double targetRatio) {
        if (rateSamples <= 0.0) return 0.0;
        return std::exp(-std::log((1.0 + targetRatio) / targetRatio) / rateSamples);
    }

    double sr_ = 48000.0;
    double a_ = 0.001, d_ = 0.1, s_ = 1.0, r_ = 0.01;
    double value_ = 0.0;
    Stage stage_ = Idle;

    double targetRatioA_  = 0.3;
    double targetRatioDR_ = 0.0001;

    double sustainLevel_ = 1.0;
    double attackRate_=0,  attackCoef_=0,  attackBase_=0;
    double decayRate_=0,   decayCoef_=0,   decayBase_=0;
    double releaseRate_=0, releaseCoef_=0, releaseBase_=0;
};

} // namespace

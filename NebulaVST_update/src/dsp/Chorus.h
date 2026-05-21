#pragma once
#include <vector>
#include <cmath>

namespace Nebula {

#ifndef NEBULA_PI
#define NEBULA_PI 3.14159265358979323846
#endif

// Истинно-стерео-чорус. Каждый голос имеет свою задержку
// и панорамируется. L и R буферы независимы — стерео сохраняется.
class Chorus {
public:
    enum Mode { Classic=0, Lush=1, Ensemble=2 };

    void setSampleRate(double sr){
        sr_ = sr;
        bufL_.assign((size_t)(sr * 0.05), 0.0f);
        bufR_.assign((size_t)(sr * 0.05), 0.0f);
        wL_ = wR_ = 0;
        phase_ = 0.0;
    }
    void setMix(double m)   { mix_   = m; }
    void setRate(double hz) { rate_  = hz; }
    void setDepth(double d) { depth_ = d; }
    void setMode(int m)     { mode_  = (Mode)m; }
    void reset(){
        std::fill(bufL_.begin(),bufL_.end(),0.0f);
        std::fill(bufR_.begin(),bufR_.end(),0.0f);
    }

    inline void process(float inL, float inR, float& outL, float& outR){
        phase_ += rate_ / sr_;
        if (phase_ >= 1.0) phase_ -= 1.0;

        int voices = (mode_ == Classic) ? 2 : (mode_ == Lush) ? 4 : 6;
        double baseMs = 11.0;
        double depthMs = 4.5 + 5.0 * depth_;

        bufL_[wL_] = inL; wL_ = (wL_ + 1) % bufL_.size();
        bufR_[wR_] = inR; wR_ = (wR_ + 1) % bufR_.size();

        double accL = 0.0, accR = 0.0;
        for (int i = 0; i < voices; ++i) {
            double phOff = (double)i / voices;
            double lfo = std::sin(2.0 * NEBULA_PI * (phase_ + phOff));
            double samples = (baseMs + depthMs * lfo) * 0.001 * sr_;

            // ВАЖНО: половина голосов читает левый буфер, половина правый.
            // Это сохраняет стерео-картинку, в отличие от прошлой реализации.
            float v = (i % 2 == 0)
                      ? readInterp(bufL_, (double)wL_ - samples - 1)
                      : readInterp(bufR_, (double)wR_ - samples - 1);

            double pan = (voices == 1) ? 0.0 : ((double)i / (voices - 1)) * 2.0 - 1.0;
            double lGain = 0.5 * (1.0 - pan);
            double rGain = 0.5 * (1.0 + pan);
            accL += v * lGain;
            accR += v * rGain;
        }
        double normK = 2.0 / voices;
        accL *= normK; accR *= normK;

        outL = (float)(inL * (1.0 - mix_) + accL * mix_);
        outR = (float)(inR * (1.0 - mix_) + accR * mix_);
    }
private:
    static float readInterp(const std::vector<float>& b, double pos){
        while (pos < 0) pos += b.size();
        while (pos >= b.size()) pos -= b.size();
        size_t i0 = (size_t)pos;
        size_t i1 = (i0 + 1) % b.size();
        double f = pos - i0;
        return (float)(b[i0] * (1.0 - f) + b[i1] * f);
    }
    std::vector<float> bufL_, bufR_;
    size_t wL_=0, wR_=0;
    double sr_=48000.0, phase_=0.0;
    double rate_ = 0.7, depth_ = 0.5, mix_=0.5;
    Mode mode_ = Classic;
};

} // namespace

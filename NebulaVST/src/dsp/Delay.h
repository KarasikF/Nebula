#pragma once
#include <vector>
#include <cmath>

namespace Nebula {

// Стерео-delay с режимами Stereo / Ping-Pong.
class Delay {
public:
    enum Mode { Stereo=0, PingPong=1 };

    void setSampleRate(double sr){
        sr_ = sr;
        size_t n = (size_t)(sr * 2.5);
        bufL_.assign(n, 0.0f);
        bufR_.assign(n, 0.0f);
        writeIdx_ = 0;
        lpL_ = lpR_ = 0.0;
    }
    void set(double timeSec, double feedback, double mix){
        targetDelay_ = timeSec * sr_;
        fb_ = feedback;
        mix_ = mix;
    }
    void setMode(int m){ mode_ = (Mode)m; }
    void reset(){
        std::fill(bufL_.begin(), bufL_.end(), 0.0f);
        std::fill(bufR_.begin(), bufR_.end(), 0.0f);
        lpL_ = lpR_ = 0.0;
    }

    inline void process(float inL, float inR, float& outL, float& outR){
        delaySamples_ += (targetDelay_ - delaySamples_) * 0.0005;

        double readPos = (double)writeIdx_ - delaySamples_;
        float echoL = readBuf(bufL_, readPos);
        float echoR = readBuf(bufR_, readPos);

        const double a = 0.35;
        lpL_ = lpL_ + a * (echoL - lpL_);
        lpR_ = lpR_ + a * (echoR - lpR_);

        if (mode_ == PingPong) {
            bufL_[writeIdx_] = (float)(inL + lpR_ * fb_);
            bufR_[writeIdx_] = (float)(inR + lpL_ * fb_);
        } else {
            bufL_[writeIdx_] = (float)(inL + lpL_ * fb_);
            bufR_[writeIdx_] = (float)(inR + lpR_ * fb_);
        }
        writeIdx_ = (writeIdx_ + 1) % bufL_.size();

        outL = (float)(inL * (1.0 - mix_) + echoL * mix_);
        outR = (float)(inR * (1.0 - mix_) + echoR * mix_);
    }
private:
    inline float readBuf(const std::vector<float>& b, double pos){
        while (pos < 0) pos += b.size();
        while (pos >= b.size()) pos -= b.size();
        size_t i0 = (size_t)pos;
        size_t i1 = (i0 + 1) % b.size();
        double f = pos - i0;
        return (float)(b[i0] * (1.0 - f) + b[i1] * f);
    }
    std::vector<float> bufL_, bufR_;
    size_t writeIdx_ = 0;
    double sr_ = 48000.0;
    double delaySamples_ = 4800.0, targetDelay_ = 4800.0;
    double fb_ = 0.4, mix_ = 0.0;
    double lpL_ = 0.0, lpR_ = 0.0;
    Mode mode_ = Stereo;
};

} // namespace

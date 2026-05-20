#pragma once
#include <vector>
#include <cmath>

namespace Nebula {

// Simple Schroeder-Moorer reverb: 4 comb + 2 allpass × stereo, с lp в петле.
// Не самый «премиальный», но вполне приличный для синта.
class Reverb {
public:
    void setSampleRate(double sr){
        sr_ = sr;
        const int combL[4] = {1116, 1188, 1277, 1356};
        const int combR[4] = {1139, 1211, 1300, 1379};
        const int apL[2]   = {556, 441};
        const int apR[2]   = {579, 464};
        double k = sr / 44100.0;
        for(int i=0;i<4;i++){ combs_[0][i].init((int)(combL[i]*k)); combs_[1][i].init((int)(combR[i]*k)); }
        for(int i=0;i<2;i++){ aps_[0][i].init((int)(apL[i]*k)); aps_[1][i].init((int)(apR[i]*k)); }
    }
    void setMix(double m){ mix_ = m; }
    void reset(){
        for(int s=0;s<2;s++){ for(auto& c:combs_[s]) c.reset(); for(auto& a:aps_[s]) a.reset(); }
    }

    inline void process(float in, float& outL, float& outR){
        const double fb = 0.84, damp = 0.2;
        double l=0, r=0;
        for(int i=0;i<4;i++){ l += combs_[0][i].process(in, fb, damp); r += combs_[1][i].process(in, fb, damp); }
        for(int i=0;i<2;i++){ l = aps_[0][i].process((float)l); r = aps_[1][i].process((float)r); }
        outL = (float)(in * (1.0 - mix_) + l * mix_ * 0.35);
        outR = (float)(in * (1.0 - mix_) + r * mix_ * 0.35);
    }
private:
    struct Comb {
        std::vector<float> buf; size_t idx=0; double filt=0.0;
        void init(int n){ buf.assign(n, 0.0f); idx=0; filt=0.0; }
        void reset(){ std::fill(buf.begin(),buf.end(),0.0f); filt=0.0; }
        float process(float in, double fb, double damp){
            float out = buf[idx];
            filt = out * (1.0 - damp) + filt * damp;
            buf[idx] = in + (float)(filt * fb);
            idx = (idx + 1) % buf.size();
            return out;
        }
    };
    struct AP {
        std::vector<float> buf; size_t idx=0;
        void init(int n){ buf.assign(n, 0.0f); idx=0; }
        void reset(){ std::fill(buf.begin(),buf.end(),0.0f); }
        float process(float in){
            float bufout = buf[idx];
            float out = -in + bufout;
            buf[idx] = in + bufout * 0.5f;
            idx = (idx + 1) % buf.size();
            return out;
        }
    };
    Comb combs_[2][4];
    AP   aps_[2][2];
    double sr_=48000.0, mix_=0.25;
};

} // namespace

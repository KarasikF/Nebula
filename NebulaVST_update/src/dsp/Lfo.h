#pragma once
#include <cmath>
#include <cstdint>

namespace Nebula {

#ifndef NEBULA_PI
#define NEBULA_PI 3.14159265358979323846
#endif

// LFO с поддержкой Sample & Hold (S&H).
class Lfo {
public:
    enum Wave { Sine=0, Triangle=1, Square=2, Saw=3, SH=4 };

    void setSampleRate(double sr){ sr_ = sr; }
    void setRate(double hz){ inc_ = hz / sr_; }
    void setWave(Wave w){ wave_ = w; }
    void reset(){ phase_ = 0.0; lastPhase_ = 0.0; shVal_ = 0.0; }

    inline double process(){
        double t = phase_;
        double y = 0.0;
        switch(wave_){
            case Sine:     y = std::sin(2.0 * NEBULA_PI * t); break;
            case Triangle: y = 4.0 * std::fabs(t - 0.5) - 1.0; break;
            case Square:   y = (t < 0.5) ? 1.0 : -1.0; break;
            case Saw:      y = 2.0 * t - 1.0; break;
            case SH: {
                // на каждом переходе через 0 — новое случайное значение
                if (lastPhase_ > t) {
                    rngState_ = rngState_ * 1664525u + 1013904223u;
                    shVal_ = (int32_t)rngState_ * (1.0 / 2147483648.0);
                }
                y = shVal_;
                break;
            }
        }
        lastPhase_ = t;
        phase_ += inc_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return y;
    }
private:
    double sr_=48000.0, inc_=0.0001, phase_=0.0, lastPhase_=0.0;
    double shVal_ = 0.0;
    uint32_t rngState_ = 0x9E3779B9u;
    Wave wave_ = Sine;
};

} // namespace

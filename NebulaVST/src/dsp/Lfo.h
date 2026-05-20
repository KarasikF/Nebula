#pragma once
#include <cmath>

namespace Nebula {

#ifndef NEBULA_PI
#define NEBULA_PI 3.14159265358979323846
#endif

class Lfo {
public:
    enum Wave { Sine=0, Triangle=1, Square=2, Saw=3 };
    void setSampleRate(double sr){ sr_ = sr; }
    void setRate(double hz){ inc_ = hz / sr_; }
    void setWave(Wave w){ wave_ = w; }
    void reset(){ phase_ = 0.0; }

    inline double process(){
        double t = phase_;
        double y = 0.0;
        switch(wave_){
            case Sine:     y = std::sin(2.0 * NEBULA_PI * t); break;
            case Triangle: y = 4.0 * std::fabs(t - 0.5) - 1.0; break;
            case Square:   y = (t < 0.5) ? 1.0 : -1.0; break;
            case Saw:      y = 2.0 * t - 1.0; break;
        }
        phase_ += inc_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return y;
    }
private:
    double sr_=48000.0, inc_=0.0001, phase_=0.0;
    Wave wave_ = Sine;
};

} // namespace

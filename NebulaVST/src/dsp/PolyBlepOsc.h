#pragma once
#include <cmath>

namespace Nebula {

#ifndef NEBULA_PI
#define NEBULA_PI 3.14159265358979323846
#endif

// PolyBLEP oscillator: алиасинг подавляется добавлением полиномиальной
// аппроксимации band-limited step / band-limited residual.
class PolyBlepOsc {
public:
    enum Wave { Sine=0, Triangle=1, Saw=2, Square=3 };

    void setSampleRate(double sr) { sr_ = sr; inv_sr_ = 1.0/sr; }
    void setFrequency(double hz)  { freq_ = hz; inc_ = hz * inv_sr_; }
    void setWave(Wave w) { wave_ = w; }
    void setPulseWidth(double pw) { pw_ = pw < 0.01 ? 0.01 : (pw > 0.99 ? 0.99 : pw); }
    void reset(double phase=0.0){ phase_ = phase; tri_state_ = 0.0; }

    inline double process() {
        double t = phase_;
        double dt = inc_;
        double out = 0.0;
        switch (wave_) {
            case Sine:
                out = std::sin(2.0 * NEBULA_PI * t);
                break;
            case Triangle: {
                double sq = (t < pw_) ? 1.0 : -1.0;
                sq += polyBlep(t, dt);
                sq -= polyBlep(std::fmod(t + (1.0 - pw_), 1.0), dt);
                tri_state_ = dt * 4.0 * sq + (1.0 - dt) * tri_state_;
                out = tri_state_;
                break;
            }
            case Saw:
                out = 2.0 * t - 1.0;
                out -= polyBlep(t, dt);
                break;
            case Square:
                out = (t < pw_) ? 1.0 : -1.0;
                out += polyBlep(t, dt);
                out -= polyBlep(std::fmod(t + (1.0 - pw_), 1.0), dt);
                break;
        }
        phase_ += dt;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return out;
    }

private:
    static inline double polyBlep(double t, double dt) {
        if (t < dt) { t /= dt; return t + t - t*t - 1.0; }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t*t + t + t + 1.0; }
        return 0.0;
    }
    double sr_ = 48000.0, inv_sr_ = 1.0/48000.0;
    double freq_ = 440.0, inc_ = 440.0/48000.0;
    double phase_ = 0.0, pw_ = 0.5, tri_state_ = 0.0;
    Wave wave_ = Saw;
};

} // namespace

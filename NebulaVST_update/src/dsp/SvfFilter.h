#pragma once
#include <cmath>

namespace Nebula {

#ifndef NEBULA_PI
#define NEBULA_PI 3.14159265358979323846
#endif

// State-Variable Filter с лёгкой нелинейностью в обратной связи —
// даёт самовозбуждение при высоком Q (~35+) и более «аналоговый» резонанс.
class SvfFilter {
public:
    enum Type { LP=0, HP=1, BP=2, Notch=3 };

    void setSampleRate(double sr) { sr_ = sr; }
    void setType(Type t) { type_ = t; }
    void reset() { ic1eq_ = ic2eq_ = 0.0; }

    void setParams(double cutoff, double q) {
        if (cutoff < 20.0) cutoff = 20.0;
        if (cutoff > sr_ * 0.45) cutoff = sr_ * 0.45;
        if (q < 0.5) q = 0.5;
        g_ = std::tan(NEBULA_PI * cutoff / sr_);
        k_ = 1.0 / q;
        a1_ = 1.0 / (1.0 + g_ * (g_ + k_));
        a2_ = g_ * a1_;
        a3_ = g_ * a2_;
        // компенсация громкости при высоком Q: чем выше Q, тем меньше gain
        // (иначе самовозбуждение «съедает» хост)
        gainComp_ = 1.0 / (1.0 + 0.05 * q);
    }

    inline double process(double v0) {
        // лёгкая нелинейность на входе резонансной петли
        double v3 = v0 - ic2eq_;
        double v1 = a1_ * ic1eq_ + a2_ * v3;
        double v2 = ic2eq_ + a2_ * ic1eq_ + a3_ * v3;
        // tanh-clip на интеграторе — типичный приём для аналогового SVF
        v1 = std::tanh(v1);
        ic1eq_ = 2.0 * v1 - ic1eq_;
        ic2eq_ = 2.0 * v2 - ic2eq_;
        double y = 0;
        switch (type_) {
            case LP:    y = v2; break;
            case HP:    y = v0 - k_*v1 - v2; break;
            case BP:    y = v1; break;
            case Notch: y = v0 - k_*v1; break;
        }
        return y * gainComp_;
    }

private:
    double sr_ = 48000.0;
    double g_=0.1, k_=1.0, a1_=1.0, a2_=0.0, a3_=0.0;
    double ic1eq_=0.0, ic2eq_=0.0;
    double gainComp_ = 1.0;
    Type type_ = LP;
};

} // namespace

#pragma once
#include <cmath>

namespace Nebula {

#ifndef NEBULA_PI
#define NEBULA_PI 3.14159265358979323846
#endif

// Topology-preserving State Variable Filter (Andrew Simper / Vadim Zavalishin).
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
    }

    inline double process(double v0) {
        double v3 = v0 - ic2eq_;
        double v1 = a1_ * ic1eq_ + a2_ * v3;
        double v2 = ic2eq_ + a2_ * ic1eq_ + a3_ * v3;
        ic1eq_ = 2.0 * v1 - ic1eq_;
        ic2eq_ = 2.0 * v2 - ic2eq_;
        switch (type_) {
            case LP:    return v2;
            case HP:    return v0 - k_*v1 - v2;
            case BP:    return v1;
            case Notch: return v0 - k_*v1;
        }
        return v2;
    }

private:
    double sr_ = 48000.0;
    double g_=0.1, k_=1.0, a1_=1.0, a2_=0.0, a3_=0.0;
    double ic1eq_=0.0, ic2eq_=0.0;
    Type type_ = LP;
};

} // namespace

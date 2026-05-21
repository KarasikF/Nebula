#pragma once
#include <cmath>

namespace Nebula {

// 3 type of analog-style saturation, control 0..1.
class Drive {
public:
    enum Type { Tube=0, Fuzz=1, Fold=2 };

    void setType(int t){ type_ = (Type)t; }
    // amount 0..1 -> внутренний gain 1..50
    void setAmount(double a){
        amount_ = a;
        gain_ = 1.0 + a * 49.0;
        invComp_ = 1.0 / (1.0 + a * 4.0); // компенсация громкости
    }

    inline double process(double x){
        if (amount_ < 1e-4) return x;
        double y;
        switch (type_) {
            case Tube: {
                // soft tube — асимметричный tanh с лёгкой 2-й гармоникой
                double g = gain_;
                y = std::tanh(g * x + 0.1 * g * x * x) / std::tanh(g);
                break;
            }
            case Fuzz: {
                // hard transistor fuzz — почти square-wave при больших значениях
                double g = gain_ * 1.5;
                double s = g * x;
                y = s / (1.0 + std::fabs(s));
                y = std::tanh(y * 2.5);
                break;
            }
            case Fold: {
                // wavefolder: сложить сигнал «зеркалами»
                double g = 1.0 + amount_ * 6.0;
                double s = x * g;
                // triangle-wave wrap
                double w = std::sin(s * 1.5707963) * 0.9;
                y = w;
                break;
            }
            default: y = x;
        }
        return y * invComp_;
    }

private:
    Type type_ = Tube;
    double amount_ = 0.0;
    double gain_ = 1.0;
    double invComp_ = 1.0;
};

} // namespace

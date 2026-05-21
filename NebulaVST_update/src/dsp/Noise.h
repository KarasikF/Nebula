#pragma once
#include <cstdint>

namespace Nebula {

// Three colors of noise: White, Pink (Paul Kellet refined), Brown (integrated).
class ColoredNoise {
public:
    enum Color { White=0, Pink=1, Brown=2 };

    void setColor(int c){ color_ = (Color)c; }
    void reset(){
        b0_=b1_=b2_=b3_=b4_=b5_=b6_=0.0;
        brownLast_=0.0;
    }

    inline float process(){
        rngState_ = rngState_ * 1664525u + 1013904223u;
        float white = (int32_t)rngState_ * (1.0f / 2147483648.0f);

        switch (color_) {
            case White: return white;
            case Pink: {
                // Paul Kellet's "refined" pink noise filter
                b0_ = 0.99886 * b0_ + white * 0.0555179;
                b1_ = 0.99332 * b1_ + white * 0.0750759;
                b2_ = 0.96900 * b2_ + white * 0.1538520;
                b3_ = 0.86650 * b3_ + white * 0.3104856;
                b4_ = 0.55000 * b4_ + white * 0.5329522;
                b5_ = -0.7616 * b5_ - white * 0.0168980;
                double pink = b0_ + b1_ + b2_ + b3_ + b4_ + b5_ + b6_ + white * 0.5362;
                b6_ = white * 0.115926;
                return (float)(pink * 0.11); // компенсация громкости
            }
            case Brown: {
                brownLast_ = (brownLast_ + 0.02 * white) * 0.998;
                return (float)(brownLast_ * 3.5);
            }
        }
        return white;
    }

private:
    Color color_ = White;
    uint32_t rngState_ = 0x5F375A86u;
    double b0_=0,b1_=0,b2_=0,b3_=0,b4_=0,b5_=0,b6_=0;
    double brownLast_ = 0;
};

} // namespace

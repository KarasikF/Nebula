#pragma once
#include <cstdint>
#include <cmath>

namespace Nebula {

// Кривая распределения для крутилок:
//   Lin    — линейно
//   Log    — log10 (плохо для коротких времён 1ms..6s — края сжаты)
//   Pow3   — power(x, 3): 50% ручки = середина диапазона по СЛЫШИМОСТИ
//             (даёт хорошее распределение 1ms..10ms..100ms..1s..6s)
enum class Curve : uint8_t { Lin, Log, Pow3 };

enum ParamId : uint32_t {
    // OSC1
    kOsc1Wave = 0, kOsc1Octave, kOsc1Semi, kOsc1Fine, kOsc1Pw, kOsc1On,
    // OSC2
    kOsc2Wave, kOsc2Octave, kOsc2Semi, kOsc2Fine, kOsc2Pw, kOsc2On,
    // MIX
    kMixOsc1, kMixOsc2, kMixSub, kMixNoise, kNoiseColor,
    // GLOBAL
    kGlide, kGlideMode, kSpread, kAnalogDrift,
    // FILTER
    kFilterOn, kFilterType, kFilterCutoff, kFilterQ, kFilterEnvAmt, kFilterKbd,
    kDriveType, kDriveAmount,
    // FILTER ENV
    kFEnvA, kFEnvD, kFEnvS, kFEnvR,
    // AMP ENV
    kAEnvA, kAEnvD, kAEnvS, kAEnvR,
    // LFO
    kLfoWave, kLfoDest, kLfoRate, kLfoAmount,
    // FX — chorus
    kFxChorusOn, kFxChorusMode, kFxChorus, kFxChorusRate, kFxChorusDepth,
    // FX — delay
    kFxDelayOn, kFxDelayMode, kFxDelay, kFxDelayTime, kFxDelayFb,
    // FX — reverb
    kFxReverbOn, kFxReverb, kFxReverbSize, kFxReverbDamp,
    // Master
    kMaster,
    kNumParams
};

struct ParamInfo {
    const char* id;
    const char* title;
    const char* units;
    double min, max, def;
    Curve curve;
    int steps;   // 0=continuous, >=2=discrete
};

inline const ParamInfo& paramInfo(uint32_t id) {
    static const ParamInfo P[kNumParams] = {
        // ---- OSC1 ----
        {"osc1.wave",  "OSC1 Wave","",   0,3,    2,    Curve::Lin, 4},
        {"osc1.octave","OSC1 Oct","oct",-2,2,    0,    Curve::Lin, 5},
        {"osc1.semi",  "OSC1 Semi","st",-12,12,  0,    Curve::Lin, 25},
        {"osc1.fine",  "OSC1 Fine","ct",-50,50,  0,    Curve::Lin, 0},
        {"osc1.pw",    "OSC1 PW","",    0,1,     0.5,  Curve::Lin, 0},
        {"osc1.on",    "OSC1 On","",    0,1,     1,    Curve::Lin, 2},
        // ---- OSC2 ----
        {"osc2.wave",  "OSC2 Wave","",   0,3,    2,    Curve::Lin, 4},
        {"osc2.octave","OSC2 Oct","oct",-2,2,    0,    Curve::Lin, 5},
        {"osc2.semi",  "OSC2 Semi","st",-12,12,  0,    Curve::Lin, 25},
        {"osc2.fine",  "OSC2 Fine","ct",-50,50,  0,    Curve::Lin, 0},
        {"osc2.pw",    "OSC2 PW","",    0,1,     0.5,  Curve::Lin, 0},
        {"osc2.on",    "OSC2 On","",    0,1,     0,    Curve::Lin, 2},  // выкл by default
        // ---- MIX ----
        {"mix.osc1", "Mix OSC1","",  0,1,    0.7,  Curve::Lin, 0},
        {"mix.osc2", "Mix OSC2","",  0,1,    0.5,  Curve::Lin, 0},
        {"mix.sub",  "Mix Sub","",   0,1,    0.0,  Curve::Lin, 0},
        {"mix.noise","Mix Noise","", 0,1,    0.0,  Curve::Lin, 0},
        {"mix.noiseColor","Noise Color","",0,2, 0, Curve::Lin, 3}, // White/Pink/Brown
        // ---- GLOBAL ----
        {"glide",    "Glide","s",     0, 2,    0.0, Curve::Pow3, 0},
        {"glideMode","Glide Mode","", 0, 1,    0,   Curve::Lin,  2}, // Always/Legato
        {"spread",   "Spread","",     0, 1,    0.3, Curve::Lin,  0}, // stereo pan random
        {"drift",    "Analog","",     0, 1,    0.25,Curve::Lin,  0}, // лёгкий drift
        // ---- FILTER ----
        {"filter.on",   "Filter On","",  0,1, 1, Curve::Lin, 2},
        {"filter.type", "Filter Type","",0,3, 0, Curve::Lin, 4},
        {"filter.cutoff","Cutoff","Hz",  20,20000, 20000, Curve::Log, 0},
        {"filter.q",    "Reso","",       0.5,50,   0.7,   Curve::Pow3,0},  // до 50!
        {"filter.envAmt","Filter Env","",-1,1,     0.0,   Curve::Lin, 0},
        {"filter.kbd",  "Kbd Track","",  0,1,      0.0,   Curve::Lin, 0},
        // ---- DRIVE ----
        {"driveType",   "Drive Type","", 0,2,      0,     Curve::Lin, 3},  // Tube/Fuzz/Fold
        {"driveAmount", "Drive","",      0,1,      0.0,   Curve::Pow3,0},
        // ---- FILTER ENV ----
        {"fenv.a","FEnv A","s",0.001,8, 0.001, Curve::Pow3, 0},
        {"fenv.d","FEnv D","s",0.001,8, 0.100, Curve::Pow3, 0},
        {"fenv.s","FEnv S","", 0,1,     1.0,   Curve::Lin,  0},
        {"fenv.r","FEnv R","s",0.001,10,0.100, Curve::Pow3, 0},
        // ---- AMP ENV ----
        {"aenv.a","AEnv A","s",0.001,8, 0.001, Curve::Pow3, 0},
        {"aenv.d","AEnv D","s",0.001,8, 0.100, Curve::Pow3, 0},
        {"aenv.s","AEnv S","", 0,1,     1.0,   Curve::Lin,  0},
        {"aenv.r","AEnv R","s",0.001,10,0.010, Curve::Pow3, 0},
        // ---- LFO ----
        {"lfo.wave",  "LFO Wave","", 0,4,    0, Curve::Lin, 5}, // +S&H
        {"lfo.dest",  "LFO Dest","", 0,3,    1, Curve::Lin, 4},
        {"lfo.rate",  "LFO Rate","Hz",0.05,30, 4.0, Curve::Log, 0},
        {"lfo.amount","LFO Amt","",  0,1,    0.0, Curve::Lin, 0},
        // ---- Chorus ----
        {"fx.chorusOn",   "Chorus On","",  0,1, 0, Curve::Lin, 2},
        {"fx.chorusMode", "Chorus Mode","",0,2, 0, Curve::Lin, 3},
        {"fx.chorus",     "Chorus Mix","", 0,1, 0.5, Curve::Lin, 0},
        {"fx.chorusRate", "Chorus Rate","Hz",0.05,8, 0.7, Curve::Log, 0},
        {"fx.chorusDepth","Chorus Depth","",0,1, 0.5, Curve::Lin, 0},
        // ---- Delay ----
        {"fx.delayOn",  "Delay On","",  0,1, 0, Curve::Lin, 2},
        {"fx.delayMode","Delay Mode","",0,1, 0, Curve::Lin, 2},
        {"fx.delay",    "Delay Mix","", 0,1, 0.3, Curve::Lin, 0},
        {"fx.delayTime","Delay Time","s",0.02,2, 0.32, Curve::Pow3, 0},
        {"fx.delayFb",  "Delay FB","",  0,0.92, 0.4, Curve::Lin, 0},
        // ---- Reverb ----
        {"fx.reverbOn",   "Reverb On","",  0,1, 0, Curve::Lin, 2},
        {"fx.reverb",     "Reverb Mix","", 0,1, 0.3, Curve::Lin, 0},
        {"fx.reverbSize", "Reverb Size","",0,1, 0.6, Curve::Lin, 0},
        {"fx.reverbDamp", "Reverb Damp","",0,1, 0.4, Curve::Lin, 0},
        // ---- Master ----
        {"master","Master","",0,2, 1.0, Curve::Lin, 0},
    };
    return P[id];
}

inline double denorm(const ParamInfo& p, double n) {
    if (p.steps > 0) {
        int steps = p.steps;
        int v = (int)::floor(n * steps);
        if (v >= steps) v = steps - 1;
        if (steps == 1) return p.min;
        return p.min + v * ((p.max - p.min) / (steps - 1));
    }
    switch (p.curve) {
        case Curve::Lin:  return p.min + (p.max - p.min) * n;
        case Curve::Log: {
            double lmin = ::log(p.min), lmax = ::log(p.max);
            return ::exp(lmin + (lmax - lmin) * n);
        }
        case Curve::Pow3: {
            double y = n * n * n;
            return p.min + (p.max - p.min) * y;
        }
    }
    return p.min;
}

inline double norm(const ParamInfo& p, double v) {
    if (p.steps > 0) {
        int steps = p.steps;
        if (steps == 1) return 0.5;
        double step = (p.max - p.min) / (steps - 1);
        int idx = (int)::floor((v - p.min) / step + 0.5);
        if (idx < 0) idx = 0;
        if (idx >= steps) idx = steps - 1;
        return (double)idx / steps + 0.5 / steps;
    }
    switch (p.curve) {
        case Curve::Lin:  return (v - p.min) / (p.max - p.min);
        case Curve::Log: {
            double lmin = ::log(p.min), lmax = ::log(p.max);
            return (::log(v) - lmin) / (lmax - lmin);
        }
        case Curve::Pow3: {
            double y = (v - p.min) / (p.max - p.min);
            if (y < 0) y = 0; if (y > 1) y = 1;
            return ::pow(y, 1.0/3.0);
        }
    }
    return 0;
}

} // namespace

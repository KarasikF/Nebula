#pragma once
#include <cstdint>
#include <cmath>

namespace Nebula {

enum ParamId : uint32_t {
    // OSC1
    kOsc1Wave = 0, kOsc1Octave, kOsc1Semi, kOsc1Fine, kOsc1Pw,
    // OSC2
    kOsc2Wave, kOsc2Octave, kOsc2Semi, kOsc2Fine, kOsc2Pw,
    // MIX
    kMixOsc1, kMixOsc2, kMixSub, kMixNoise,
    // FILTER
    kFilterType, kFilterCutoff, kFilterQ, kFilterEnvAmt, kFilterKbd, kFilterDrive,
    // FILTER ENV
    kFEnvA, kFEnvD, kFEnvS, kFEnvR,
    // AMP ENV
    kAEnvA, kAEnvD, kAEnvS, kAEnvR,
    // LFO
    kLfoWave, kLfoDest, kLfoRate, kLfoAmount,
    // FX — chorus
    kFxChorusMode, kFxChorus, kFxChorusRate, kFxChorusDepth,
    // FX — delay
    kFxDelayMode, kFxDelay, kFxDelayTime, kFxDelayFb,
    // FX — reverb
    kFxReverb,
    // Master
    kMaster,
    kNumParams
};

struct ParamInfo {
    const char* id;
    const char* title;
    const char* units;
    double min, max, def;     // def — это «init»-значение в физических единицах
    bool isLog;
    int steps;                // 0 = continuous, иначе число дискретных шагов
};

// Дефолты намеренно «нейтральные» — Init-пресет:
//   - частоты OSC: oct=0, semi=0, fine=0
//   - mix: только OSC1 70%, остальное 0
//   - filter: открыт (cutoff=20k, q=1, env=0)
//   - envelopes: молниеносная атака, мгновенный D, S=1, короткий R (как у органа)
//   - FX: всё в ноль
//   - master: 1.0 (середина диапазона 0..2)
inline const ParamInfo& paramInfo(uint32_t id) {
    static const ParamInfo P[kNumParams] = {
        // ---- OSC1 ----
        {"osc1.wave",   "OSC1 Wave",  "",    0,    3,     2,    false, 4},   // SAW
        {"osc1.octave", "OSC1 Octave","oct", -2,   2,     0,    false, 5},
        {"osc1.semi",   "OSC1 Semi",  "st",  -12,  12,    0,    false, 25},
        {"osc1.fine",   "OSC1 Fine",  "ct",  -50,  50,    0,    false, 0},
        {"osc1.pw",     "OSC1 PW",    "",    0,    1,     0.5,  false, 0},
        // ---- OSC2 ----
        {"osc2.wave",   "OSC2 Wave",  "",    0,    3,     2,    false, 4},   // SAW
        {"osc2.octave", "OSC2 Octave","oct", -2,   2,     0,    false, 5},
        {"osc2.semi",   "OSC2 Semi",  "st",  -12,  12,    0,    false, 25},  // init = 0
        {"osc2.fine",   "OSC2 Fine",  "ct",  -50,  50,    0,    false, 0},   // init = 0
        {"osc2.pw",     "OSC2 PW",    "",    0,    1,     0.5,  false, 0},
        // ---- MIX ----
        {"mix.osc1",    "Mix OSC1",   "",    0,    1,     0.7,  false, 0},
        {"mix.osc2",    "Mix OSC2",   "",    0,    1,     0.0,  false, 0},   // init: только OSC1
        {"mix.sub",     "Mix Sub",    "",    0,    1,     0.0,  false, 0},
        {"mix.noise",   "Mix Noise",  "",    0,    1,     0.0,  false, 0},
        // ---- FILTER ----
        {"filter.type",   "Filter Type","",  0,    3,     0,    false, 4},   // LP
        {"filter.cutoff", "Cutoff",     "Hz",20,   20000, 20000,true,  0},   // полностью открыт
        {"filter.q",      "Reso",       "",  0.1,  20,    1.0,  false, 0},
        {"filter.envAmt", "Filter Env Amt","",-1,  1,     0.0,  false, 0},   // 0
        {"filter.kbd",    "Kbd Track",  "",  0,    1,     0.0,  false, 0},   // 0
        {"filter.drive",  "Drive",      "",  1,    10,    1.0,  false, 0},   // без драйва
        // ---- FILTER ENV ----
        {"fenv.a","FEnv A","s",0.001,4,0.001,true,0},
        {"fenv.d","FEnv D","s",0.001,4,0.100,true,0},
        {"fenv.s","FEnv S","", 0,1,    1.0,  false,0},
        {"fenv.r","FEnv R","s",0.001,6,0.100,true,0},
        // ---- AMP ENV (init = «orgaт»: A=1ms, D=любой, S=1, R=10ms) ----
        {"aenv.a","AEnv A","s",0.001,4,0.001,true,0},
        {"aenv.d","AEnv D","s",0.001,4,0.100,true,0},
        {"aenv.s","AEnv S","", 0,1,    1.0,  false,0},
        {"aenv.r","AEnv R","s",0.001,6,0.010,true,0},
        // ---- LFO ----
        {"lfo.wave",  "LFO Wave",  "", 0,3,    0,   false,4}, // SIN
        {"lfo.dest",  "LFO Dest",  "", 0,3,    1,   false,4}, // FILT
        {"lfo.rate",  "LFO Rate",  "Hz",0.05,30,4.0,true, 0},
        {"lfo.amount","LFO Amount","", 0,1,    0.0, false,0}, // выключен
        // ---- Chorus ----
        {"fx.chorusMode", "Chorus Mode", "", 0,2,    0,   false,3}, // CLASSIC
        {"fx.chorus",     "Chorus Mix",  "", 0,1,    0.0, false,0}, // выключен
        {"fx.chorusRate", "Chorus Rate","Hz",0.05,8, 0.7, true, 0},
        {"fx.chorusDepth","Chorus Depth","",0,1,     0.5, false,0},
        // ---- Delay ----
        {"fx.delayMode","Delay Mode","", 0,1,    0,   false,2}, // STEREO
        {"fx.delay",    "Delay Mix", "", 0,1,    0.0, false,0}, // выключен
        {"fx.delayTime","Delay Time","s",0.05,1, 0.32,false,0},
        {"fx.delayFb",  "Delay FB",  "", 0,0.85, 0.4, false,0},
        // ---- Reverb ----
        {"fx.reverb","Reverb","",0,1,0.0,false,0},               // выключен
        // ---- Master (диапазон 0..2 = до +6 dB) ----
        {"master","Master","",0,2,1.0,false,0},                  // 100%
    };
    return P[id];
}

inline double denorm(const ParamInfo& p, double n) {
    if (p.steps > 0) {
        int v = (int)::floor(n * p.steps);
        if (v >= p.steps) v = p.steps - 1;
        return p.min + v * ((p.max - p.min) / (p.steps - 1));
    }
    if (p.isLog) {
        double lmin = ::log(p.min), lmax = ::log(p.max);
        return ::exp(lmin + (lmax - lmin) * n);
    }
    return p.min + (p.max - p.min) * n;
}
inline double norm(const ParamInfo& p, double v) {
    if (p.steps > 0) {
        double step = (p.max - p.min) / (p.steps - 1);
        int idx = (int)::floor((v - p.min) / step + 0.5);
        return (double)idx / p.steps + 0.5 / p.steps;
    }
    if (p.isLog) {
        double lmin = ::log(p.min), lmax = ::log(p.max);
        return (::log(v) - lmin) / (lmax - lmin);
    }
    return (v - p.min) / (p.max - p.min);
}

} // namespace

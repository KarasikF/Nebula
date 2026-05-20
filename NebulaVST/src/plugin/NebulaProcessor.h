#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "ParameterIds.h"
#include "dsp/Voice.h"
#include "dsp/Lfo.h"
#include "dsp/Chorus.h"
#include "dsp/Delay.h"
#include "dsp/Reverb.h"
#include <array>
#include <atomic>

namespace Nebula {

static const Steinberg::FUID NebulaProcessorUID(0xE3B12001, 0xF94B4E8C, 0x9C5A1B07, 0xA1C5F100);

class NebulaProcessor : public Steinberg::Vst::AudioEffect {
public:
    NebulaProcessor();

    static Steinberg::FUnknown* createInstance(void*) {
        return (Steinberg::Vst::IAudioProcessor*) new NebulaProcessor();
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* ctx) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setBusArrangements(
        Steinberg::Vst::SpeakerArrangement* in, Steinberg::int32 ni,
        Steinberg::Vst::SpeakerArrangement* out, Steinberg::int32 no) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 sym) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

private:
    void handleParamChange(uint32_t id, double norm);
    void handleNoteOn(int midi, int vel);
    void handleNoteOff(int midi);
    void rebuildVoiceParams();
    int  allocateVoice(int midi);

    static constexpr int kVoices = 16;
    std::array<Voice, kVoices> voices_;
    Lfo lfo_;
    Chorus chorus_;
    Delay delay_;
    Reverb reverb_;

    std::atomic<double> params_[kNumParams];

    VoiceParams cachedVoiceParams_;
    int lfoDest_ = 1;
    double lfoAmount_ = 0.0;
    double masterGain_ = 1.0;

    double sampleRate_ = 48000.0;
};

} // namespace

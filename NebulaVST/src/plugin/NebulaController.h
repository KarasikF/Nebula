#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "ParameterIds.h"

namespace Nebula {

class NebulaController : public Steinberg::Vst::EditController {
public:
    static Steinberg::FUnknown* createInstance(void*) {
        return (Steinberg::Vst::IEditController*) new NebulaController();
    }
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* ctx) SMTG_OVERRIDE;
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;
};

} // namespace

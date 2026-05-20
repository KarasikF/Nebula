#include "NebulaController.h"
#include "NebulaEditor.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Nebula {

tresult PLUGIN_API NebulaController::initialize(FUnknown* ctx) {
    tresult r = EditController::initialize(ctx);
    if (r != kResultOk) return r;

    for (uint32_t i = 0; i < kNumParams; ++i) {
        const auto& info = paramInfo(i);
        Steinberg::Vst::Parameter* p = new Steinberg::Vst::RangeParameter(
            USTRING(info.title), i,
            USTRING(info.units),
            info.min, info.max, info.def,
            info.steps,
            Steinberg::Vst::ParameterInfo::kCanAutomate);
        parameters.addParameter(p);
    }
    return kResultOk;
}

IPlugView* PLUGIN_API NebulaController::createView(FIDString name) {
    if (Steinberg::FIDStringsEqual(name, ViewType::kEditor))
        return new NebulaEditor(this);
    return nullptr;
}

tresult PLUGIN_API NebulaController::setComponentState(IBStream* state) {
    if (!state) return kResultFalse;
    IBStreamer s(state, kLittleEndian);
    for (uint32_t i = 0; i < kNumParams; ++i) {
        double v = 0.0;
        if (!s.readDouble(v)) break;
        setParamNormalized(i, v);
    }
    return kResultOk;
}

} // namespace

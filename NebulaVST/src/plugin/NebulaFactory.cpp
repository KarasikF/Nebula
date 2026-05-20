#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/vsttypes.h"

#include "NebulaProcessor.h"
#include "NebulaController.h"

#define NEBULA_VERSION "1.0.0"

using namespace Steinberg;
using namespace Steinberg::Vst;

bool InitModule()   { return true; }
bool DeinitModule() { return true; }

BEGIN_FACTORY_DEF("Nebula Audio",
                  "https://example.com",
                  "mailto:info@example.com")

    DEF_CLASS2(INLINE_UID_FROM_FUID(Nebula::NebulaProcessorUID),
        PClassInfo::kManyInstances,
        kVstAudioEffectClass,
        "Nebula",
        Vst::kDistributable,
        Vst::PlugType::kInstrumentSynth,
        NEBULA_VERSION,
        kVstVersionString,
        Nebula::NebulaProcessor::createInstance)

    DEF_CLASS2(INLINE_UID(0xE3B12002, 0xF94B4E8C, 0x9C5A1B07, 0xA1C5F100),
        PClassInfo::kManyInstances,
        kVstComponentControllerClass,
        "NebulaController",
        0,
        "",
        NEBULA_VERSION,
        kVstVersionString,
        Nebula::NebulaController::createInstance)

END_FACTORY

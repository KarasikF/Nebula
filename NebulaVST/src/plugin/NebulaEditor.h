#pragma once

#include "public.sdk/source/common/pluginview.h"
#include "pluginterfaces/gui/iplugview.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>
#include <wrl/client.h>
#include "WebView2.h"
#include <string>

namespace Nebula {

class NebulaController;

class NebulaEditor : public Steinberg::CPluginView {
public:
    NebulaEditor(NebulaController* c);
    ~NebulaEditor();

    Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API removed() SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* newSize) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect* size) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API canResize() SMTG_OVERRIDE { return Steinberg::kResultTrue; }
    Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect* rect) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API onFocus(Steinberg::TBool state) SMTG_OVERRIDE;

private:
    void initWebView(HWND parent);
    void onWebMessage(const std::wstring& msg);
    void pushAllParamsToUI();
    void showHostContextMenu(uint32_t paramId, int xClient, int yClient);

    NebulaController* controller_;
    HWND parentHwnd_ = nullptr;

    Microsoft::WRL::ComPtr<ICoreWebView2Environment> env_;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller>  ctrl_;
    Microsoft::WRL::ComPtr<ICoreWebView2>            web_;
    EventRegistrationToken msgToken_{};

    int curW_ = 1600;
    int curH_ = 720;
};

} // namespace

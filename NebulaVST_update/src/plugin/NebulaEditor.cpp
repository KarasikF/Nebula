#include "NebulaEditor.h"
#include "NebulaController.h"
#include "EmbeddedHtml.h"
#include "NebulaProcessor.h"

#include "pluginterfaces/vst/ivstcontextmenu.h"

#include <string>
#include <sstream>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace Microsoft::WRL;

namespace Nebula {

static constexpr int kDefaultW = 1600;
static constexpr int kDefaultH = 760;
static constexpr int kMinW     = 1280;
static constexpr int kMinH     = 700;

static const wchar_t* kContainerClass = L"NebulaContainer";

NebulaEditor::NebulaEditor(NebulaController* c) : controller_(c) {
    ViewRect r(0, 0, kDefaultW, kDefaultH);
    setRect(r);
    curW_ = kDefaultW;
    curH_ = kDefaultH;
}
NebulaEditor::~NebulaEditor() {
    if (controller_) controller_->setEditor(nullptr);
}

tresult PLUGIN_API NebulaEditor::isPlatformTypeSupported(FIDString type) {
    return FIDStringsEqual(type, kPlatformTypeHWND) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API NebulaEditor::getSize(ViewRect* size) {
    *size = ViewRect(0, 0, curW_, curH_);
    return kResultOk;
}

tresult PLUGIN_API NebulaEditor::checkSizeConstraint(ViewRect* r) {
    if (!r) return kResultFalse;
    int w = r->getWidth();
    int h = r->getHeight();
    if (w < kMinW) w = kMinW;
    if (h < kMinH) h = kMinH;
    r->right  = r->left + w;
    r->bottom = r->top + h;
    return kResultOk;
}

tresult PLUGIN_API NebulaEditor::onSize(ViewRect* newSize) {
    rect = *newSize;
    curW_ = newSize->getWidth();
    curH_ = newSize->getHeight();
    if (containerHwnd_) {
        SetWindowPos(containerHwnd_, nullptr, 0, 0, curW_, curH_,
                     SWP_NOZORDER | SWP_NOMOVE);
    }
    if (ctrl_) {
        RECT r{ 0, 0, curW_, curH_ };
        ctrl_->put_Bounds(r);
    }
    return kResultOk;
}

tresult PLUGIN_API NebulaEditor::onFocus(TBool state) {
    if (ctrl_ && state) {
        ctrl_->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
    }
    return kResultOk;
}

// ----------------------------------------------------------------------------
// Контейнер-окно. Ключевое для решения «фокус не передаётся» (пункт 9):
// мы перехватываем WM_MOUSEACTIVATE и возвращаем MA_ACTIVATE, что заставляет
// хост активировать наше окно при клике в любую точку.
// ----------------------------------------------------------------------------
LRESULT CALLBACK NebulaEditor::ContainerWndProc(HWND h, UINT msg, WPARAM w, LPARAM l){
    if (msg == WM_MOUSEACTIVATE) {
        // Активировать окно (а не дать клику просто провалиться)
        return MA_ACTIVATE;
    }
    if (msg == WM_SETFOCUS) {
        // когда наше окно получило фокус — пробросим в WebView
        auto* self = (NebulaEditor*)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (self && self->ctrl_) {
            self->ctrl_->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        }
        return 0;
    }
    if (msg == WM_SIZE) {
        auto* self = (NebulaEditor*)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (self && self->ctrl_) {
            RECT r{ 0, 0, LOWORD(l), HIWORD(l) };
            self->ctrl_->put_Bounds(r);
        }
        return 0;
    }
    return DefWindowProcW(h, msg, w, l);
}

tresult PLUGIN_API NebulaEditor::attached(void* parent, FIDString type) {
    parentHwnd_ = (HWND)parent;

    // Регистрируем класс окна (один раз на DLL)
    WNDCLASSEXW wc = { sizeof(wc) };
    if (!GetClassInfoExW(GetModuleHandleW(nullptr), kContainerClass, &wc)) {
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = ContainerWndProc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = kContainerClass;
        RegisterClassExW(&wc);
    }

    containerHwnd_ = CreateWindowExW(
        0, kContainerClass, L"Nebula",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, curW_, curH_,
        parentHwnd_, nullptr, GetModuleHandleW(nullptr), nullptr);

    if (containerHwnd_) {
        SetWindowLongPtrW(containerHwnd_, GWLP_USERDATA, (LONG_PTR)this);
    }

    if (controller_) controller_->setEditor(this);

    initWebView();
    return CPluginView::attached(parent, type);
}

tresult PLUGIN_API NebulaEditor::removed() {
    webReady_ = false;
    if (ctrl_) { ctrl_->Close(); ctrl_.Reset(); }
    web_.Reset();
    env_.Reset();
    if (containerHwnd_) { DestroyWindow(containerHwnd_); containerHwnd_ = nullptr; }
    parentHwnd_ = nullptr;
    if (controller_) controller_->setEditor(nullptr);
    return CPluginView::removed();
}

void NebulaEditor::initWebView() {
    wchar_t appData[MAX_PATH] = {};
    GetEnvironmentVariableW(L"LOCALAPPDATA", appData, MAX_PATH);
    std::wstring udf = std::wstring(appData) + L"\\Nebula\\WebView2";

    auto self = this;
    HWND host = containerHwnd_ ? containerHwnd_ : parentHwnd_;
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, udf.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [self, host](HRESULT, ICoreWebView2Environment* env) -> HRESULT {
                if (!env) return S_OK;
                self->env_ = env;
                env->CreateCoreWebView2Controller(host,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [self](HRESULT, ICoreWebView2Controller* ctrl) -> HRESULT {
                            if (!ctrl) return S_OK;
                            self->ctrl_ = ctrl;
                            ctrl->get_CoreWebView2(&self->web_);

                            RECT r{ 0, 0, self->curW_, self->curH_ };
                            ctrl->put_Bounds(r);
                            ctrl->put_IsVisible(TRUE);

                            ComPtr<ICoreWebView2Settings> s;
                            self->web_->get_Settings(&s);
                            if (s) {
                                s->put_AreDefaultContextMenusEnabled(FALSE);
                                s->put_IsStatusBarEnabled(FALSE);
                                s->put_AreDevToolsEnabled(FALSE);
                            }

                            self->web_->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [self](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        LPWSTR raw = nullptr;
                                        if (SUCCEEDED(args->TryGetWebMessageAsString(&raw)) && raw) {
                                            self->onWebMessage(std::wstring(raw));
                                            CoTaskMemFree(raw);
                                        }
                                        return S_OK;
                                    }).Get(), &self->msgToken_);

                            std::string html(kEmbeddedHtml);
                            int len = MultiByteToWideChar(CP_UTF8, 0, html.c_str(), -1, nullptr, 0);
                            std::wstring whtml(len, 0);
                            MultiByteToWideChar(CP_UTF8, 0, html.c_str(), -1, &whtml[0], len);
                            self->web_->NavigateToString(whtml.c_str());

                            self->web_->add_NavigationCompleted(
                                Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                    [self](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*) -> HRESULT {
                                        self->webReady_ = true;
                                        self->pushAllParamsToUI();
                                        return S_OK;
                                    }).Get(), nullptr);
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());
}

void NebulaEditor::showHostContextMenu(uint32_t paramId, int xClient, int yClient) {
    if (!controller_) return;
    FUnknownPtr<IComponentHandler3> handler3(controller_->getComponentHandler());
    if (!handler3) return;
    ParamID pid = (ParamID)paramId;
    IContextMenu* menu = handler3->createContextMenu(this, &pid);
    if (!menu) return;
    menu->popup((UCoord)xClient, (UCoord)yClient);
    menu->release();
}

void NebulaEditor::onWebMessage(const std::wstring& msg) {
    auto wfind = [&](const wchar_t* key, double def) -> double {
        size_t p = msg.find(key);
        if (p == std::wstring::npos) return def;
        p = msg.find(L':', p);
        if (p == std::wstring::npos) return def;
        return _wtof(msg.c_str() + p + 1);
    };
    auto wfindStr = [&](const wchar_t* key) -> std::wstring {
        size_t p = msg.find(key);
        if (p == std::wstring::npos) return L"";
        p = msg.find(L'"', p + wcslen(key));
        if (p == std::wstring::npos) return L"";
        size_t e = msg.find(L'"', p + 1);
        return msg.substr(p + 1, e - p - 1);
    };

    std::wstring type = wfindStr(L"\"type\"");
    if (type == L"param") {
        int id = (int)wfind(L"\"id\"", -1);
        double v = wfind(L"\"value\"", 0.0);
        if (id >= 0 && id < (int)kNumParams && controller_) {
            // Используем БАЗОВУЮ установку, чтобы не сработал наш override и
            // не зациклить обновление обратно в UI.
            controller_->EditController::setParamNormalized((ParamID)id, v);
            controller_->performEdit(id, v);
        }
    } else if (type == L"beginEdit") {
        int id = (int)wfind(L"\"id\"", -1);
        if (id >= 0 && id < (int)kNumParams && controller_)
            controller_->beginEdit(id);
    } else if (type == L"endEdit") {
        int id = (int)wfind(L"\"id\"", -1);
        if (id >= 0 && id < (int)kNumParams && controller_)
            controller_->endEdit(id);
    } else if (type == L"contextMenu") {
        int id = (int)wfind(L"\"id\"", -1);
        int x  = (int)wfind(L"\"x\"", 0);
        int y  = (int)wfind(L"\"y\"", 0);
        if (id >= 0 && id < (int)kNumParams)
            showHostContextMenu((uint32_t)id, x, y);
    } else if (type == L"noteOn") {
        int midi = (int)wfind(L"\"midi\"", -1);
        int vel  = (int)wfind(L"\"vel\"", 100);
        if (midi >= 0 && midi < 128) {
            auto* p = NebulaProcessor::current();
            if (p) p->postUiNote(midi, vel, true);
        }
    } else if (type == L"noteOff") {
        int midi = (int)wfind(L"\"midi\"", -1);
        if (midi >= 0 && midi < 128) {
            auto* p = NebulaProcessor::current();
            if (p) p->postUiNote(midi, 0, false);
        }
    }
}

void NebulaEditor::pushAllParamsToUI() {
    if (!web_ || !controller_) return;
    std::wstringstream ss;
    ss << L"window.nebulaInit && window.nebulaInit([";
    for (uint32_t i = 0; i < kNumParams; ++i) {
        if (i) ss << L",";
        ss << controller_->getParamNormalized(i);
    }
    ss << L"]);";
    web_->ExecuteScript(ss.str().c_str(), nullptr);
}

// Вызывается из Controller::setParamNormalized когда хост (FL Studio)
// или Paste из контекстного меню меняют параметр — пушим в JS.
void NebulaEditor::pushParamToUI(uint32_t paramId, double normalized) {
    if (!web_ || !webReady_) return;
    std::wstringstream ss;
    ss << L"window.nebulaSetParam && window.nebulaSetParam("
       << paramId << L"," << normalized << L");";
    web_->ExecuteScript(ss.str().c_str(), nullptr);
}

} // namespace

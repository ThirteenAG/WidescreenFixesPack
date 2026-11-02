#include "stdafx.h"
#include "FixSettings.hpp"
#include "d3d9.h"
#include "Shared/D3D9Options.hpp"

import ComVars;
import ButtonIcons;
import DeveloperOverlay;
import Focus;
import FrameRate;
import Gamepad;
import Mouse;
import NativeMenu;
import Updates;
import Window;

import Camera;
import Rendering;
import TurnIndicators;

#include "Shared/WidescreenFixConflict.hpp"

void Init()
{
    InitComVarsRequiredRefs();
    InitCameraRequiredRefs();
    InitRenderingRequiredRefs();
    InitTurnIndicatorsRequiredRefs();
    FixSettings();
    DPLMenu::Init();
    DPLUpdates::Install();
    DPLFocus::Install();
    DPLMouse::Install();
    DPLPad::Install();
    DPLIcons::Install();
    DPLMenu::padHelp = DPLPad::HelpPressed;
    DPLMenu::blocksInput = []() { return DPLOverlay::BlocksInput(); };
    DPLMenu::helpPrefix = []()
    {
        return DPLIcons::ready && DPLIcons::textureHook && !DPLIcons::textureFailed && !DPLIcons::fonts.empty() && !DPLIcons::fontTextures.empty() ? L"F1 / \u2463: " : L"F1: ";
    };
    DPLFocus::onDeactivate = []() { DPLPad::StopEffects(); };
    DPLMouse::drainController = DPLPad::Drain;
    WFP::onGameProcessEvent() += []()
    {
        DPLOverlay::AttachWindow(hWnd.get());
        DPLOverlay::Keyboard(hWnd.get());
        const bool modeInput = DPLWindow::Frame(Direct3DDevice, DPLOverlay::BlocksInput());
        BlockGameInput = modeInput || DPLOverlay::BlocksInput() || (hWnd.get() && GetForegroundWindow() != hWnd.get());
        DPLPad::Frame();
        DPLIcons::Frame();
        DPLUpdates::Tick();
    };
    WFP::onEndScene() += []()
    {
        DPLOverlay::Attach(Direct3DDevice);
        DPLIcons::Attach(Direct3DDevice);
    };
    WFP::onBeforeReset() += []() { DPLIcons::Reset(); };
    D3D9Options::beforeReset = [](IDirect3DDevice9* device)
    {
        if (device == Direct3DDevice.get()) WFP::onBeforeReset().executeAll();
    };

    asi_settings::LiveLink::Start();
    // Otherwise with display scaling Windows gives the game a scaled down desktop and stretches its window,
    // and the window does not match the display modes of the resolution list

    auto pattern = hook::pattern("A1 ? ? ? ? 85 C0 8D 7E");
    auto nativeWindow = *pattern.get_first<HWND*>(1);
    hWnd.SetAddress(nativeWindow);
    D3D9Options::frameLimit = 0;
    DPLWindow::Init();
    FixSettings().Availability("MAIN.WindowMode", [](std::string_view value) { return value == "0" || DPLWindow::ready; },
        [] { return "Window mode is unavailable."; });
    D3D9Options::HookCreation();

    pattern = hook::pattern("38 05 ? ? ? ? 75 ? 39 81 ? ? ? ? 74");
    dwWindowedMode.SetAddress(*pattern.get_first<uint8_t*>(2));
    DPLWindow::onCreate = []() { dwWindowedMode = DPLWindow::mode != 0; };

    pattern = hook::pattern("8B 45 ? 8B 48 ? 57");
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        static std::list<int64_t> times;
        static const int64_t frequency = []()
        {
            LARGE_INTEGER value{};
            QueryPerformanceFrequency(&value);
            return value.QuadPart;
        }();
        LARGE_INTEGER time{};
        QueryPerformanceCounter(&time);
        if (times.size() == 50) times.pop_front();
        times.push_back(time.QuadPart);
        const double elapsed = static_cast<double>(times.back() - times.front());
        fTimeStep = times.size() > 1 && elapsed > 0.0 && frequency > 0 ?
            static_cast<float>(std::clamp(elapsed / ((times.size() - 1) * frequency), 0.001, 0.1)) : 1.0f / 60.0f;

        static std::once_flag of;
        std::call_once(of, []()
        {
            WFP::onGameInitEvent().executeAll();
        });

        WFP::onGameProcessEvent().executeAll();
    });

    pCurrentMovieStatus.SetAddress(*hook::get_pattern<int32_t*>("BE ? ? ? ? 33 C0 B9 20 01 00 00 8B FE F3 AB", 1) + 0x1D);

    pattern = hook::pattern("8D 55 ? 8D B3");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    pattern = hook::pattern("A1 ? ? ? ? 8B 08 6A 00 50 FF 91 ? ? ? ? 8D 4E 18");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));

    auto ptr = (int*)Direct3DDevice.get_ptr();
    BackbufferWidth.SetAddress(ptr + 2);
    BackbufferHeight.SetAddress(ptr + 3);
    DPLWindow::publishDimensions = [](int width, int height)
    {
        BackbufferWidth = width; BackbufferHeight = height;
    };

    pattern = hook::pattern("C7 45 ? ? ? ? ? 8D BB");
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onEndScene().executeAll();
    });

    WFP::onInitEvent().executeAll();

    static auto futures = WFP::onInitEventAsync().executeAllAsync();

    WFP::onGameInitEvent() += []()
    {
        for (auto& f : futures.get())
            f.wait();
        futures.get().clear();
    };
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        WindowedModeWrapper::EnableDpiAwareness();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("55 8B EC 83 EC 60 53 56 57 68 ? ? ? ? 8D 4D D8 E8 ? ? ? ? 33 F6 33 DB 46 53 89 75 F4 FF"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}

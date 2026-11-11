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
import NativeMenu;
import Updates;
import Window;


ASI_SETTINGS_EXPORT(FixSettings())
#include "Shared/CursorConfinement.hpp"
#include "Shared/WidescreenFixConflict.hpp"

void Init()
{
    InputManager.SetAddress(*hook::pattern("A1 ? ? ? ? 56 8B F1 E8 ? ? ? ? 8B 8E C0 00 00 00").get_first<uintptr_t*>(1));
    WindowedModeWrapper::EnableDpiAwareness();
    Driv3rMenu::Init();
    Driv3rUpdates::Install();
    Driv3rFocus::Install();
    Driv3rIcons::Install();
    Driv3rPad::Install();
    Driv3rFocus::onDeactivate = []() { Driv3rPad::StopEffects(); };
    Driv3rMenu::padHelp = []() { return IsJoyButtonPressed(4); };
    Driv3rMenu::blocksInput = []() { return Driv3rOverlay::BlocksInput(); };
    Driv3rMenu::helpPrefix = []()
    {
        return Driv3rIcons::ready && Driv3rIcons::textureHook && !Driv3rIcons::textureFailed && !Driv3rIcons::fonts.empty() ? L"F1 / \u2463: " : L"F1: ";
    };
    WFP::onGameProcessEvent() += []()
    {
        Driv3rOverlay::AttachWindow(hWnd.get());
        Driv3rOverlay::Keyboard(hWnd.get());
        BlockGameInput = Driv3rOverlay::BlocksInput() || (hWnd.get() && GetForegroundWindow() != hWnd.get());
        BlockGameInput = Driv3rWindow::Frame(Direct3DDevice.get(), Driv3rOverlay::BlocksInput()) || BlockGameInput;
        Driv3rPad::Frame();
        CursorConfinement::Suspend(Driv3rOverlay::BlocksInput());
        Driv3rUpdates::Tick();
        Driv3rIcons::Frame();
    };
    WFP::onEndScene() += []()
    {
        Driv3rOverlay::Attach(Direct3DDevice);
        Driv3rIcons::Attach(Direct3DDevice);
    };
    WFP::onBeforeReset() += []() { Driv3rIcons::Reset(); };

    D3D9Options::frameLimit = 0;
    D3D9Options::anisotropy = FixSettings().Int("GRAPHICS.AnisotropicFiltering");
    D3D9Options::HookCreation();

    auto pattern = hook::pattern("A1 ? ? ? ? 85 C0 55");
    auto nativeWindow = *pattern.get_first<HWND*>(1);
    hWnd.SetAddress(nativeWindow);

    pattern = hook::pattern("E8 ? ? ? ? ? ? 8D 4C 24 ? 51 8D 54 24");
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPaused = true;

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

    pattern = hook::pattern("D9 44 24 ? A1 ? ? ? ? D9 18");
    static auto TimingHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPaused = false;
    });

    pattern = hook::pattern("B9 ? ? ? ? 8D 7C 24 ? 52");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    // Switching resolution and resizing the back buffer reset the device too, the reset above only runs when those fail
    pattern = hook::pattern("B9 0E 00 00 00 8B F5 8D 7C 24 ? 52 F3 A5");
    static auto BeforeModeResetHook = safetyhook::create_mid(pattern.count(2).get(0).get<void>(0), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    static auto BeforeResizeResetHook = safetyhook::create_mid(pattern.count(2).get(1).get<void>(0), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    auto dword_8D7DF0 = *hook::get_pattern<int*>("8B 0D ? ? ? ? 8B 44 24 14 8D 54 24 2C", 2);
    RendererModeIndex.SetAddress(dword_8D7DF0 - 2);
    BackbufferWidth.SetAddress(dword_8D7DF0 + 2);
    BackbufferHeight.SetAddress(dword_8D7DF0 + 3);
    Driv3rWindow::Init(reinterpret_cast<uint8_t*>(dword_8D7DF0 - 2) - 0x150);

    pattern = hook::pattern("A1 ? ? ? ? ? ? 8D 7B ? 57 6A ? 6A");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));

    pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? 50 FF 92 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 8B C5 8D 6F ? BB");
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

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        FixSettings();
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("55 8B EC 83 EC 60 53 56 57"));
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

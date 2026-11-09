module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"


export module Core;

import ComVars;
import Cutscene;
import Window;
import Hud;
import Camera;
import NativeMenu;
import DeveloperOverlay;


bool isTTB;
// Texture::CopyFromFrontBuffer read its target texture without checking it: in a window mode the texture can be
// missing when the copy is asked for, and the game crashed. Without a texture the copy now reports its failure.
SafetyHookInline CopyFromFrontBufferHook;
uint8_t nCopyTextureOffset = 0x64; // Ties That Bind: 0x68

int __fastcall CopyFromFrontBuffer(uintptr_t self, void*, int surface, int* rect)
{
    if (!*reinterpret_cast<void**>(self + nCopyTextureOffset))
        return 0;
    return CopyFromFrontBufferHook.unsafe_thiscall<int>(self, surface, rect);
}

void InitFrontBufferCopy()
{
    auto pattern = hook::pattern("83 EC 18 A1 ? ? ? ? 56 8D 54 24 0C 52 C7 44 24 08 00 00 00 00 8B 80 ? ? ? ? 6A 00 8B F1 8B 08 50 FF 51 20");
    auto code = pattern.get_first<uint8_t>();
    auto start = reinterpret_cast<uintptr_t>(code);
    auto texture = hook::range_pattern(start, start + 0x100, "85 C0 0F 85 ? ? ? ? 8B 46 ?");
    nCopyTextureOffset = *texture.get_first<uint8_t>(10);
    CopyFromFrontBufferHook = safetyhook::create_inline(code, CopyFromFrontBuffer);
}

// Loading a level could hang on multi-core CPUs: the main thread took the model lock and then the resource lock, the
// streaming thread the same two in the opposite order. The model routine now takes the resource lock instead (both
// are recursive critical sections), so the two threads always lock in one order.
void InitLoadingDeadlockFix()
{
    auto model = hook::pattern("6A FF B9 ? ? ? ? E8 ? ? ? ? F6 47 0C 01 75 08 56 8B CF E8 ? ? ? ? 68 ? ? ? ? FF 15");
    auto resource = hook::pattern("51 55 56 57 8B F1 6A FF B9 ? ? ? ? 89 74 24 10");
    const auto resourceLock = *resource.get_first<uint32_t>(9); //0x6DCBC0
    injector::WriteMemory<uint32_t>(model.get_first<uint32_t>(3), resourceLock, true); //0x458A1F
    injector::WriteMemory<uint32_t>(model.get_first<uint32_t>(27), resourceLock, true); //0x458A37
}

// Skip intro: the startup logos are the first movie the game makes, before any menu; the movie is asked to end at
// once (its end request, bit 0 of its flags), as the game's own skip does
void InitSkipIntro()
{
    if (!FixSettings().Bool("MAIN.SkipIntro"))
        return;
    auto pattern = hook::pattern("E8 ? ? ? ? 33 D2 C7 06 ? ? ? ? C7 46 58 ? ? ? ? C7 46 5C ? ? ? ? 89 56 60");
    static auto MovieCreated = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
    {
        static bool first = true;
        if (std::exchange(first, false))
            *reinterpret_cast<uint32_t*>(regs.esi + 0x28) |= 1;
    }); //0x48683B
}

std::atomic<int> requestedFrameLimit{ 0 };

void UpdateFrameLimit()
{
    requestedFrameLimit.store(FixSettings().Int("MAIN.FPSLimit"), std::memory_order_relaxed);
}

void ApplyFrameLimit(HWND target)
{
    const int requested = requestedFrameLimit.load(std::memory_order_relaxed);
    if (requested >= 0)
    {
        D3D9Options::frameLimit = requested;
        return;
    }
    static HMONITOR previous = nullptr;
    static ULONGLONG lastCheck = 0;
    static int refresh = 60;
    const auto monitor = MonitorFromWindow(target, MONITOR_DEFAULTTOPRIMARY);
    const auto now = GetTickCount64();
    if (monitor != previous || now - lastCheck >= 2000)
    {
        previous = monitor;
        lastCheck = now;
        MONITORINFOEXW info{};
        info.cbSize = sizeof(info);
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        refresh = GetMonitorInfoW(monitor, &info) &&
            EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ?
            int(mode.dmDisplayFrequency) : 60;
    }
    D3D9Options::frameLimit = refresh;
}

void InitFrame()
{
    UpdateFrameLimit();
    FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
    CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    FixSettings().OnChange("MAIN.ConfineCursor", []()
    {
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    });
    D3D9Options::frame = [](IDirect3DDevice9* device)
    {
        ApplyFrameLimit(hGameWindow);
        CursorConfinement::Suspend(SufferingOverlay::BlocksInput());
        CursorConfinement::Frame(hGameWindow);
        RWGTA::UpdateCheck::Tick();
        if (NativeFOV.is_initialized())
            Screen.fFieldOfView = CurrentFieldOfView();
        UpdateCameraLenses();
        ResetIconProjection();
        HookIconDevice(device);
        DrawPillarbox(device);
        SufferingOverlay::Present(device, hGameWindow);
    };
    D3D9Options::beforeReset = SufferingOverlay::BeforeReset;

    // Update notice: in the fix's page of the game's own menu
    RWGTA::UpdateCheck::Config update;
    update.pluginId = "TheSuffering.FusionFix";
    update.fileVersion = rsc_FileVersion;
    update.settings = &FixSettings();
    update.playable = []() { return true; };
    update.show = [](const std::wstring& line) { SufferingMenu::notice = asi_settings::text::Narrow(line); };
    RWGTA::UpdateCheck::Init(std::move(update));
    D3D9Options::HookCreation();

    // The game's device without D3DCREATE_PUREDEVICE (0x54 -> 0x44): the overlay reads and restores the device's state
    auto flags = hook::pattern("6A 54 51 8B 0D ? ? ? ? 6A 01 51 50 FF 52 40");
    injector::WriteMemory<uint8_t>(flags.get_first<uint8_t>(1), 0x44, true); //0x49D44F

    auto pattern = hook::pattern("83 EC 40 56 8B 74 24 4C 81 FE 02 01 00 00 57 8B 7C 24 54 0F 87");
    WindowProcedureHook = safetyhook::create_inline(pattern.get_first(), WindowProcedure); //0x437AA0
}

void InitPatches()
{
    hook::pattern pattern;
    InitWindowMode();
    InitFrame();
    InitFrontBufferCopy();
    InitLoadingDeadlockFix();
    InitCameraLenses();
    InitSkipIntro();
    pCutsceneBorders = &FixSettings().Int("MAIN.CutsceneBorders");
    pFOVFactor = &FixSettings().Float("MAIN.FOVFactor");

    //unlocking resolutions
    pattern = hook::pattern("75 4B D9 45 ? D8 5D ? DF E0 F6 C4 41");
    injector::MakeNOP(pattern.get_first(0), 2, true);
    injector::MakeNOP(pattern.get_first(13), 2, true);

    //unlocking windowed resolutions
    pattern = hook::pattern("8B 15 ? ? ? ? 83 E2 01"); //0x4B35E2
    injector::WriteMemory<uint16_t>(pattern.get_first<uint16_t>(9), 0xE990, true);

    //The Game Play disk could not be found error
    pattern = hook::pattern("85 C0 ? ? 8B 4C 24 0C 51"); //0x43B989 in ttb
    injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(2), 0xEB, true); //0x43B959
    pattern = hook::pattern("75 ? 8D 44 24 08 50 8B 44 24 10");
    injector::MakeNOP(pattern.get_first(0), 2, true);

    InitFOV();

    InitHud();

    InitResolution();

    InitCutsceneBorders();
}


class Core
{
public:
    Core()
    {
        WFP::onInitEvent() += InitPatches;
    }
} Core;

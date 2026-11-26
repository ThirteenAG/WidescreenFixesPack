module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/RefreshRatePicker.hpp"
#include "Shared/RWGTA/WindowRefreshRate.hpp"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") // needed for timeBeginPeriod()/timeEndPeriod()

export module Menu;

import Skeleton;
import Options;

export enum CutsceneBordersMode : int8_t
{
    Off = 0,
    Letterbox = 1,
    Pillarbox = 2,
    Both = 3
};

export enum FrameLimiterMode : int8_t
{
    eOff, e30, e40, e50, e60, e75, e100, e120, e144, e165, e200, e240, e90, e180
};

// MAIN.FPSLimit: 0 unlimited, -1 the monitor's refresh rate, or frames per second; the native row's steps
constexpr int FrameLimitSteps[] = { 0, -1, 30, 40, 50, 60, 75, 90, 100, 120, 144, 165, 180, 200, 240 };

static int NextFrameLimit(int current, int direction)
{
    constexpr int count = int(std::size(FrameLimitSteps));
    auto it = std::find(std::begin(FrameLimitSteps), std::end(FrameLimitSteps), current);
    if (it != std::end(FrameLimitSteps))
        return FrameLimitSteps[(int(it - std::begin(FrameLimitSteps)) + count + direction) % count];
    // A value set elsewhere: the nearest step that way
    if (direction > 0)
    {
        for (int i = 2; i < count; ++i)
            if (FrameLimitSteps[i] > current)
                return FrameLimitSteps[i];
        return FrameLimitSteps[0];
    }
    for (int i = count - 1; i >= 2; --i)
        if (FrameLimitSteps[i] < current)
            return FrameLimitSteps[i];
    return FrameLimitSteps[count - 1];
}

static int cutsceneBorders = 3;
static int frameLimit = -1;

static void ReadGameplayPreferences()
{
    cutsceneBorders = std::clamp(ReadFixSetting("MAIN.CutsceneBorders", 3), 0, 3);
    frameLimit = ReadFixSetting("MAIN.FPSLimit", -1);
}

static int CutsceneBordersSetting() { return cutsceneBorders; }
static int FrameLimitSetting() { return frameLimit; }

export namespace CMenuManager
{
    GameRef<int8_t> m_PrefsUseWideScreen([]() -> int8_t*
    {
        auto pattern = hook::pattern("C6 05 ? ? ? ? ? C6 05 ? ? ? ? ? 8B 45 ? 89 45");
        return *pattern.get_first<int8_t*>(2);
    });
}

export namespace CMenuManager
{
    GameRef<int8_t> m_PrefsFrameLimiter([]() -> int8_t*
    {
        auto pattern = hook::pattern("C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? C6 05");
        return *pattern.get_first<int8_t*>(2);
    });

    GameRef<int8_t> m_bIsActive([]() -> int8_t*
    {
        auto pattern = hook::pattern("80 3D ? ? ? ? ? 74 ? 80 3D ? ? ? ? ? 0F 85 ? ? ? ? B9");
        return *pattern.get_first<int8_t*>(2);
    });
}

namespace CText
{
    injector::hook_back<const wchar_t* (__fastcall*)(void*, void*, const char*)> hbGet;
    // The row text's call site keeps its own previous target (Options.ixx answers the fix's rows there too)
    injector::hook_back<const wchar_t* (__fastcall*)(void*, void*, const char*)> hbGetRowText;
    const wchar_t* __fastcall GetMenuOptionText(void* CText, void* edx, const char* name)
    {
        if (std::string_view(name) == "FED_WIS")
        {
            auto text = FixOptions::GameText("cutsceneborders");
            return *text ? text : L"CUTSCENE BORDERS";
        }
        return hbGetRowText.fun(CText, edx, name);
    }

    const wchar_t* Translated(const char* key, const wchar_t* fallback)
    {
        auto text = FixOptions::GameText(key);
        return *text ? text : fallback;
    }

    const wchar_t* GetCutsceneBordersText(void* CText, void* edx, const char* name)
    {
        switch (CutsceneBordersSetting())
        {
            case CutsceneBordersMode::Off:
                return hbGet.fun(CText, edx, "FEM_OFF");
            case CutsceneBordersMode::Letterbox:
                return Translated("cutsceneborders.letterbox", L"LETTERBOX");
            case CutsceneBordersMode::Pillarbox:
                return Translated("cutsceneborders.pillarbox", L"PILLARBOX");
            default:
                return Translated("cutsceneborders.both", L"LETTERBOX + PILLARBOX");
        }
    }

    const wchar_t* __fastcall GetCutsceneBordersOn(void* CText, void* edx, const char* name)
    {
        return GetCutsceneBordersText(CText, edx, name);
    }

    const wchar_t* __fastcall GetCutsceneBordersOff(void* CText, void* edx, const char* name)
    {
        return GetCutsceneBordersText(CText, edx, name);
    }

    const wchar_t* GetFrameLimiterText(void* CText, void* edx, const char* name)
    {
        int limit = FrameLimitSetting();
        if (limit == 0)
            return Translated("fpslimit.unlimited", L"UNLIMITED");
        if (limit < 0)
            return Translated("fpslimit.monitor", L"MONITOR REFRESH RATE");
        static std::wstring value;
        value = Translated("fpslimit.value", L"{0} FPS");
        auto at = value.find(L"{0}");
        if (at == std::wstring::npos)
            value = std::to_wstring(limit) + L" " + value;
        else
            value.replace(at, 3, std::to_wstring(limit));
        return value.c_str();
    }

    const wchar_t* __fastcall GetFrameLimiterOn(void* CText, void* edx, const char* name)
    {
        return GetFrameLimiterText(CText, edx, name);
    }

    const wchar_t* __fastcall GetFrameLimiterOff(void* CText, void* edx, const char* name)
    {
        return GetFrameLimiterText(CText, edx, name);
    }
}

enum RsEvent
{
    rsCAMERASIZE,
    rsCOMMANDLINE,
    rsFILELOAD,
    rsINITDEBUG,
    rsINPUTDEVICEATTACH,
    rsLEFTBUTTONDOWN,
    rsLEFTBUTTONUP,
    rsMOUSEMOVE,
    rsMOUSEWHEELMOVE,
    rsPLUGINATTACH,
    rsREGISTERIMAGELOADER,
    rsRIGHTBUTTONDOWN,
    rsRIGHTBUTTONUP,
    _rs_13,
    _rs_14,
    _rs_15,
    _rs_16,
    _rs_17,
    _rs_18,
    _rs_19,
    _rs_20,
    rsRWINITIALIZE,
    rsRWTERMINATE,
    rsSELECTDEVICE,
    rsINITIALIZE,
    rsTERMINATE,
    rsIDLE,
    rsFRONTENDIDLE,
    rsKEYDOWN,
    rsKEYUP,
    rsQUITAPP,
    rsPADBUTTONDOWN,
    rsPADBUTTONUP,
    rsPADANALOGUELEFT,
    rsPADANALOGUELEFTRESET,
    rsPADANALOGUERIGHT,
    rsPADANALOGUERIGHTRESET,
    rsPREINITCOMMANDLINE,
    rsACTIVATE,
};

class FrameLimiter
{
public:
    enum FPSLimitMode
    {
        FPS_NONE,
        FPS_REALTIME,
        FPS_ACCURATE
    };

    FPSLimitMode mFPSLimitMode = FPS_NONE;

private:
    double TIME_Frequency = 0.0;
    double TIME_Ticks = 0.0;
    double TIME_Frametime = 0.0;
    float fFPSLimit = 0.0f;
    bool bFpsLimitWasUpdated = false;

public:
    void Init(FPSLimitMode mode, float fps_limit)
    {
        bFpsLimitWasUpdated = true;
        mFPSLimitMode = mode;
        fFPSLimit = fps_limit;

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        static constexpr auto TICKS_PER_FRAME = 1;
        auto TICKS_PER_SECOND = (TICKS_PER_FRAME * fFPSLimit);

        if (mFPSLimitMode == FPS_ACCURATE)
        {
            TIME_Frametime = 1000.0 / (double)fFPSLimit;
            TIME_Frequency = (double)frequency.QuadPart / 1000.0;
        }
        else
        {
            TIME_Frequency = (double)frequency.QuadPart / (double)TICKS_PER_SECOND;
        }

        Ticks();
    }

    DWORD Sync_RT()
    {
        if (bFpsLimitWasUpdated)
        {
            bFpsLimitWasUpdated = false;
            return 1;
        }

        DWORD lastTicks, currentTicks;
        LARGE_INTEGER counter;

        QueryPerformanceCounter(&counter);
        lastTicks = (DWORD)TIME_Ticks;
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
        currentTicks = (DWORD)TIME_Ticks;

        return (currentTicks > lastTicks) ? currentTicks - lastTicks : 0;
    }

    DWORD Sync_SLP()
    {
        if (bFpsLimitWasUpdated)
        {
            bFpsLimitWasUpdated = false;
            return 1;
        }

        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        double millis_current = (double)counter.QuadPart / TIME_Frequency;
        double millis_delta = millis_current - TIME_Ticks;

        if (TIME_Frametime <= millis_delta)
        {
            TIME_Ticks = millis_current;
            return 1;
        }
        else if (TIME_Frametime - millis_delta > 2.0)
            Sleep(1);
        else
            Sleep(0);

        return 0;
    }

    void Sync()
    {
        if (fFPSLimit <= 0.0f)
            return;

        if (mFPSLimitMode == FPS_REALTIME)
            while (!Sync_RT());
        else if (mFPSLimitMode == FPS_ACCURATE)
            while (!Sync_SLP());
    }

private:
    void Ticks()
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
    }
};

static float GetFrameLimiterValue()
{
    int limit = FrameLimitSetting();
    if (limit < 0)
    {
        const auto window = RsGlobal->ps ? *reinterpret_cast<HWND*>(RsGlobal->ps) : nullptr;
        return float(RWGTA::WindowRefreshRate(window));
    }
    return float(limit);
}

// The game's own preferences follow the settings: the cutscene borders' mode, and the frame limiter on or off
static void MirrorPreferences()
{
    CMenuManager::m_PrefsUseWideScreen = int8_t(CutsceneBordersSetting());
    CMenuManager::m_PrefsFrameLimiter = int8_t(FrameLimitSetting() != 0 ? FrameLimiterMode::e30 : FrameLimiterMode::eOff);
}

// Keep menu pacing independent of the gameplay limiter's timestamps and setting.
static FrameLimiter menuFpsLimiter;
static float menuFrameLimit = 60.0f;
static bool menuFrameLimitChanged = false;

static void LimitMenuFrame()
{
    if (menuFrameLimit <= 0.0f)
        return;

    static bool initialized = false;
    if (!initialized || menuFrameLimitChanged)
    {
        if (!initialized)
            timeBeginPeriod(1);
        menuFpsLimiter.Init(FrameLimiter::FPS_ACCURATE, menuFrameLimit);
        initialized = true;
        menuFrameLimitChanged = false;
    }
    menuFpsLimiter.Sync();
}

injector::hook_back<bool(__cdecl*)(RsEvent event, void* data)> hbRsEventHandler;
bool __cdecl RsEventHandler(RsEvent event, void* data)
{
    if (CMenuManager::m_bIsActive)
        return hbRsEventHandler.fun(event, data);

    static FrameLimiter fpsLimiter;
    static constexpr auto mode = FrameLimiter::FPS_ACCURATE;
    static bool bTimerResolutionSet = false;
    static float fLastFpsLimit = -1.0f;

    float fFpsLimit = GetFrameLimiterValue();

    if (fFpsLimit != fLastFpsLimit)
    {
        fLastFpsLimit = fFpsLimit;

        if (!bTimerResolutionSet)
        {
            timeBeginPeriod(1);
            bTimerResolutionSet = true;
        }

        RsGlobal->maxFPS = static_cast<int32_t>(fFpsLimit);
        fpsLimiter.Init(mode, fFpsLimit);
    }

    fpsLimiter.Sync();

    return hbRsEventHandler.fun(event, data);
}

static SafetyHookInline DrawMenuHook;

static void __fastcall DrawMenuWithIndicators(uint8_t* manager, void*)
{
    FixOptions::BeginMenuDraw();
    DrawMenuHook.unsafe_thiscall<void>(manager);
    FixOptions::DrawScrollIndicators();
}

class Menu
{
public:
    Menu()
    {
        WFP::onInitEvent() += []()
        {
            RWGTA::InstallRefreshRatePicker<false>("39 E8 73 07 83 F8 3C 72 02 89 C5");
        };

        WFP::onGameInitEvent() += []()
        {
            // The cutscene borders and the frame limit are the fix's settings, whatever the game's settings file had
            ReadGameplayPreferences();
            MirrorPreferences();
            OnSettingChange("MAIN.CutsceneBorders", ReadGameplayPreferences);
            OnSettingChange("MAIN.FPSLimit", ReadGameplayPreferences);
            WFP::onGameProcessEvent() += []()
            {
                MirrorPreferences();
            };

            menuFrameLimit = static_cast<float>(std::max(0, ReadFixSetting("MAIN.FPSLimitMenus", 60)));
            OnSettingChange("MAIN.FPSLimitMenus", []()
            {
                menuFrameLimit = static_cast<float>(std::max(0, ReadFixSetting("MAIN.FPSLimitMenus", 60)));
                menuFrameLimitChanged = true;
            });

            auto pattern = hook::pattern("E8 ? ? ? ? 89 44 24 ? 8B B5");
            CText::hbGetRowText.fun = injector::MakeCALL(pattern.get_first(0), CText::GetMenuOptionText).get();
            CText::hbGet.fun = CText::hbGetRowText.fun;

            pattern = hook::pattern("E8 ? ? ? ? 89 44 24 ? E9 ? ? ? ? 8D 84 20 ? ? ? ? B9 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 44 24 ? E9 ? ? ? ? 0F BE 05");
            CText::hbGet.fun = injector::MakeCALL(pattern.get_first(0), CText::GetCutsceneBordersOn).get();

            pattern = hook::pattern("E8 ? ? ? ? 89 44 24 ? E9 ? ? ? ? 0F BE 05");
            CText::hbGet.fun = injector::MakeCALL(pattern.get_first(0), CText::GetCutsceneBordersOff).get();

            pattern = hook::pattern("A2 ? ? ? ? 6A ? 68 ? ? ? ? E8 ? ? ? ? 89 D9 E9 ? ? ? ? 31 C0 80 3D ? ? ? ? ? 75 ? 40 A2");
            injector::MakeNOP(pattern.get_first(), 5, true);
            injector::MakeNOP(pattern.get_first(-6), 1, true);
            static auto ProcessOnOffMenuOptionsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                int direction = *(uint8_t*)(regs.esp + 0xC) ? 1 : -1;
                int borders = (CutsceneBordersSetting() + direction + 4) % 4;
                FixSettings().Set("MAIN.CutsceneBorders", std::to_string(borders));
                ReadGameplayPreferences();
                MirrorPreferences();
            });

            pattern = hook::pattern("E8 ? ? ? ? 89 44 24 ? E9 ? ? ? ? 89 C0");
            CText::hbGet.fun = injector::MakeCALL(pattern.get_first(0), CText::GetFrameLimiterOn).get();

            pattern = hook::pattern("E8 ? ? ? ? 89 44 24 ? E9 E8 05 00 00");
            CText::hbGet.fun = injector::MakeCALL(pattern.get_first(0), CText::GetFrameLimiterOff).get();

            pattern = hook::pattern("A2 ? ? ? ? 6A 00 68 98 00 00 00 E8 ? ? ? ? 89 D9 E8 ? ? ? ? E9 78 02 00 00");
            injector::MakeNOP(pattern.get_first(), 5, true);
            injector::MakeNOP(pattern.get_first(-6), 1, true);
            static auto ProcessOnOffMenuOptionsHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                int direction = *(uint8_t*)(regs.esp + 0xC) ? 1 : -1;
                FixSettings().Set("MAIN.FPSLimit", std::to_string(NextFrameLimit(FrameLimitSetting(), direction)));
                ReadGameplayPreferences();
                MirrorPreferences();
            });

            // A new screen resolution (CMenuManager::ProcessButtonPresses) resizes the window under a cursor that
            // stays put, and the menu took the cursor's new place in the window (or at its edge) for a mouse movement
            // and selected the row there. Until the cursor really moves on the screen, the menu doesn't see one.
            static POINT resolutionCursor{};
            static bool resolutionCursorHeld = false;
            pattern = hook::pattern("8B 45 04 3B 45 00 0F 84 ? ? ? ? 89 45 00 8B 45 00 50 E8 ? ? ? ? 59");
            static auto ResolutionAppliedHook = safetyhook::create_mid(pattern.get_first(24), [](SafetyHookContext&)
            {
                resolutionCursorHeld = GetCursorPos(&resolutionCursor) != FALSE;
            });

            // CMenuManager::ProcessButtonPresses, once the mouse position is updated (previous at +53Ch/+540h, current
            // at +118h/+11Ch): row hover in CMenuManager::Draw only follows a position that differs from the last one
            pattern = hook::pattern("83 F8 1E 74 ? 83 F8 36 74 ? 83 F8 37 0F 85");
            static auto MouseMovedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (!resolutionCursorHeld)
                    return;
                POINT cursor{};
                if (!GetCursorPos(&cursor) || cursor.x != resolutionCursor.x || cursor.y != resolutionCursor.y)
                {
                    resolutionCursorHeld = false;
                    return;
                }
                auto manager = reinterpret_cast<uint8_t*>(regs.ebp);
                auto field = [manager](size_t offset) -> int32_t& { return *reinterpret_cast<int32_t*>(manager + offset); };
                field(0x53C) = field(0x118);
                field(0x540) = field(0x11C);
            });

            pattern = hook::pattern("74 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0 80 E4 ? 80 FC ? 0F 85 ? ? ? ? 6A");
            injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true);

            pattern = hook::pattern("E8 ? ? ? ? 59 59 EB ? A1 ? ? ? ? 50");
            hbRsEventHandler.fun = injector::MakeCALL(pattern.get_first(0), RsEventHandler).get();

            pattern = hook::pattern("E8 ? ? ? ? 83 BB ? ? ? ? ? 59 75 ? 80 BB");
            static auto DrawFrontEndHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
            {
                LimitMenuFrame();
            });
            DrawMenuHook = safetyhook::create_inline(pattern.get_first<uint8_t>(-9), DrawMenuWithIndicators);
        };
    }
} Menu;
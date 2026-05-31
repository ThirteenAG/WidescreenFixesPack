module;

#include <stdafx.h>
#include "common.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") // needed for timeBeginPeriod()/timeEndPeriod()

export module Menu;

import Skeleton;

export enum CutsceneBordersMode : int8_t
{
    Off = 0,
    Letterbox = 1,
    Pillarbox = 2,
    Both = 3
};

export enum FrameLimiterMode : int8_t
{
    eOff, e30, e40, e50, e60, e75, e100, e120, e144, e165, e200, e240
};

export namespace CMenuManager
{
    GameRef<int8_t> m_PrefsUseWideScreen([]() -> int8_t*
    {
        auto pattern = hook::pattern("C6 05 ? ? ? ? ? C6 05 ? ? ? ? ? 8B 45 ? 89 45");
        if (!pattern.empty())
            return *pattern.get_first<int8_t*>(2);
        return nullptr;
    });
}

export namespace CMenuManager
{
    GameRef<int8_t> m_PrefsFrameLimiter([]() -> int8_t*
    {
        auto pattern = hook::pattern("C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? C6 05");
        if (!pattern.empty())
            return *pattern.get_first<int8_t*>(2);
        return nullptr;
    });

    GameRef<int8_t> m_bIsActive([]() -> int8_t*
    {
        auto pattern = hook::pattern("80 3D ? ? ? ? ? 74 ? 80 3D ? ? ? ? ? 0F 85 ? ? ? ? B9");
        if (!pattern.empty())
            return *pattern.get_first<int8_t*>(2);
        return nullptr;
    });
}

namespace CText
{
    injector::hook_back<const wchar_t* (__fastcall*)(void*, void*, const char*)> hbGet;
    const wchar_t* __fastcall GetMenuOptionText(void* CText, void* edx, const char* name)
    {
        if (std::string_view(name) == "FED_WIS")
            return L"CUTSCENE BORDERS";
        return hbGet.fun(CText, edx, name);
    }

    const wchar_t* GetCutsceneBordersText(void* CText, void* edx, const char* name)
    {
        switch (CMenuManager::m_PrefsUseWideScreen)
        {
            case CutsceneBordersMode::Off:
                return hbGet.fun(CText, edx, "FEM_OFF");
            case CutsceneBordersMode::Letterbox:
                return L"LETTERBOX";
            case CutsceneBordersMode::Pillarbox:
                return L"PILLARBOX";
            case CutsceneBordersMode::Both:
                return L"LETTERBOX + PILLARBOX";
            default:
                break;
        };

        return hbGet.fun(CText, edx, "FEM_ON");
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
        switch (CMenuManager::m_PrefsFrameLimiter)
        {
            case FrameLimiterMode::eOff:
                return hbGet.fun(CText, edx, "FEM_OFF");
            case FrameLimiterMode::e30:
                return L"30 FPS";
            case FrameLimiterMode::e40:
                return L"40 FPS";
            case FrameLimiterMode::e50:
                return L"50 FPS";
            case FrameLimiterMode::e60:
                return L"60 FPS";
            case FrameLimiterMode::e75:
                return L"75 FPS";
            case FrameLimiterMode::e100:
                return L"100 FPS";
            case FrameLimiterMode::e120:
                return L"120 FPS";
            case FrameLimiterMode::e144:
                return L"144 FPS";
            case FrameLimiterMode::e165:
                return L"165 FPS";
            case FrameLimiterMode::e200:
                return L"200 FPS";
            case FrameLimiterMode::e240:
                return L"240 FPS";
            default:
                break;
        };

        return hbGet.fun(CText, edx, "FEM_ON");
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
    switch (CMenuManager::m_PrefsFrameLimiter)
    {
        case FrameLimiterMode::e30:  return 30.0f;
        case FrameLimiterMode::e40:  return 40.0f;
        case FrameLimiterMode::e50:  return 50.0f;
        case FrameLimiterMode::e60:  return 60.0f;
        case FrameLimiterMode::e75:  return 75.0f;
        case FrameLimiterMode::e100: return 100.0f;
        case FrameLimiterMode::e120: return 120.0f;
        case FrameLimiterMode::e144: return 144.0f;
        case FrameLimiterMode::e165: return 165.0f;
        case FrameLimiterMode::e200: return 200.0f;
        case FrameLimiterMode::e240: return 240.0f;
        case FrameLimiterMode::eOff:
        default:
            return 0.0f;
    }
}

injector::hook_back<bool(__cdecl*)(RsEvent event, void* data)> hbRsEventHandler;
bool __cdecl RsEventHandler(RsEvent event, void* data)
{
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

class Menu
{
public:
    Menu()
    {
        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("E8 ? ? ? ? 89 44 24 ? 8B B5");
            CText::hbGet.fun = injector::MakeCALL(pattern.get_first(0), CText::GetMenuOptionText).get();

            pattern = hook::pattern("E8 ? ? ? ? 89 44 24 ? E9 ? ? ? ? 8D 84 20 ? ? ? ? B9 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 44 24 ? E9 ? ? ? ? 0F BE 05");
            CText::hbGet.fun = injector::MakeCALL(pattern.get_first(0), CText::GetCutsceneBordersOn).get();

            pattern = hook::pattern("E8 ? ? ? ? 89 44 24 ? E9 ? ? ? ? 0F BE 05");
            CText::hbGet.fun = injector::MakeCALL(pattern.get_first(0), CText::GetCutsceneBordersOff).get();

            pattern = hook::pattern("A2 ? ? ? ? 6A ? 68 ? ? ? ? E8 ? ? ? ? 89 D9 E9 ? ? ? ? 31 C0 80 3D ? ? ? ? ? 75 ? 40 A2");
            injector::MakeNOP(pattern.get_first(), 5, true);
            injector::MakeNOP(pattern.get_first(-6), 1, true);
            static auto ProcessOnOffMenuOptionsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto pref = CMenuManager::m_PrefsUseWideScreen;

                auto menuDirection = 1;

                if (!*(uint8_t*)(regs.esp + 0xC))
                    menuDirection = -1;

                pref += menuDirection;

                if (pref > CutsceneBordersMode::Both)
                    pref = CutsceneBordersMode::Off;
                if (pref < CutsceneBordersMode::Off)
                    pref = CutsceneBordersMode::Both;
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
                auto pref = CMenuManager::m_PrefsFrameLimiter;

                auto menuDirection = 1;

                if (!*(uint8_t*)(regs.esp + 0xC))
                    menuDirection = -1;

                pref += menuDirection;

                if (pref > FrameLimiterMode::e240)
                    pref = FrameLimiterMode::eOff;
                if (pref < FrameLimiterMode::eOff)
                    pref = FrameLimiterMode::e240;
            });

            pattern = hook::pattern("74 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0 80 E4 ? 80 FC ? 0F 85 ? ? ? ? 6A");
            injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true);

            pattern = hook::pattern("E8 ? ? ? ? 59 59 EB ? A1 ? ? ? ? 50");
            hbRsEventHandler.fun = injector::MakeCALL(pattern.get_first(0), RsEventHandler).get();
        };
    }
} Menu;
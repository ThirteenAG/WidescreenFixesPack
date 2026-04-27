module;

#include <stdafx.h>
#include "common.h"

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

            pattern = hook::pattern("DB 05 ? ? ? ? D8 3D ? ? ? ? D8 D9 DF E0 80 E4 05 80 FC 01 0F 85");
            struct GenerationDistMultiplierFMUL
            {
                void operator()(injector::reg_pack& regs)
                {
                    float maxFPS = 30.0f;
                    auto pref = CMenuManager::m_PrefsFrameLimiter.get();
                    switch (pref)
                    {
                        case FrameLimiterMode::e30: maxFPS = 30.0f; break;
                        case FrameLimiterMode::e40: maxFPS = 40.0f; break;
                        case FrameLimiterMode::e50: maxFPS = 50.0f; break;
                        case FrameLimiterMode::e60: maxFPS = 60.0f; break;
                        case FrameLimiterMode::e75: maxFPS = 75.0f; break;
                        case FrameLimiterMode::e100: maxFPS = 100.0f; break;
                        case FrameLimiterMode::e120: maxFPS = 120.0f; break;
                        case FrameLimiterMode::e144: maxFPS = 144.0f; break;
                        case FrameLimiterMode::e165: maxFPS = 165.0f; break;
                        case FrameLimiterMode::e200: maxFPS = 200.0f; break;
                        case FrameLimiterMode::e240: maxFPS = 240.0f; break;
                        default:
                            break;
                    }

                    _asm {fld dword ptr[maxFPS]}

                }
            }; injector::MakeInline<GenerationDistMultiplierFMUL>(pattern.get_first(0), pattern.get_first(6));
        };
    }
} Menu;
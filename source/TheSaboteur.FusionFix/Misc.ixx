module;

#include <stdafx.h>

export module Misc;

import ComVars;

static int WINAPI GetSystemMetrics_Hook(int nIndex)
{
    if (nIndex == SM_CYCAPTION || nIndex == SM_CYSIZEFRAME)
        return 0;
    return GetSystemMetrics(nIndex);
}

static HWND WINAPI CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    auto hWnd = WindowedModeWrapper::CreateWindowExA_Hook(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    if (hWnd && hWnd == WindowedModeWrapper::GameHWND)
        WindowedModeWrapper::SetWindowLongA_Hook(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE));
    return hWnd;
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            auto bBorderlessWindowed = iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0;
            auto nFPSLimit = iniReader.ReadInteger("MAIN", "FPSLimit", 0);

            // Unlock resolutions with any refresh rate
            auto pattern = hook::pattern("74 ? 83 C6 01 83 C0 20");
            injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true);

            // Whole section below is for fixing the game above 1080p. God this game is broken.
            pattern = hook::pattern("8B ? A8 02 00 00 3D 00 04 00 00");
            static std::vector<SafetyHookMid> HudDrawCountHooks;
            for (size_t i = 0; i < pattern.size(); i++)
            {
                static constexpr auto nMaxDraws = 1024u;
                static constexpr auto nDrawCountOffset = 0x2A8;
                auto match = pattern.get(i);
                switch (*match.get<uint8_t>(1) & 7)
                {
                    case 1:
                        HudDrawCountHooks.emplace_back(safetyhook::create_mid(match.get<void>(6), [](SafetyHookContext& regs)
                        {
                            if (regs.eax >= nMaxDraws)
                                regs.eax = *(uint32_t*)(regs.ecx + nDrawCountOffset) = nMaxDraws - 1;
                        }));
                        break;
                    case 6:
                        HudDrawCountHooks.emplace_back(safetyhook::create_mid(match.get<void>(6), [](SafetyHookContext& regs)
                        {
                            if (regs.eax >= nMaxDraws)
                                regs.eax = *(uint32_t*)(regs.esi + nDrawCountOffset) = nMaxDraws - 1;
                        }));
                        break;
                    case 7:
                        HudDrawCountHooks.emplace_back(safetyhook::create_mid(match.get<void>(6), [](SafetyHookContext& regs)
                        {
                            if (regs.eax >= nMaxDraws)
                                regs.eax = *(uint32_t*)(regs.edi + nDrawCountOffset) = nMaxDraws - 1;
                        }));
                        break;
                    default:
                        break;
                }
            }

            pattern = hook::pattern("C7 44 24 1C 30 00 00 00 89 54 24 20 C7 44 24 24 00 01 00 00 E8");
            static auto FontCacheConfigHook = safetyhook::create_mid(pattern.get_first(20), [](SafetyHookContext& regs)
            {
                struct TextureConfig
                {
                    uint32_t TextureWidth;
                    uint32_t TextureHeight;
                    uint32_t MaxNumTextures;
                    uint32_t MaxSlotHeight;
                    uint32_t SlotPadding;
                    uint32_t TexUpdWidth;
                    uint32_t TexUpdHeight;
                };

                auto pConfig = *(TextureConfig**)(regs.esp);
                pConfig->TextureWidth = 2048;
                pConfig->TextureHeight = 2048;
                pConfig->MaxSlotHeight = 96;
            });

            pattern = hook::pattern("A1 ? ? ? ? 8B 88 74 06 00 00 DB 80 74 06 00 00");
            static auto ppRenderer = *pattern.get_first<uintptr_t*>(1);

            static auto GetCursorSpeedBoost = []() -> float
            {
                auto pRenderer = *ppRenderer;
                if (!pRenderer)
                    return 0.0f;
                auto fHeight = static_cast<float>(*(uint32_t*)(pRenderer + 0x678));
                return std::max(fHeight / 1080.0f, 1.0f) - 1.0f;
            };

            pattern = hook::pattern("D9 9E 24 02 00 00 D9 EE D8 96 24 02 00 00");
            static auto CursorXHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
            {
                *(float*)(regs.esi + 0x224) += static_cast<float>(*(int32_t*)(regs.esi + 0x100)) * GetCursorSpeedBoost();
            });

            pattern = hook::pattern("D9 9E 28 02 00 00 D8 96 28 02 00 00");
            static auto CursorYHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
            {
                *(float*)(regs.esi + 0x228) += static_cast<float>(*(int32_t*)(regs.esi + 0x104)) * GetCursorSpeedBoost();
            });

            pattern = hook::pattern("D9 05 ? ? ? ? DE D9 DF E0 F6 C4 41 75 49 8B 0D");
            static auto pMapCanvasHeight = *pattern.get_first<float*>(2);
            static auto pMapCanvasWidth = pMapCanvasHeight - 1;

            pattern = hook::pattern("56 D9 58 04 E8 ? ? ? ? 83 C4 14 EB 14");
            static auto DrawMapCircle = (int(__cdecl*)(void*, float, float, float, int))injector::GetBranchDestination(pattern.get_first(4), true).as_int();
            static auto DrawMapCircleHook = [](void* pDrawContext, float fPosX, float fPosY, float fRadius, int nColor) -> int
            {
                auto fWidth = *pMapCanvasWidth;
                auto fHeight = *pMapCanvasHeight;
                if (fWidth > 0.0f && fHeight > 0.0f)
                {
                    float mat0[4] = { 2.0f / fWidth, 0.0f, 0.0f, -1.0f };
                    float mat1[4] = { 0.0f, -2.0f / fHeight, 0.0f, 1.0f };
                    auto vtbl = *(void***)pDrawContext;
                    auto SetViewport = (void(__thiscall*)(void*, float, float, float, float, float, float))vtbl[148 / 4];
                    auto SetShaderConstant = (void(__thiscall*)(void*, int, float*))vtbl[188 / 4];
                    SetViewport(pDrawContext, 0.0f, 0.0f, fWidth, fHeight, 0.0f, 1.0f);
                    SetShaderConstant(pDrawContext, 0, mat0);
                    SetShaderConstant(pDrawContext, 1, mat1);
                }
                return DrawMapCircle(pDrawContext, fPosX, fPosY, fRadius, nColor);
            };
            injector::MakeCALL(pattern.get_first(4), static_cast<int(__cdecl*)(void*, float, float, float, int)>(DrawMapCircleHook), true);

            // This section fixes the audio stuttering that happens sometimes when you close the game
            pattern = hook::pattern("83 EC 2C 8B 0D ? ? ? ? 56 8B 74 24 34 6A 08 8D 44 24 08 50 66 C7 44 24 0E 19 00");
            static auto StopAllSounds = (void(__cdecl*)(uint32_t))pattern.get_first(0);

            pattern = hook::pattern("83 EC 28 80 7C 24 2C 00 56 8B 35 ? ? ? ? 74 2C 6A 00 68");
            static auto EnableAccessibilityShortcuts = (int(__cdecl*)(char))pattern.get_first(0);

            pattern = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 FF 15 ? ? ? ? 8B 35");
            static auto pJobSystem = *pattern.get_first<uintptr_t*>(2);
            static auto pGameWindow = *pattern.get_first<HWND*>(17);

            pattern = hook::pattern("8B C6 5E 88 1D ? ? ? ? C6 05 ? ? ? ? 01 5B");
            static auto pGameIsRunning = *pattern.get_first<uint8_t*>(5);
            static auto CloseWindowHook = safetyhook::create_mid(pattern.get_first(3), [](SafetyHookContext& regs)
            {
                *pGameIsRunning = 0;

                StopAllSounds(0xFFFFFFFF);
                ShowWindow(*pGameWindow, SW_HIDE);
                EnableAccessibilityShortcuts(1);

                if (*pJobSystem)
                {
                    static constexpr auto nWorkerThreads = 7;
                    static constexpr auto nFirstThreadHandleOffset = 0x44;
                    static constexpr auto nWorkerSize = 0x18;
                    HANDLE hThreads[nWorkerThreads];
                    DWORD nCount = 0;
                    for (auto i = 0; i < nWorkerThreads; i++)
                    {
                        auto hThread = *(HANDLE*)(*pJobSystem + nFirstThreadHandleOffset + i * nWorkerSize);
                        if (hThread && GetThreadId(hThread) != GetCurrentThreadId())
                            hThreads[nCount++] = hThread;
                    }
                    if (nCount)
                        WaitForMultipleObjects(nCount, hThreads, TRUE, 1000);
                }

                WFP::onShutdownEvent().executeAll();
                TerminateProcess(GetCurrentProcess(), 0);
            });

            pattern = hook::pattern("83 EC 44 55 56 8B E9 8B 85 04 02 00 00 57 33 F6 83 CF FF");
            static auto RaceHUDHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                struct Element
                {
                    const char* szVisible;
                    bool bVisible = true;
                    float fHideTimer = 0.0f;
                };
                static Element RaceHUD = { "_root.RaceHUD._visible" };
                static Element TimerHUD = { "_root.TimerHUD._visible" };
                static void* pLastMovie = nullptr;
                static constexpr auto fHideDelay = 1.0f;

                auto pMovie = *(void**)(regs.ecx + 0x10);
                if (!pMovie)
                    return;

                if (pMovie != pLastMovie)
                {
                    pLastMovie = pMovie;
                    RaceHUD.bVisible = TimerHUD.bVisible = true;
                    RaceHUD.fHideTimer = TimerHUD.fHideTimer = fHideDelay;
                }

                auto fFrameTime = *(float*)(regs.esp + 0x04);
                auto Update = [&](Element& element, bool bShown)
                {
                    if (bShown)
                        element.fHideTimer = 0.0f;
                    else if (element.bVisible)
                        element.fHideTimer += fFrameTime;

                    auto bVisible = bShown || (element.bVisible && element.fHideTimer < fHideDelay);
                    if (bVisible == element.bVisible)
                        return;

                    struct GFxValue
                    {
                        uint32_t Type = 2;
                        uint32_t Pad = 0;
                        union { double Number; bool Bool; } Value = {};
                    } value;
                    value.Value.Bool = bVisible;

                    auto SetVariable = (bool(__thiscall*)(void*, const char*, GFxValue*, int))(*(void***)pMovie)[44 / 4];
                    SetVariable(pMovie, element.szVisible, &value, 1);
                    element.bVisible = bVisible;
                };

                Update(RaceHUD, *(int32_t*)(regs.ecx + 60) != 0);
                Update(TimerHUD, *(uint8_t*)(regs.ecx + 64) != 0);
            });

            pattern = hook::pattern("8B 55 1C 8D 7E 1C 50 8B CE 89 17 E8");
            static auto CivilianPropMaskHook = safetyhook::create_mid(pattern.get_first(11), [](SafetyHookContext& regs)
            {
                static constexpr uint32_t nHumanDistanceBits = 0x80 | 0x100 | 0x200;
                auto& nMask = *(uint32_t*)(regs.esi + 0x1C);
                if (nMask & nHumanDistanceBits)
                    nMask = (nMask & ~nHumanDistanceBits) | 0x01;
            });

            pattern = hook::pattern("56 50 51 53 8B CF E8 ? ? ? ? 84 C0 0F 85");
            injector::WriteMemoryRaw(pattern.get_first(6), (void*)"\x83\xC4\x10\x32\xC0", 5, true);

            if (bBorderlessWindowed)
            {
                IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                    std::forward_as_tuple("GetSystemMetrics", GetSystemMetrics_Hook),
                    std::forward_as_tuple("CreateWindowExA", CreateWindowExA_Hook),
                    std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
                    std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                    std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                    std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                    std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)
                );
            }

            if (nFPSLimit > 0)
            {
                static float f = 1.0f / static_cast<float>(nFPSLimit);
                auto pattern = hook::pattern("D9 05 ? ? ? ? A2 ? ? ? ? D9 1D ? ? ? ? E8");
                injector::WriteMemory(pattern.get_first(2), &f, true);

                pattern = hook::pattern("8B 54 24 0C 01 56 40 D9 6C 24 04");
                static auto GameClockHook = safetyhook::create_mid(pattern.get_first(4), [](SafetyHookContext& regs)
                {
                    static std::unordered_map<uintptr_t, double> remainders;
                    auto& fRemainder = remainders[regs.esi];
                    auto fMilliseconds = static_cast<double>(*(float*)(regs.esi + 0x18)) * 1000.0 + fRemainder;
                    auto nMilliseconds = static_cast<uint32_t>(fMilliseconds);
                    fRemainder = fMilliseconds - static_cast<double>(nMilliseconds);
                    regs.edx = nMilliseconds;
                });

                static double fDriveSpeedDamping = 0.9990000128746033;
                static double fDriveSpeedDampingSlow = 0.9900000095367432;
                pattern = hook::pattern("D9 C9 DC 0D ? ? ? ? EB 0A DD D9 D9 07 DC 0D ? ? ? ? D9 1F");
                injector::WriteMemory(pattern.get_first(4), &fDriveSpeedDamping, true);
                injector::WriteMemory(pattern.get_first(16), &fDriveSpeedDampingSlow, true);

                pattern = hook::pattern("83 EC 1C D9 EE 56 8B F1 D9 54 24 04 80 BE B8 09 00 00 00");
                static auto BrakeForceHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
                {
                    auto fFrames = static_cast<double>(*(float*)(regs.esp + 0x04)) * 60.0;
                    fDriveSpeedDamping = std::pow(0.9990000128746033, fFrames);
                    fDriveSpeedDampingSlow = std::pow(0.9900000095367432, fFrames);
                });

                static double fMouseAxisScale = 0.04;
                pattern = hook::pattern("D9 86 2C 02 00 00 DD 05 ? ? ? ? DC C9");
                injector::WriteMemory(pattern.get_first(8), &fMouseAxisScale, true);
                static auto MouseLookHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
                {
                    auto fFrameTime = *(float*)(regs.esp + 0x18);
                    if (fFrameTime > 0.0f)
                        fMouseAxisScale = 0.04 * ((1.0 / 60.0) / static_cast<double>(fFrameTime));
                });
            }
        };
    }
} Misc;
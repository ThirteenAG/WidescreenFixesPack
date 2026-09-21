module;

#include <stdafx.h>

export module Misc;

import ComVars;

struct SabWindowedModeWrapper
{
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
};

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

            if (bBorderlessWindowed)
            {
                IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
				    std::forward_as_tuple("GetSystemMetrics", SabWindowedModeWrapper::GetSystemMetrics_Hook),
                    std::forward_as_tuple("CreateWindowExA", SabWindowedModeWrapper::CreateWindowExA_Hook),
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
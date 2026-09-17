#include "stdafx.h"
#include "callargs.h"
#include <d3d9.h>

import PostFX;

void Init()
{
    CIniReader iniReader("");
    bool bWriteSettingsToFile = iniReader.ReadInteger("MAIN", "WriteSettingsToFile", 1) != 0;

    if (bWriteSettingsToFile)
    {
        auto szSettingsSavePath = GetExeModulePath<std::string>() + "savegame\\settings.bin";
        RegistryWrapper("Illusion Softworks", szSettingsSavePath);
        RegistryWrapper::OverrideTypeREG_NONE = REG_BINARY;

        IATHook::Replace(GetModuleHandleA(NULL), "ADVAPI32.DLL",
            std::forward_as_tuple("RegCloseKey", RegistryWrapper::RegCloseKey),
            std::forward_as_tuple("RegCreateKeyA", RegistryWrapper::RegCreateKeyA),
            std::forward_as_tuple("RegOpenKeyA", RegistryWrapper::RegOpenKeyA),
            std::forward_as_tuple("RegOpenKeyExA", RegistryWrapper::RegOpenKeyExA),
            std::forward_as_tuple("RegCreateKeyExA", RegistryWrapper::RegCreateKeyExA),
            std::forward_as_tuple("RegQueryValueExA", RegistryWrapper::RegQueryValueExA),
            std::forward_as_tuple("RegSetValueExA", RegistryWrapper::RegSetValueExA),
            std::forward_as_tuple("RegQueryValueA", RegistryWrapper::RegQueryValueA),
            std::forward_as_tuple("RegDeleteKeyA", RegistryWrapper::RegDeleteKeyA),
            std::forward_as_tuple("RegEnumKeyA", RegistryWrapper::RegEnumKeyA)
        );
    }
}

int WINAPI GetSystemMetricsHook(int nIndex)
{
    return 0;
}

HWND WINAPI CreateWindowExAHook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    auto hwnd = ::CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle & ~WS_OVERLAPPEDWINDOW, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    if (dwStyle != 0x50000000)
    {
        auto [DesktopResW, DesktopResH] = GetDesktopRes();
        tagRECT rect;
        rect.left = (LONG)(((float)DesktopResW / 2.0f) - (nWidth / 2.0f));
        rect.top = (LONG)(((float)DesktopResH / 2.0f) - (nHeight / 2.0f));
        rect.right = nWidth;
        rect.bottom = nHeight;
        SetWindowPos(hwnd, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
    }

    return hwnd;
}

void InitLS3DF()
{
    CIniReader iniReader("");

    if (iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LS3DF"), "8B 35 ? ? ? ? 0F 84 ? ? ? ? 8B 3D ? ? ? ? 55 55 55 55");
        injector::WriteMemory(*pattern.get_first<uint32_t*>(2), GetSystemMetricsHook, true);
        pattern = hook::module_pattern(GetModuleHandle(L"LS3DF"), "FF 15 ? ? ? ? 3B C5 A3 ? ? ? ? 0F 84 ? ? ? ? 50");
        injector::WriteMemory(*pattern.get_first<uint32_t*>(2), CreateWindowExAHook, true);
    }

    // Shadows Z-Fighting
    {
        static std::vector<SafetyHookMid> zbiasHooks;

        std::vector<std::pair<std::string, int>> patterns = {
            { "FF 92 ? ? ? ? A1 ? ? ? ? ? ? 6A ? 6A ? 50 FF 91 ? ? ? ? F6 86", 0 },
            { "FF 92 ? ? ? ? A1 ? ? ? ? ? ? 6A ? 6A ? 50 FF 91 ? ? ? ? 8B 96", 0 },
            { "FF 92 ? ? ? ? ? ? ? ? 56", 0 },
            { "51 6A ? 89 0D ? ? ? ? ? ? 50 FF 92 ? ? ? ? C3", 12 },
            { "FF A2 ? ? ? ? 8B 44 24 ? 39 05 ? ? ? ? 0F 84 ? ? ? ? 8B 0D ? ? ? ? 50 6A ? 6A ? A3 ? ? ? ? ? ? 51 FF 92 ? ? ? ? 5B C2 ? ? 8B 44 24 ? 39 05 ? ? ? ? 0F 84 ? ? ? ? 8B 0D ? ? ? ? 50 6A ? 6A ? A3 ? ? ? ? ? ? 51 FF 92 ? ? ? ? 5B C2 ? ? 8B 44 24 ? 8B 0D", 0 },
        };

        for (const auto& p : patterns)
        {
            auto pattern = hook::module_pattern(GetModuleHandle(L"LS3DF"), p.first);
            zbiasHooks.emplace_back(safetyhook::create_mid(pattern.get_first(p.second), [](SafetyHookContext& regs)
            {
                constexpr auto D3DRS_ZBIAS = 47;
                constexpr auto D3DRS_ZVISIBLE = 30;
                using SetRenderState = HRESULT(__stdcall*)(void* pDevice, DWORD State, DWORD Value);
                auto [pDevice, State, Value] = deduce_args<SetRenderState>(regs);

                if (State != D3DRS_ZBIAS)
                    return;

                if (Value)
                    Value = 16;

                // On real D3D8 the driver biases slope dependently, so ZBIAS alone fixes the flickering.
                // d3d8to9 only turns it into a constant D3DRS_DEPTHBIAS, so set the bias on the D3D9 device
                // ourselves and hand d3d8to9 a state it ignores (ZVISIBLE returns D3D_OK at once), otherwise
                // its own conversion would overwrite the values set here.
                auto pDevice9 = GetDevice9();
                if (!pDevice9)
                    return;

                constexpr float fDepthBiasUnit = -1.0f / ((1 << 20) - 1); // 24-bit depth, same as CalcDepthBias
                const float fDepthBias = Value * fDepthBiasUnit;
                const float fSlopeDepthBias = -0.125f * static_cast<float>(Value); // per ZBIAS unit, -2.0 at 16
                pDevice9->SetRenderState(D3DRS_DEPTHBIAS, *reinterpret_cast<const DWORD*>(&fDepthBias));
                pDevice9->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *reinterpret_cast<const DWORD*>(&fSlopeDepthBias));

                State = D3DRS_ZVISIBLE;
            }));
        }
    }
}

void InitIJoy()
{
    CIniReader iniReader("");
    static float fLeftStickDeadzone = iniReader.ReadFloat("MAIN", "LeftStickDeadzone", 10.0f) / 100.0f;
    static float fRightStickDeadzone = iniReader.ReadFloat("MAIN", "RightStickDeadzone", 10.0f) / 100.0f;
    static constexpr auto pad_max = 10000.0f; //[ -10000 0 10000 ]

    if (fLeftStickDeadzone)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"IJoy"), "DB 06 F6 C1 04 D8 85");
        struct LSHook
        {
            void operator()(injector::reg_pack& regs)
            {
                float x = static_cast<float>(*(int32_t*)(regs.esi + 0x00)) / pad_max;
                float y = static_cast<float>(*(int32_t*)(regs.esi + 0x04)) / pad_max;
                float f = *(float*)(regs.ebp + 0xF38);

                auto len = sqrtf(x * x + y * y);
                if (len < fLeftStickDeadzone)
                {
                    x = 0.0f;
                    y = 0.0f;
                }
                else
                {
                    auto scale = std::min(1.0f, (len - fLeftStickDeadzone) / (1.0f - fLeftStickDeadzone)) / len;
                    x *= scale * pad_max;
                    y *= scale * pad_max;
                };

                *(int32_t*)(regs.esi + 0x00) = static_cast<int32_t>(x);
                *(int32_t*)(regs.esi + 0x04) = static_cast<int32_t>(y);

                _asm {fld  dword ptr[x]}
                _asm {fadd dword ptr[f]}
            }
        }; injector::MakeInline<LSHook>(pattern.get_first(0), pattern.get_first(11)); //0x10002F71
        injector::WriteMemory(pattern.get_first(5), 0x9004C1F6, true); //test cl, 4
    }

    if (fRightStickDeadzone)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"IJoy"), "DB 45 2C F6 C1 04 D8 85");
        struct RSX
        {
            void operator()(injector::reg_pack& regs)
            {
                float x = static_cast<float>(*(int32_t*)(regs.ebp + 0x2C)) / pad_max;
                float y = static_cast<float>(*(int32_t*)(regs.ebp + 0x30)) / pad_max;
                float f = *(float*)(regs.ebp + 0xF38);

                auto len = sqrtf(x * x + y * y);
                if (len < fRightStickDeadzone)
                {
                    x = 0.0f;
                    y = 0.0f;
                }
                else
                {
                    auto scale = std::min(1.0f, (len - fRightStickDeadzone) / (1.0f - fRightStickDeadzone)) / len;
                    x *= scale * pad_max;
                    y *= scale * pad_max;
                };

                *(int32_t*)(regs.ebp + 0x2C) = static_cast<int32_t>(x);
                *(int32_t*)(regs.ebp + 0x30) = static_cast<int32_t>(y);

                _asm {fld  dword ptr[x]}
                _asm {fadd dword ptr[f]}
            }
        }; injector::MakeInline<RSX>(pattern.get_first(0), pattern.get_first(12));
        injector::WriteMemory(pattern.get_first(5), 0x9004C1F6, true); //test cl, 4
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init);
        CallbackHandler::RegisterCallback(L"LS3DF.dll", InitLS3DF);
        CallbackHandler::RegisterCallback(L"LS3DF.dll", InitPostFX);
        CallbackHandler::RegisterCallback(L"IJoy.dll", InitIJoy);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
#include "stdafx.h"
#include "FixSettings.hpp"
#include "NativeMenu.hpp"
#include "callargs.h"
#include <d3d9.h>

import ComVars;

ASI_SETTINGS_EXPORT(FixSettings())
import PostFX;
import RestoredFeatures;
import WidescreenFix;
import Camera;

#include "WindowMode.hpp"

void CheckCompatibility()
{
    std::filesystem::path widescreenASI;
    if (auto hModule = GetModuleHandleW(L"Mafia.WidescreenFix.asi"))
        widescreenASI = GetModulePath(hModule);
    else
    {
        auto gamePath = GetExeModulePath();
        for (const auto& directory : { GetThisModulePath(), gamePath, gamePath / L"scripts", gamePath / L"plugins" })
        {
            auto file = directory / L"Mafia.WidescreenFix.asi";
            std::error_code ec;
            if (std::filesystem::is_regular_file(file, ec))
            {
                widescreenASI = file;
                break;
            }
        }
    }
    if (widescreenASI.empty()) return;

    auto message = L"Mafia Fusion Fix now includes the widescreen fix.\n\nDelete Mafia.WidescreenFix.asi and restart the game:\n\n"
        + widescreenASI.wstring();
    MessageBoxW(nullptr, message.c_str(), L"Mafia Fusion Fix", MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
    TerminateProcess(GetCurrentProcess(), EXIT_FAILURE);
}

char __fastcall SkipLogo(const char*, void*, float, float)
{
    return 1;
}

void InitIntro()
{
    if (!FixSettings().Bool("MAIN.SkipIntro")) return;
    auto logos = hook::pattern("68 00 00 16 44 68 00 00 48 44 B9 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? 68 00 00 16 44 68 00 00 48 44 E8 ? ? ? ? B9 ? ? ? ? 68 00 00 C8 43 68 00 00 30 44 E8 ? ? ? ?");
    if (logos.size() == 1 && std::strcmp(*logos.get_first<const char*>(11), "logo1") == 0 &&
        std::strcmp(*logos.get_first<const char*>(21), "logo3") == 0 && std::strcmp(*logos.get_first<const char*>(41), "logo2") == 0 &&
        injector::GetBranchDestination(logos.get_first(15)) == injector::GetBranchDestination(logos.get_first(35)) &&
        injector::GetBranchDestination(logos.get_first(15)) == injector::GetBranchDestination(logos.get_first(55)))
    {
        for (int offset : { 15, 35, 55 })
            injector::MakeCALL(logos.get_first(offset), SkipLogo, true);
    }
    else
        OutputDebugStringA("Mafia: startup logo patterns unavailable.\n");
}

void Init()
{
    WindowedModeWrapper::EnableDpiAwareness();
    InitIntro();
    InitWidescreenFix();
    InitCamera();
    RestoredFeatures::InitRadar();
    bool bWriteSettingsToFile = FixSettings().Bool("MAIN.PortableSettings") != 0;

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
    MafiaMenu::Init();
}

BOOL WINAPI SetWindowPosHook(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
{
    if (!nWindowMode || window != WindowedModeWrapper::GameHWND ||
        (flags & (SWP_NOMOVE | SWP_NOSIZE)) == (SWP_NOMOVE | SWP_NOSIZE))
        return SetWindowPos(window, after, x, y, width, height, flags);
    WindowedModeWrapper::desiredClientWidth = nWindowWidth;
    WindowedModeWrapper::desiredClientHeight = nWindowHeight;
    WindowedModeWrapper::afterCreateWindow();
    if (nWindowMode == 2)
        WindowedModeWrapper::CenterWindowPosition(nWindowWidth, nWindowHeight);
    return SetWindowPos(window, after, 0, 0, 0, 0, (flags | SWP_NOMOVE | SWP_NOSIZE) & ~SWP_FRAMECHANGED);
}

HWND WINAPI CreateWindowExAHook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    if (nWindowMode && !(dwStyle & WS_CHILD))
    {
        WindowedModeWrapper::bBorderlessWindowed = nWindowMode == 2;
        WindowedModeWrapper::bStretchWindow = nWindowMode == 2;
        WindowedModeWrapper::desiredClientWidth = nWindowWidth;
        WindowedModeWrapper::desiredClientHeight = nWindowHeight;
        auto window = WindowedModeWrapper::CreateWindowExA_Hook(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
        MafiaWindow::Attach(window);
        return window;
    }
    auto window = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    if (window && !(dwStyle & WS_CHILD))
    {
        WindowedModeWrapper::GameHWND = window;
        MafiaWindow::Attach(window);
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static auto setAttribute = []() -> SetAttribute
        {
            auto dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return dwm ? reinterpret_cast<SetAttribute>(GetProcAddress(dwm, "DwmSetWindowAttribute")) : nullptr;
        }();
        int preference = 1;
        if (setAttribute)
            setAttribute(window, 33, &preference, sizeof(preference));
    }
    return window;
}

void InitWindowMode()
{
    auto engine = GetLS3DF();
    auto mode = hook::module_pattern(engine, "F6 05 ? ? ? ? 02 8B 35 ? ? ? ? 0F 84 ? ? ? ? 8B 3D ? ? ? ? 55 55 55 55");
    auto create = hook::module_pattern(engine, "FF 15 ? ? ? ? 3B C5 A3 ? ? ? ? 0F 84 ? ? ? ? 50");
    if (mode.size() != 1 || create.size() != 1)
    {
        OutputDebugStringA("Mafia: native window mode patterns unavailable.\n");
        return;
    }
    nWindowMode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
    static auto parameters = *mode.get_first<uint32_t*>(2);
    CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    FixSettings().OnChange("MAIN.ConfineCursor", []()
    {
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    });
    MafiaWindow::Init(engine, parameters);
    static auto windowMode = safetyhook::create_mid(mode.get_first(), [](SafetyHookContext&)
    {
        auto flags = *parameters;
        flags = nWindowMode ? (flags & ~(2u | 8u | 0x80u)) : (flags | 2u);
        flags = FixSettings().Bool("MAIN.PauseOnFocusLoss") ? flags | 0x20u : flags & ~0x20u;
        ProtectedGameRef<uint32_t>{ parameters } = flags;
        nWindowWidth = static_cast<int>(parameters[-7]);
        nWindowHeight = static_cast<int>(parameters[-6]);
        if (nWindowMode == 2 || nWindowWidth <= 0 || nWindowHeight <= 0 || nWindowWidth > 16384 || nWindowHeight > 16384)
            std::tie(nWindowWidth, nWindowHeight) = GetDesktopRes();
        if (nWindowWidth <= 0 || nWindowHeight <= 0)
            std::tie(nWindowWidth, nWindowHeight) = std::make_tuple(640, 480);
        ProtectedGameRef<uint32_t>{ parameters - 7 } = nWindowWidth;
        ProtectedGameRef<uint32_t>{ parameters - 6 } = nWindowHeight;
        if (nWindowMode == 2 && pResolutionWidth && pResolutionHeight)
        {
            ProtectedGameRef<int32_t>{ pResolutionWidth } = nWindowWidth;
            ProtectedGameRef<int32_t>{ pResolutionHeight } = nWindowHeight;
        }
    });
    FixSettings().OnChange("MAIN.PauseOnFocusLoss", []()
    {
        const bool pause = FixSettings().Bool("MAIN.PauseOnFocusLoss");
        ProtectedGameRef<uint32_t>{ parameters } = pause ? *parameters | 0x20u : *parameters & ~0x20u;
        auto window = WindowedModeWrapper::GameHWND;
        if (!pause && window && GetForegroundWindow() != window)
            SendMessageW(window, WM_ACTIVATE, WA_INACTIVE, 0);
    });
    ProtectedGameRef<decltype(&CreateWindowExAHook)>{ reinterpret_cast<decltype(&CreateWindowExAHook)*>(*create.get_first<uint32_t*>(2)) } = CreateWindowExAHook;
    IATHook::Replace(engine, "USER32.dll", std::forward_as_tuple("SetWindowPos", SetWindowPosHook));
}

void InitLS3DF()
{
    WindowedModeWrapper::EnableDpiAwareness();
    RestoredFeatures::InitProjectorOpacity();

    InitWindowMode();

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
            auto pattern = hook::module_pattern(GetLS3DF(), p.first);
            if (pattern.size() != 1)
                continue;
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
    static float fLeftStickDeadzone = FixSettings().Float("MAIN.LeftStickDeadzone") / 100.0f;
    static float fRightStickDeadzone = FixSettings().Float("MAIN.RightStickDeadzone") / 100.0f;
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
                if (fLeftStickDeadzone >= 1.0f || len <= fLeftStickDeadzone)
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
        ProtectedGameRef<std::remove_cvref_t<decltype(0x9004C1F6)>>{pattern.get_first<std::remove_cvref_t<decltype(0x9004C1F6)>>(5)} = 0x9004C1F6; //test cl, 4
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
                if (fRightStickDeadzone >= 1.0f || len <= fRightStickDeadzone)
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
        ProtectedGameRef<std::remove_cvref_t<decltype(0x9004C1F6)>>{pattern.get_first<std::remove_cvref_t<decltype(0x9004C1F6)>>(5)} = 0x9004C1F6; //test cl, 4
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        FixSettings();
        asi_settings::LiveLink::Start();
        CheckCompatibility();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init);
        CallbackHandler::RegisterCallback(L"rw_data.dll", RestoredFeatures::InitEyeAnimations);
        for (auto name : { L"LS3DF.dll", L"LSV11.dll", L"LSV10.dll" })
        {
            CallbackHandler::RegisterCallback(name, +[]()
            {
                static std::once_flag flag;
                std::call_once(flag, []()
                {
                    InitLS3DF();
                    InitPostFX();
                });
            });
        }
        CallbackHandler::RegisterCallback(L"IJoy.dll", InitIJoy);
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
        CursorConfinement::Shutdown();
    }
    return TRUE;
}

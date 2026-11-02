module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Window;

import ComVars;

int WindowMode = 0;
bool bWindowedMode = false;

void FitFramedClient()
{
    static thread_local bool arranging = false;
    const auto window = WindowedModeWrapper::GameHWND;
    const int width = WindowedModeWrapper::desiredClientWidth;
    const int height = WindowedModeWrapper::desiredClientHeight;
    if (arranging || WindowedModeWrapper::bBorderlessWindowed || !IsWindow(window) || width <= 0 || height <= 0)
        return;
    RECT client{};
    if (!GetClientRect(window, &client) || (client.right == width && client.bottom == height))
        return;
    arranging = true;
    RECT frame{ 0, 0, width, height };
    const auto style = static_cast<DWORD>(GetWindowLongW(window, GWL_STYLE));
    const auto exStyle = static_cast<DWORD>(GetWindowLongW(window, GWL_EXSTYLE));
    WindowedModeWrapper::AdjustFrame(&frame, style, FALSE, exStyle);
    const auto monitor = WindowedModeWrapper::WindowMonitor();
    const int x = monitor.rcMonitor.left + (monitor.rcMonitor.right - monitor.rcMonitor.left - width) / 2;
    const int y = monitor.rcMonitor.top + (monitor.rcMonitor.bottom - monitor.rcMonitor.top - height) / 2;
    SetWindowPos(window, nullptr, x + frame.left, y + frame.top, frame.right - frame.left,
        frame.bottom - frame.top, SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
    arranging = false;
}

BOOL WINAPI SetGameWindowPos(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
{
    const auto result = WindowedModeWrapper::SetWindowPos_Hook(window, after, x, y, width, height, flags);
    if (result && window == WindowedModeWrapper::GameHWND)
        FitFramedClient();
    return result;
}

BOOL WINAPI MoveGameWindow(HWND window, int x, int y, int width, int height, BOOL repaint)
{
    const auto result = WindowedModeWrapper::MoveWindow_Hook(window, x, y, width, height, repaint);
    if (result && window == WindowedModeWrapper::GameHWND)
        FitFramedClient();
    return result;
}

// The engine sizes its render window once at start and only stretches it when the window is resized, so the framed
// window keeps the chosen size (change it in the settings application)
void LockFrame(HWND window)
{
    if (!window || window != WindowedModeWrapper::GameHWND || WindowedModeWrapper::bBorderlessWindowed)
        return;
    const auto style = GetWindowLongW(window, GWL_STYLE);
    if (style & (WS_THICKFRAME | WS_MAXIMIZEBOX))
    {
        SetWindowLongW(window, GWL_STYLE, style & ~(WS_THICKFRAME | WS_MAXIMIZEBOX));
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    FitFramedClient();
}

HWND WINAPI CreateGameWindowA(DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
{
    const auto window = WindowedModeWrapper::CreateWindowExA_Hook(exStyle, className, windowName, style, x, y, width, height, parent, menu, instance, parameter);
    LockFrame(window);
    return window;
}

LONG WINAPI SetGameWindowLongA(HWND window, int index, LONG value)
{
    const auto result = WindowedModeWrapper::SetWindowLongA_Hook(window, index, value);
    if (index == GWL_STYLE)
        LockFrame(window);
    return result;
}

// The resolution chosen in the settings application; the game itself falls back to the nearest display mode
std::pair<int32_t, int32_t> RequestedResolution()
{
    auto [ResX, ResY] = GetDesktopRes();
    HKEY key = nullptr;
    const auto path = Screen.bSignatureEdition ?
        "Software\\Ubisoft\\KingKong\\{111E336D-30BF-4CD4-8D69-4541732AFB27}\\Basic video" :
        "Software\\Ubisoft\\KingKong\\{2C391F94-B8B9-4832-9C57-3AFC332CC037}\\Basic video";
    if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_READ, &key) == ERROR_SUCCESS)
    {
        auto readDimension = [key](const char* name, int32_t fallback)
        {
            DWORD value = 0, type = 0, size = sizeof(value);
            if (RegQueryValueExA(key, name, nullptr, &type, reinterpret_cast<BYTE*>(&value), &size) == ERROR_SUCCESS &&
                type == REG_DWORD && size == sizeof(value) && value > 0 && value <= 16384)
                return static_cast<int32_t>(value);
            return fallback;
        };
        ResX = readDimension("ResolutionWidth", ResX);
        ResY = readDimension("ResolutionHeight", ResY);
        RegCloseKey(key);
    }
    return { ResX, ResY };
}

std::pair<int, int> WindowResolution(int width, int height)
{
    if (WindowMode == 2)
    {
        MONITORINFOEX info = { sizeof(info) };
        auto window = WindowedModeWrapper::GameHWND;
        GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &info);
        DEVMODE mode = { .dmSize = sizeof(mode) };
        if (EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &mode))
        {
            width = static_cast<int>(mode.dmPelsWidth);
            height = static_cast<int>(mode.dmPelsHeight);
        }
    }
    else if (WindowMode == 1)
    {
        // A window can have any size, not only one of the monitor's display modes
        std::tie(width, height) = RequestedResolution();
    }
    if (width <= 0 || height <= 0)
        std::tie(width, height) = GetDesktopRes();
    if (width <= 0 || height <= 0)
        std::tie(width, height) = std::make_tuple(640, 480);
    WindowedModeWrapper::desiredClientWidth = width;
    WindowedModeWrapper::desiredClientHeight = height;
    return std::pair{ width, height };
}

void SetScreenVars(int width, int height)
{
    Screen.Width = width;
    Screen.Height = height;
    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);
    Screen.fHudOffset = (1.0f / (Screen.fHeight * (4.0f / 3.0f))) * ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
    Screen.fHudScale = Screen.fHeight / Screen.fWidth;
    Screen.fTextScale = Screen.fHudScale;
    Screen.fTextBox = 1.0f;
    Screen.fFieldOfView = 1.0f * (((4.0f / 3.0f)) / (Screen.fAspectRatio));
    Screen.fCutOffArea = 0.5f / Screen.fFieldOfView;
    Screen.fFMVScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
}

class Window
{
public:
    Window()
    {
        WFP::onInitEvent() += []()
        {
            WindowMode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
            bWindowedMode = WindowMode != 0;
            WindowedModeWrapper::EnableDpiAwareness();
            WindowedModeWrapper::bBorderlessWindowed = WindowMode == 2;

            if (bWindowedMode)
            {
                IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                    std::forward_as_tuple("CreateWindowExA", CreateGameWindowA),
                    std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
                    std::forward_as_tuple("SetWindowLongA", SetGameWindowLongA),
                    std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                    std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                    std::forward_as_tuple("SetWindowPos", SetGameWindowPos),
                    std::forward_as_tuple("MoveWindow", MoveGameWindow)
                );
            }

            auto pattern = hook::pattern("89 86 BC 02 00 00 89 8E C0 02 00 00 89 96 C4 02 00 00"); //0x9F2161
            if (!Screen.bSignatureEdition) // Gamer's Edition
            {
                struct ResHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        auto [width, height] = WindowResolution(regs.eax, regs.ecx);
                        regs.eax = width;
                        regs.ecx = height;

                        *(uint32_t*)(regs.esi + 0x2BC) = width;
                        *(uint32_t*)(regs.esi + 0x2C0) = height;
                        *(uint32_t*)(regs.esi + 0x2C4) = regs.edx;

                        if (bWindowedMode)
                            *(uint32_t*)(regs.esi + 0x2C8) = 1;

                        SetScreenVars(width, height);
                    }
                }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(18));
            }
            else
            {
                pattern = hook::pattern("8B 46 64 8B 4E 68");
                if (!pattern.empty()) // 9d
                {
                    struct ResHook
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            auto [width, height] = WindowResolution(*reinterpret_cast<int*>(regs.esi + 0x64), *reinterpret_cast<int*>(regs.esi + 0x68));
                            regs.eax = width;
                            regs.ecx = height;
                            *reinterpret_cast<int*>(regs.esi + 0x64) = width;
                            *reinterpret_cast<int*>(regs.esi + 0x68) = height;
                            SetScreenVars(width, height);
                        }
                    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(6));

                    if (bWindowedMode)
                    {
                        pattern = hook::pattern("75 ? 8B 4E ? 8B 56 ? 8D 44 24");
                        injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(0), 0xEB, true);
                    }
                }
                else
                {
                    pattern = hook::pattern("8B 56 68 8B 44 24 44");
                    if (!pattern.empty()) // 9
                    {
                        struct ResHook
                        {
                            void operator()(injector::reg_pack& regs)
                            {
                                regs.eax = *(uint32_t*)(regs.esp + 0x44);

                                auto [width, height] = WindowResolution(*reinterpret_cast<int*>(regs.esi + 0x64), *reinterpret_cast<int*>(regs.esi + 0x68));
                                regs.ecx = width;
                                regs.edx = height;
                                *reinterpret_cast<int*>(regs.esi + 0x64) = width;
                                *reinterpret_cast<int*>(regs.esi + 0x68) = height;
                                SetScreenVars(width, height);
                            }
                        }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(10));

                        pattern = hook::pattern("8B 46 ? 8D 4C 24 ? 51 8B 4E");
                        static auto WindowedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                        {
                            if (bWindowedMode)
                            {
                                *(uint32_t*)(regs.esi + 0x10) = 1;
                                regs.eax = 1;
                            }
                        });
                    }
                    else
                    {
                        pattern = hook::pattern("8B 4E ? 8B 56 ? 53");
                        struct ResHook
                        {
                            void operator()(injector::reg_pack& regs)
                            {
                                auto [width, height] = WindowResolution(*reinterpret_cast<int*>(regs.esi + 0x64), *reinterpret_cast<int*>(regs.esi + 0x68));
                                regs.ecx = width;
                                regs.edx = height;
                                *reinterpret_cast<int*>(regs.esi + 0x64) = width;
                                *reinterpret_cast<int*>(regs.esi + 0x68) = height;
                                SetScreenVars(width, height);
                            }
                        }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(6));

                        pattern = hook::pattern("8B 46 ? 8D 54 24 ? 52 8B 16");
                        static auto WindowedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                        {
                            if (bWindowedMode)
                            {
                                *(uint32_t*)(regs.esi + 0x10) = 1;
                                regs.eax = 1;
                            }
                        });
                    }
                }
            }

            // Windowed mode text fix
            {
                auto [ResX, ResY] = RequestedResolution();
                pattern = hook::pattern("68 E0 01 00 00 68 80 02 00 00");
                (void)pattern.get_first();
                for (size_t i = 0; i < pattern.size(); ++i)
                {
                    injector::WriteMemory<int32_t>(pattern.get(i).get<int32_t>(6), ResX, true);
                    injector::WriteMemory<int32_t>(pattern.get(i).get<int32_t>(1), ResY, true);
                }
            }
        };
    }
} Window;

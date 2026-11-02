module;
#include <stdafx.h>
#include "Shared/CursorConfinement.hpp"
#include "FixSettings.hpp"

export module Window;
import ComVars;


export namespace KnightRiderWindow
{
    inline int Mode = 0;
    inline int Width = 640;
    inline int Height = 480;
    inline SafetyHookMid RequestHook, RendererHook, StartupHook;

    inline void Startup(SafetyHookContext& context)
    {
        const auto application = reinterpret_cast<uint8_t*>(context.ecx);
        application[36] = Mode == 0;
        application[40] = 0;
        *reinterpret_cast<uint32_t*>(application + 44) = 32;
    }

    inline void Renderer(SafetyHookContext& context)
    {
        // Screenshot mode also creates a renderer directly, without the device-picker request.
        *reinterpret_cast<uint32_t*>(context.esp + 4) = Mode == 0;
    }

    inline void Request(SafetyHookContext& context)
    {
        // Select the native device before it creates the parent and child rendering surfaces.
        const auto request = *reinterpret_cast<uint8_t**>(context.esp + 4);
        request[13] = Mode == 0;
        // Windowed DirectDraw surfaces must use the desktop's 32-bit format.
        if (Mode != 0)
            *reinterpret_cast<uint32_t*>(request + 8) = 1;
    }

    inline HWND WINAPI Create(DWORD exStyle, LPCSTR className, LPCSTR title, DWORD style,
        int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        WindowedModeWrapper::EnableDpiAwareness();
        if (style & WS_CHILD)
            return CreateWindowExA(exStyle, className, title, style, x, y, width, height,
                parent, menu, instance, parameter);
        if (Mode != 0)
        {
            WindowedModeWrapper::bBorderlessWindowed = Mode == 2;
            WindowedModeWrapper::bEnableWindowResize = false;
            WindowedModeWrapper::desiredClientWidth = Width;
            WindowedModeWrapper::desiredClientHeight = Height;
            std::tie(x, y, width, height) = WindowedModeWrapper::beforeCreateWindow(Width, Height);
            style = Mode == 2 ? WS_POPUP : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        }
        else
        {
            style &= ~WS_OVERLAPPEDWINDOW;
        }
        const auto window = CreateWindowExA(exStyle, className, title, style, x, y, width, height,
            parent, menu, instance, parameter);
        if (window)
        {
            WindowedModeWrapper::GameHWND = window;
            WindowedModeWrapper::bBorderlessWindowed = Mode != 1;
            WindowedModeWrapper::afterCreateWindow();
            CursorConfinement::Attach(window);
        }
        return window;
    }

    inline BOOL WINAPI Adjust(LPRECT rect, DWORD style, BOOL menu)
    {
        return WindowedModeWrapper::AdjustFrame(rect,
            Mode == 1 ? WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX : style & ~WS_OVERLAPPEDWINDOW,
            menu, 0);
    }

    inline BOOL WINAPI Move(HWND window, int x, int y, int width, int height, BOOL repaint)
    {
        if (Mode != 0 && window == WindowedModeWrapper::GameHWND)
        {
            WindowedModeWrapper::afterCreateWindow();
            return TRUE;
        }
        return MoveWindow(window, x, y, width, height, repaint);
    }

    inline void Init(int width, int height)
    {
        Mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        Width = width;
        Height = height;
        auto request = hook::pattern("83 EC 08 53 55 56 8B F1 57 33 DB 8D BE 90 03 00 00");
        RequestHook = safetyhook::create_mid(request.get_first(), Request);
        auto renderer = hook::pattern("8A 44 24 04 81 EC B4 06 00 00 84 C0 53 55 56 57 8B F1");
        RendererHook = safetyhook::create_mid(renderer.get_first(), Renderer);
        auto startup = hook::pattern("81 EC 00 02 00 00 53 55 8B E9 56 57 8A 45 24 8D 5D 24");
        StartupHook = safetyhook::create_mid(startup.get_first(), Startup);
        auto create = hook::pattern("8B 2D ? ? ? ? 50 68 ? ? ? ? 57 FF D5");
        injector::WriteMemory<decltype(&Create)>(*create.get_first<decltype(&Create)*>(2), Create, true);
        auto adjust = hook::pattern("FF 15 ? ? ? ? A1 ? ? ? ? BB 00 00 00 40 8A 48 24 84 C9");
        injector::WriteMemory<decltype(&Adjust)>(*adjust.get_first<decltype(&Adjust)*>(2), Adjust, true);
        auto move = hook::pattern("FF 15 ? ? ? ? 8B 4C 24 0C 8B 56 14 8B 41 08");
        injector::WriteMemory<decltype(&Move)>(*move.get_first<decltype(&Move)*>(2), Move, true);
    }
}

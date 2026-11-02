module;
#include <stdafx.h>
#include "Shared/CursorConfinement.hpp"
#include "FixSettings.hpp"

export module Window;
import ComVars;


export namespace KnightRider2Window
{
    inline int Mode = 0;
    inline int Width = 640;
    inline int Height = 480;
    inline SafetyHookMid RendererHook, StartupHook;


    inline void Setup(SafetyHookContext&)
    {
        if (Application.is_initialized() && Application.get())
        {
            Application.get()[36] = Mode == 0;
            Application.get()[40] = 0;
            *reinterpret_cast<uint32_t*>(Application.get() + 44) = 32;
        }
    }

    inline void Renderer(SafetyHookContext& context)
    {
        *reinterpret_cast<uint32_t*>(context.esp + 4) = Mode == 0;
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
        auto application = hook::pattern("FF 15 ? ? ? ? A1 ? ? ? ? 8A 48 24 84 C9");
        auto applicationAddress = *application.get_first<uint8_t**>(7);
        if (applicationAddress)
            Application.SetAddress(applicationAddress);
        auto setup = hook::pattern("83 EC 10 8B 15 ? ? ? ? 53 55 56 57 8B F1 8B 46 14 8B 4E 1C 33 FF 89 4C 24 1C");
        StartupHook = safetyhook::create_mid(setup.get_first(), Setup);
        auto renderer = hook::pattern("8A 44 24 04 81 EC B4 06 00 00 53 55 56 84 C0 57 8B F1 BD 7C 00 00 00 8D 84 24 C4 00 00 00");
        RendererHook = safetyhook::create_mid(renderer.get_first(), Renderer);
        auto create = hook::pattern("8B 2D ? ? ? ? 50 68 ? ? ? ? 57 FF D5");
        injector::WriteMemory<decltype(&Create)>(*create.get_first<decltype(&Create)*>(2), Create, true);
        injector::WriteMemory<decltype(&Adjust)>(*application.get_first<decltype(&Adjust)*>(2), Adjust, true);
        IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll", std::forward_as_tuple("MoveWindow", Move));
    }
}

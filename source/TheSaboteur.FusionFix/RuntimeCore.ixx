module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"

export module RuntimeCore;

import ComVars;

export namespace SaboteurRuntimeCore
{
    inline std::atomic<HWND> Window{ nullptr };
    inline std::atomic<int> ClientWidth{ 0 }, ClientHeight{ 0 };
    inline constexpr UINT DpiChanged = 0x02E0;

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
    {
        if (message == DpiChanged && (GetWindowLongW(window, GWL_STYLE) & WS_CAPTION))
        {
            const auto width = ClientWidth.load();
            const auto height = ClientHeight.load();
            if (width > 0 && height > 0 && lParam)
            {
                RECT frame{ 0, 0, width, height };
                const auto suggested = *reinterpret_cast<const RECT*>(lParam);
                const auto style = GetWindowLongW(window, GWL_STYLE);
                const auto extended = GetWindowLongW(window, GWL_EXSTYLE);
                using Adjust = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
                const auto adjust = reinterpret_cast<Adjust>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "AdjustWindowRectExForDpi"));
                const bool sized = adjust ? adjust(&frame, style, GetMenu(window) != nullptr, extended, LOWORD(wParam))
                    : AdjustWindowRectEx(&frame, style, GetMenu(window) != nullptr, extended);
                if (sized)
                {
                    SetWindowPos(window, nullptr, suggested.left, suggested.top, frame.right - frame.left,
                        frame.bottom - frame.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
                    return 0;
                }
            }
        }
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, Procedure, id);
            if (Window.load() == window)
                Window.store(nullptr);
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    inline void Attach(HWND window)
    {
        Window.store(window);
        RECT client{};
        if (GetClientRect(window, &client))
        {
            ClientWidth.store(client.right - client.left);
            ClientHeight.store(client.bottom - client.top);
        }
        SetWindowSubclass(window, Procedure, reinterpret_cast<UINT_PTR>(Procedure), 0);
        CursorConfinement::Attach(window);
    }

    inline void Frame()
    {
        CursorConfinement::Update(Window.load());
        CursorConfinement::Bind();
    }

    inline void Initialize()
    {
        auto& settings = FixSettings();
        CursorConfinement::SetEnabled(settings.Bool("MAIN.ConfineCursor"));
        settings.OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        auto pattern = hook::pattern("80 3D ? ? ? ? 00 75 7D 80 3D ? ? ? ? 00 74 74 83 3D");
        PauseInBackground.SetAddress(*pattern.get_first<uint8_t*>(11));
        PauseInBackground = settings.Bool("MAIN.PauseOnFocusLoss") ? 1 : 0;
        settings.OnChange("MAIN.PauseOnFocusLoss", []()
        {
            PauseInBackground = FixSettings().Bool("MAIN.PauseOnFocusLoss") ? 1 : 0;
        });
        WFP::onEndScene() += Frame;
        WFP::onShutdownEvent() += []()
        {
            CursorConfinement::Shutdown();
        };
    }
}

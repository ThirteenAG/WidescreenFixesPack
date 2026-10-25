#pragma once
#include <d3d9.h>
#include "../../external/FusionFixUtils/include/WindowStyle.hpp"

namespace Driv3rWindow
{
    inline SafetyHookMid modeHook, publicationHook;
    inline void (*onPrepared)(void*) = nullptr;
    inline SafetyHookInline procedureHook;
    inline HWND* windowSlot = nullptr;
    inline uint8_t* renderer = nullptr;
    inline bool ready = false;
    inline int mode = 0, requestedIndex = -1;

    inline HWND Window() { return windowSlot ? *windowSlot : nullptr; }

    inline bool Dimensions(int& width, int& height)
    {
        if (mode == 2)
        {
            const auto rect = WindowStyle::MonitorRect(Window());
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
        }
        return width > 0 && height > 0 && width <= 16384 && height <= 16384;
    }

    inline bool Place(int width, int height)
    {
        const auto window = Window();
        if (!window)
            return false;
        WindowStyle::SquareCorners(window);
        if (!ready || mode == 0 || !Dimensions(width, height))
            return false;
        const DWORD frameBits = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        DWORD style = DWORD(GetWindowLongW(window, GWL_STYLE)) & ~(frameBits | WS_POPUP);
        style |= mode == 2 ? WS_POPUP : WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
        SetWindowLongW(window, GWL_STYLE, LONG(style));
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        WindowStyle::Place(window, width, height);
        return true;
    }

    inline void Request(SafetyHookContext& context)
    {
        if (!ready || mode == 0 || reinterpret_cast<uint8_t*>(context.eax) != renderer)
            return;
        auto index = *reinterpret_cast<const int**>(context.esp + 8);
        if (index) requestedIndex = *index;
        auto requested = *reinterpret_cast<uint8_t**>(context.esp + 12);
        if (requested)
            *requested = 1;
    }

    inline bool Prepare(void* context, int width, int height)
    {
        if (!ready || mode == 0 || context != renderer || !Dimensions(width, height))
            return false;
        auto& present = *reinterpret_cast<D3DPRESENT_PARAMETERS*>(renderer + 0x160);
        // The native windowed branch uses zero dimensions, which mean the
        // client size to D3D9 but cannot be published as the game's resolution.
        present.BackBufferWidth = UINT(width);
        present.BackBufferHeight = UINT(height);
        present.Windowed = TRUE;
        present.FullScreen_RefreshRateInHz = 0;
        *reinterpret_cast<uint8_t*>(renderer + 0x14C) = 1;
        Place(width, height);
        return true;
    }

    inline void Configured(SafetyHookContext& context)
    {
        auto native = reinterpret_cast<void*>(context.ebx);
        if (Prepare(native, *reinterpret_cast<int*>(context.esp + 8), *reinterpret_cast<int*>(context.esp + 12)) && onPrepared)
            onPrepared(native);
    }

    inline bool SynchronizeViewport(void* view, int requestedIndex, int actualIndex, int width, int height, bool built)
    {
        auto bytes = static_cast<uint8_t*>(view);
        auto& currentIndex = *reinterpret_cast<int32_t*>(bytes + 0x1098);
        if (currentIndex == requestedIndex && currentIndex != actualIndex)
        {
            currentIndex = actualIndex;
            *reinterpret_cast<int32_t*>(bytes + 0x105C) = width;
            *reinterpret_cast<int32_t*>(bytes + 0x1060) = height;
        }
        if (!built || width <= 0 || height <= 0)
            return false;
        *reinterpret_cast<int32_t*>(bytes + 0x105C) = width;
        *reinterpret_cast<int32_t*>(bytes + 0x1060) = height;
        return true;
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (ready && mode != 0 && window == Window() && message == WM_CLOSE)
        {
            // The native message pump consumes WM_QUIT and unwinds the loop;
            // WinMain then destroys its own window and releases its services.
            PostQuitMessage(0);
            return 0;
        }
        return procedureHook.stdcall<LRESULT>(window, message, wParam, lParam);
    }

    inline void Init(HWND* nativeWindow, uint8_t* nativeRenderer)
    {
        windowSlot = nativeWindow;
        renderer = nativeRenderer;
        mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        auto builder = hook::pattern("83 EC 10 53 55 8B 6C 24 20 8B D8 8B 45 00 3B 83 50 01 00 00 75 1D 8B 4C 24 24 8A 11 3A 93 4C 01 00 00");
        auto publication = hook::pattern("89 8B 80 01 00 00");
        auto windowed = hook::pattern("89 83 80 01 00 00 EB 2A");
        auto procedure = hook::pattern("8B 44 24 08 8B 54 24 10 83 EC 28 3D 03 02 00 00 53 55 8B 6C 24 3C 56 57 0F 87 A6 02 00 00");
        if (!windowSlot || !renderer || builder.size() != 1 || publication.size() != 2 || windowed.size() != 1 || procedure.size() != 1)
        {
            OutputDebugStringA("Driv3r: window mode contracts unavailable; native mode retained.\n");
            return;
        }
        modeHook = safetyhook::create_mid(builder.get_first(), Request);
        publicationHook = safetyhook::create_mid(windowed.get_first(), Configured);
        procedureHook = safetyhook::create_inline(procedure.get_first(), Procedure);
        if (!modeHook || !publicationHook || !procedureHook)
        {
            modeHook.reset(); publicationHook.reset(); procedureHook.reset();
            OutputDebugStringA("Driv3r: window mode hooks failed; native mode retained.\n");
            return;
        }
        ready = true;
    }
}

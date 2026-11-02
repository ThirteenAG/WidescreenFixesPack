module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include "include/WindowStyle.hpp"
#include "Shared/WindowCursor.hpp"
#include <atomic>
#include <mutex>

export module Window;

import EngineText;
import WindowBindings;

export namespace REWindow
{
    inline SafetyHookMid startupHook;
    inline SafetyHookInline resolutionHook;
    inline std::atomic<int> mode{ 2 };
    inline int windowedMode = 1;
    inline std::atomic<HWND> window{ nullptr };
    inline constexpr wchar_t procedureProperty[] = L"ResidentEvil.FusionFix.Window.Procedure";
    inline std::atomic<int> placeWidth{ 0 }, placeHeight{ 0 };
    inline std::atomic<bool> toggleHeld{ false }, toggleRequested{ false };
    inline std::atomic<int> requestedMode{ -1 };
    inline std::atomic<bool> changingMode{ false };
    inline std::atomic<bool> placementPending{ false };
    inline std::atomic<uint32_t> modeVersion{ 0 };
    inline std::recursive_mutex modeMutex;
    inline int framedWidth = 0, framedHeight = 0;
    inline int lastWidth = 0, lastHeight = 0, lastMode = -1;
    inline bool initialized = false, ready = false;
    inline constexpr UINT placeMessage = WM_APP + 0x4F1;

    inline bool BlocksInput() { return toggleHeld.load() || toggleRequested.load() || requestedMode.load() >= 0 || changingMode.load(); }

    inline BOOL WINAPI NativeClip(const RECT* rect)
    {
        const auto handle = window.load();
        if (!handle) return ClipCursor(rect);
        const std::lock_guard lock(CursorConfinement::Mutex);
        // Native menu frames release their own clip; retain the player's independent confinement choice.
        if (!rect && CursorConfinement::Enabled && !CursorConfinement::Suspended && GetForegroundWindow() == handle)
            return TRUE;
        if (rect && (!CursorConfinement::Enabled || CursorConfinement::Suspended || GetForegroundWindow() != handle))
            return TRUE;
        const auto result = ClipCursor(rect);
        if (result)
        {
            CursorConfinement::Owned = rect && GetClipCursor(&CursorConfinement::OwnedRect);
        }
        return result;
    }

    inline bool ValidSize(int width, int height)
    {
        return width > 0 && height > 0 && width <= 16384 && height <= 16384;
    }

    inline void Dimensions(int& width, int& height)
    {
        if (mode.load() != 2) return;
        const auto monitor = WindowStyle::MonitorRect(window.load());
        width = monitor.right - monitor.left;
        height = monitor.bottom - monitor.top;
    }

    inline bool FullscreenSize(IDirect3DDevice9* device, int* size)
    {
        D3DDEVICE_CREATION_PARAMETERS creation{};
        IDirect3D9* parent = nullptr;
        if (!device || FAILED(device->GetCreationParameters(&creation)) || FAILED(device->GetDirect3D(&parent))) return false;
        D3DDISPLAYMODE desktop{};
        if (FAILED(parent->GetAdapterDisplayMode(creation.AdapterOrdinal, &desktop)))
        {
            parent->Release();
            return false;
        }
        bool supported = false;
        const auto count = std::min<UINT>(4096, parent->GetAdapterModeCount(creation.AdapterOrdinal, desktop.Format));
        for (UINT i = 0; i < count && !supported; ++i)
        {
            D3DDISPLAYMODE candidate{};
            supported = SUCCEEDED(parent->EnumAdapterModes(creation.AdapterOrdinal, desktop.Format, i, &candidate)) &&
                candidate.Width == static_cast<UINT>(size[0]) && candidate.Height == static_cast<UINT>(size[1]);
        }
        parent->Release();
        if (!supported)
        {
            size[0] = static_cast<int>(desktop.Width);
            size[1] = static_cast<int>(desktop.Height);
        }
        return ValidSize(size[0], size[1]);
    }

    inline LRESULT CALLBACK Procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
    {
        const auto previous = reinterpret_cast<WNDPROC>(GetPropW(handle, procedureProperty));
        if (!previous) return DefWindowProcW(handle, message, wParam, lParam);
        if (handle == window.load() && wParam == VK_RETURN &&
            (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP || message == WM_KEYUP) && ((lParam & (1u << 29)) || toggleHeld.load()))
        {
            if (message == WM_SYSKEYUP || message == WM_KEYUP) toggleHeld.store(false);
            else if (!(lParam & (1u << 30)) && GetForegroundWindow() == handle)
            {
                toggleHeld.store(true);
                toggleRequested.store(true);
            }
            return 0;
        }
        if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam))
        {
            toggleHeld.store(false);
            toggleRequested.store(false);
        }
        if (message == placeMessage && handle == window.load())
        {
            const int width = placeWidth.load(), height = placeHeight.load();
            const int selected = mode.load();
            WindowStyle::SquareCorners(handle);
            if (selected && ValidSize(width, height))
            {
                DWORD style = DWORD(GetWindowLongW(handle, GWL_STYLE));
                style &= ~(WS_OVERLAPPEDWINDOW | WS_POPUP | WS_MAXIMIZE);
                style |= selected == 2 ? WS_POPUP : WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
                SetWindowLongW(handle, GWL_STYLE, LONG(style));
                SetWindowPos(handle, nullptr, 0, 0, 0, 0,
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                WindowStyle::Place(handle, width, height);
            }
            else WindowStyle::Unplace(handle);
            WindowCursor::Attach(handle);
            if (static_cast<uint32_t>(wParam) == modeVersion.load()) changingMode.store(false);
            placementPending.store(false);
            return 0;
        }
        CursorConfinement::Message(message, wParam);
        if (message == WM_NCDESTROY) RemovePropW(handle, procedureProperty);
        return CallWindowProcW(previous, handle, message, wParam, lParam);
    }

    inline int ApplyResolution(void* renderer, bool fullscreen, const int* size, bool force, int preferred = -1)
    {
        const std::lock_guard lock(modeMutex);
        const int previous = mode.load();
        const int selected = fullscreen ? 0 : preferred > 0 ? preferred : (previous ? previous : windowedMode);
        if (previous) windowedMode = previous;
        mode.store(selected);
        int dimensions[2]{ size[0], size[1] };
        Dimensions(dimensions[0], dimensions[1]);
        if (!ValidSize(dimensions[0], dimensions[1]))
        {
            mode.store(previous);
            return resolutionHook.thiscall<int>(renderer, fullscreen, size, force);
        }
        const auto wasChanging = changingMode.exchange(true);
        modeVersion.fetch_add(1);
        const int result = resolutionHook.thiscall<int>(renderer, fullscreen, dimensions, force);
        if (result < 0)
        {
            mode.store(previous);
            changingMode.store(wasChanging);
        }
        else if (FixSettings().Int("MAIN.WindowMode") != selected)
            FixSettings().Set("MAIN.WindowMode", std::to_string(selected));
        return result;
    }

    inline int __fastcall ChangeResolution(void* renderer, void*, bool fullscreen, const int* size, bool force)
    {
        return ApplyResolution(renderer, fullscreen, size, force);
    }

    inline void Frame(void* renderer, HWND handle, bool overlay, IDirect3DDevice9* device)
    {
        const std::lock_guard lock(modeMutex);
        if (!ready || !renderer || !handle) return;
        if (window.load() != handle)
        {
            const auto original = GetWindowLongPtrW(handle, GWLP_WNDPROC);
            if (!original || !SetPropW(handle, procedureProperty, reinterpret_cast<HANDLE>(original))) return;
            const auto previous = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(handle, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(Procedure)));
            if (!previous)
            {
                RemovePropW(handle, procedureProperty);
                return;
            }
            window.store(handle);
            lastWidth = lastHeight = 0;
            lastMode = -1;
            initialized = false;
            placementPending.store(false);
        }
        CursorConfinement::Suspend(overlay);
        int width = REText::Read<int>(renderer, 0x433E50);
        int height = REText::Read<int>(renderer, 0x433E54);
        if (!ValidSize(width, height)) return;
        if (overlay) toggleRequested.store(false);
        if ((toggleRequested.load() || requestedMode.load() >= 0) && !(REText::Read<uint32_t>(renderer, 0x433E84) & 5) &&
            device && device->TestCooperativeLevel() == D3D_OK)
        {
            const auto previous = mode.load();
            const auto fromSetting = requestedMode.exchange(-1);
            const auto toggle = toggleRequested.exchange(false);
            const auto selected = fromSetting >= 0 ? fromSetting : previous ? 0 : windowedMode;
            if ((toggle || fromSetting >= 0) && selected != previous)
            {
                if (previous == 1) { framedWidth = width; framedHeight = height; }
                if (previous) windowedMode = previous;
                int requested[2]{ width, height };
                if (selected == 1 && ValidSize(framedWidth, framedHeight))
                {
                    requested[0] = framedWidth;
                    requested[1] = framedHeight;
                }
                if (!selected && !FullscreenSize(device, requested)) return;
                ApplyResolution(renderer, selected == 0, requested, true, selected);
                return;
            }
        }
        if (!initialized)
        {
            initialized = true;
            if (!ValidSize(framedWidth, framedHeight)) { framedWidth = width; framedHeight = height; }
            int requested[2]{ width, height };
            Dimensions(requested[0], requested[1]);
            if (ValidSize(requested[0], requested[1]) && (requested[0] != width || requested[1] != height))
            {
                ChangeResolution(renderer, nullptr, mode.load() == 0, requested, true);
                return;
            }
        }
        // A mode request is only published after the native UI thread has reset its device.
        if (REText::Read<uint32_t>(renderer, 0x433E84) & 5) return;
        const int selected = mode.load();
        RECT client{};
        const bool clientMatches = !selected || (GetClientRect(handle, &client) &&
            client.right - client.left == width && client.bottom - client.top == height);
        if (width == lastWidth && height == lastHeight && selected == lastMode && clientMatches && !changingMode.load()) return;
        placeWidth.store(width);
        placeHeight.store(height);
        if (placementPending.exchange(true)) return;
        if (PostMessageW(handle, placeMessage, modeVersion.load(), 0))
        {
            lastWidth = width;
            lastHeight = height;
            lastMode = selected;
        }
        else placementPending.store(false);
    }

    inline void Install()
    {
        mode.store(std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2));
        windowedMode = mode.load() ? mode.load() : 1;
        WindowCursor::Initialize(FixSettings());
        if (Bind())
        {
            resolutionHook = safetyhook::create_inline(changeResolution, ChangeResolution);
            if (resolutionHook)
                startupHook = safetyhook::create_mid(fullscreenInstruction, [](SafetyHookContext& context)
                {
                    context.eax = (context.eax & ~0xFFu) | (mode.load() == 0 ? 1u : 0u);
                });
            ready = bool(startupHook);
            if (!ready) resolutionHook.reset();
        }
        FixSettings().Availability("MAIN.WindowMode", [](std::string_view) { return ready; });
        FixSettings().Availability("MAIN.ConfineCursor", [](std::string_view value) { return value == "0" || ready; });
        FixSettings().OnChange("MAIN.WindowMode", []()
        {
            requestedMode.store(std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2));
        });
        if (ready)
            IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll", std::forward_as_tuple("ClipCursor", NativeClip));
    }
}

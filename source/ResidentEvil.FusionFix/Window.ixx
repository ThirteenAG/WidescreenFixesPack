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
import LightSync;
import WindowBindings;
import Video;

export namespace REWindow
{
    inline SafetyHookMid startupHook, resetResultHook;
    inline SafetyHookInline resolutionHook, resetDisplayHook;
    inline std::atomic<int> mode{ 2 };
    inline int windowedMode = 1;
    inline std::atomic<HWND> window{ nullptr };
    inline std::mutex windowMutex;
    inline HHOOK attachHook{};
    inline HWND attaching{};
    inline bool attached{};
    inline uint32_t windowGeneration{};
    inline int placeWidth{}, placeHeight{};
    inline std::atomic<bool> toggleHeld{ false }, toggleRequested{ false };
    inline std::atomic<int> requestedMode{ -1 };
    inline std::atomic<int> resettingMode{ -1 };
    inline std::atomic<bool> changingMode{ false };
    inline bool placementPending{};
    inline std::atomic<uint32_t> modeVersion{ 0 };
    inline int framedWidth = 0, framedHeight = 0;
    inline int lastWidth = 0, lastHeight = 0, lastMode = -1;
    inline bool initialized = false;
    inline constexpr UINT placeMessage = WM_APP + 0x4F1;
    inline constexpr UINT ownerMessage = WM_APP + 0x4F2;

    inline bool BlocksInput() { return toggleHeld.load() || toggleRequested.load() || requestedMode.load() >= 0 || resettingMode.load() >= 0 || changingMode.load(); }

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

    inline void Dimensions(int selected, int* size)
    {
        if (selected != 2) return;
        const auto monitor = WindowStyle::MonitorRect(window.load());
        size[0] = monitor.right - monitor.left;
        size[1] = monitor.bottom - monitor.top;
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

    inline void CancelWindowInput()
    {
        toggleHeld.store(false);
        toggleRequested.store(false);
        placementPending = false;
    }

    inline LRESULT CALLBACK Procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (message == WM_NCDESTROY || message == ownerMessage)
        {
            const std::lock_guard lock(windowMutex);
            const bool active = handle == window.load();
            if (message == WM_NCDESTROY || (!active && wParam == 0))
                RemoveWindowSubclass(handle, Procedure, 0);
            if (message == ownerMessage)
                return 0;
            if (active)
            {
                CancelWindowInput();
                window.store(nullptr);
                attached = false;
                ++windowGeneration;
            }
        }
        else if (message == WM_KILLFOCUS || message == WM_ACTIVATEAPP ||
            message == WM_SYSKEYDOWN || message == WM_SYSKEYUP || message == WM_KEYUP)
        {
            const std::lock_guard lock(windowMutex);
            if (handle == window.load())
            {
                if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam))
                {
                    toggleHeld.store(false);
                    toggleRequested.store(false);
                }
                else if (message != WM_ACTIVATEAPP && wParam == VK_RETURN && ((lParam & (1u << 29)) || toggleHeld.load()))
                {
                    if (message != WM_SYSKEYDOWN)
                        toggleHeld.store(false);
                    else if (!(lParam & (1u << 30)) && GetForegroundWindow() == handle)
                    {
                        toggleHeld.store(true);
                        toggleRequested.store(true);
                    }
                    return 0;
                }
            }
        }
        else if (message == placeMessage)
        {
            int width, height;
            const auto generation = static_cast<uint32_t>(lParam);
            {
                const std::lock_guard lock(windowMutex);
                if (handle != window.load() || generation != windowGeneration)
                    return 0;
                width = placeWidth;
                height = placeHeight;
            }
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
            {
                const std::lock_guard lock(windowMutex);
                if (handle == window.load() && generation == windowGeneration)
                {
                    if (static_cast<uint32_t>(wParam) == modeVersion.load())
                        changingMode.store(false);
                    placementPending = false;
                }
            }
            return 0;
        }
        return DefSubclassProc(handle, message, wParam, lParam);
    }

    inline void CancelAttach()
    {
        if (attachHook)
            UnhookWindowsHookEx(attachHook);
        attachHook = nullptr;
        attaching = nullptr;
    }

    inline LRESULT CALLBACK AttachOnOwner(int code, WPARAM remove, LPARAM data)
    {
        const auto message = reinterpret_cast<const MSG*>(data);
        if (code == HC_ACTION && remove == PM_REMOVE && message->message == ownerMessage && message->wParam == 1)
        {
            const std::lock_guard lock(windowMutex);
            if (message->hwnd == attaching)
            {
                CancelAttach();
                if (message->hwnd == window.load())
                    attached = SetWindowSubclass(message->hwnd, Procedure, 0, 0) != FALSE;
            }
        }
        return CallNextHookEx(nullptr, code, remove, data);
    }

    inline bool Follow(HWND handle)
    {
        const std::lock_guard lock(windowMutex);
        if (window.load() != handle)
        {
            const auto previous = window.exchange(handle);
            CancelAttach();
            CancelWindowInput();
            ++windowGeneration;
            attached = false;
            lastWidth = lastHeight = 0;
            lastMode = -1;
            if (previous)
                PostMessageW(previous, ownerMessage, 0, 0);
        }
        if (!attached && attaching != handle)
        {
            const auto owner = GetWindowThreadProcessId(handle, nullptr);
            if (owner == GetCurrentThreadId())
                attached = SetWindowSubclass(handle, Procedure, 0, 0) != FALSE;
            else if (owner)
            {
                attachHook = SetWindowsHookExW(WH_GETMESSAGE, AttachOnOwner, nullptr, owner);
                if (attachHook && PostMessageW(handle, ownerMessage, 1, 0))
                    attaching = handle;
                else
                    CancelAttach();
            }
        }
        return attached;
    }

    inline void PublishMode(int selected)
    {
        const auto value = std::to_string(selected);
        if (requestedMode.load() < 0 && FixSettings().Get("MAIN.WindowMode") != value)
            FixSettings().Set("MAIN.WindowMode", value);
    }

    inline int ApplyResolution(void* renderer, int selected, const int* size, bool force)
    {
        int dimensions[2]{ size[0], size[1] };
        Dimensions(selected, dimensions);
        if (selected == 0 && !FullscreenSize(REText::Read<IDirect3DDevice9*>(renderer, 0x98), dimensions))
        {
            PublishMode(mode.load());
            return D3DERR_INVALIDCALL;
        }
        const int result = resolutionHook.thiscall<int>(renderer, selected == 0,
            ValidSize(dimensions[0], dimensions[1]) ? dimensions : size, force);
        if (result >= 0)
            resettingMode.store(selected);
        else
            PublishMode(mode.load());
        return result;
    }

    inline int __fastcall ChangeResolution(void* renderer, void*, bool fullscreen, const int* size, bool force)
    {
        const int previous = mode.load();
        return ApplyResolution(renderer, fullscreen ? 0 : previous ? previous : windowedMode, size, force);
    }

    inline int __fastcall ResetDisplay(void* renderer, void*)
    {
        // The native setter waits for the window thread. Run it on the game's update thread, never from Present.
        RELightSync::Sample();
        REVideo::Scene();
        if (window.load() && resettingMode.load() < 0 && !(REText::Read<uint32_t>(renderer, 0x433E84) & 5))
        {
            const int previous = mode.load();
            int size[2]{ REText::Read<int>(renderer, 0x433E50), REText::Read<int>(renderer, 0x433E54) };
            if (ValidSize(size[0], size[1]))
            {
                if (previous == 1 || !initialized)
                {
                    framedWidth = size[0];
                    framedHeight = size[1];
                }
                const int setting = requestedMode.exchange(-1);
                const bool toggle = []()
                {
                    const std::lock_guard lock(windowMutex);
                    return toggleRequested.exchange(false);
                }();
                const int selected = setting >= 0 ? setting : toggle ? (previous ? 0 : windowedMode) : previous;
                int target[2]{ size[0], size[1] };
                if (selected == 1 && ValidSize(framedWidth, framedHeight))
                {
                    target[0] = framedWidth;
                    target[1] = framedHeight;
                }
                Dimensions(selected, target);
                if (selected != previous || (!initialized && selected == 2 && (target[0] != size[0] || target[1] != size[1])))
                {
                    if (ValidSize(target[0], target[1]))
                        ApplyResolution(renderer, selected, target, false);
                    else
                        PublishMode(previous);
                }
                initialized = true;
            }
        }
        const int result = resetDisplayHook.thiscall<int>(renderer);
        if (resettingMode.load() >= 0 && !(REText::Read<uint32_t>(renderer, 0x433E84) & 5))
        {
            const int selected = resettingMode.exchange(-1);
            if ((selected == 0) == REText::Read<bool>(renderer, 0x22))
            {
                const int previous = mode.exchange(selected);
                if (selected) windowedMode = selected;
                else if (previous) windowedMode = previous;
                modeVersion.fetch_add(1);
                changingMode.store(true);
            }
            PublishMode(mode.load());
        }
        return result;
    }

    inline void Frame(void* renderer, HWND handle, bool overlay)
    {
        if (!renderer || !handle) return;
        const bool ready = Follow(handle);
        WindowCursor::Follow(handle);
        CursorConfinement::Suspend(overlay);
        if (!ready) return;
        int width = REText::Read<int>(renderer, 0x433E50);
        int height = REText::Read<int>(renderer, 0x433E54);
        if (!ValidSize(width, height)) return;
        const std::lock_guard lock(windowMutex);
        if (handle != window.load()) return;
        if (overlay) toggleRequested.store(false);
        if (resettingMode.load() >= 0 || (REText::Read<uint32_t>(renderer, 0x433E84) & 5)) return;
        const int selected = mode.load();
        RECT client{};
        const bool clientMatches = !selected || (GetClientRect(handle, &client) &&
            client.right - client.left == width && client.bottom - client.top == height);
        if (width == lastWidth && height == lastHeight && selected == lastMode && clientMatches && !changingMode.load()) return;
        placeWidth = width;
        placeHeight = height;
        if (placementPending) return;
        if (PostMessageW(handle, placeMessage, modeVersion.load(), windowGeneration))
        {
            placementPending = true;
            lastWidth = width;
            lastHeight = height;
            lastMode = selected;
        }
    }

    inline void Install()
    {
        BindLiveInt("MAIN.WindowMode", [](int value)
        {
            requestedMode.store(std::clamp(value, 0, 2));
        });
        const int initialMode = requestedMode.exchange(-1);
        mode.store(initialMode);
        windowedMode = initialMode ? initialMode : 1;
        BindLiveInt("MAIN.ConfineCursor", [](int value)
        {
            CursorConfinement::SetEnabled(value != 0);
        });
        Bind();
        resolutionHook = safetyhook::create_inline(changeResolution, ChangeResolution);
        resetDisplayHook = safetyhook::create_inline(resetDisplay, ResetDisplay);
        resetResultHook = safetyhook::create_mid(resetResult, [](SafetyHookContext& context)
        {
            if (FAILED(static_cast<HRESULT>(context.eax)) && resettingMode.exchange(-1) >= 0)
                PublishMode(mode.load());
        });
        startupHook = safetyhook::create_mid(fullscreenInstruction, [](SafetyHookContext& context)
        {
            context.eax = (context.eax & ~0xFFu) | (mode.load() == 0 ? 1u : 0u);
        });
        IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll", std::forward_as_tuple("ClipCursor", NativeClip));
    }
}

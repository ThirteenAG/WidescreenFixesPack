module;

#include <stdafx.h>
#include "Shared/CursorConfinement.hpp"
#include "FixSettings.hpp"
#include <atomic>
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")

export module Overlay;

import Window;

import Input;
import ComVars;

export namespace GodfatherOverlay
{
    asi_overlay::D3D9Overlay Overlay;
    IDirect3DDevice9* CurrentDevice = nullptr;
    LONG HotkeyDown = 0, HotkeyRequested = 0;
    std::atomic<unsigned> nativeHotkey{ 0 };

    HMODULE ThisModule()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ThisModule), &module);
        return module;
    }

    std::string BuildTime()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(ThisModule(), path, DWORD(std::size(path)));
        DWORD ignored = 0;
        const auto size = GetFileVersionInfoSizeW(path, &ignored);
        std::vector<uint8_t> data(size);
        if (!size || !GetFileVersionInfoW(path, 0, size, data.data()))
            return "Unknown";
        wchar_t* value = nullptr;
        UINT length = 0;
        if (!VerQueryValueW(data.data(), L"\\StringFileInfo\\040004b0\\BuildTimeUtc", reinterpret_cast<void**>(&value), &length) || !value)
            return "Unknown";
        return asi_settings::text::Narrow(value);
    }

    bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
        {
            if (message == WM_KEYUP || message == WM_SYSKEYUP)
            {
                if (!(nativeHotkey.load(std::memory_order_relaxed) & 4))
                    InterlockedExchange(&HotkeyDown, 0);
            }
            else if (!(lParam & (1u << 30)) && GetForegroundWindow() == window &&
                !InterlockedCompareExchange(&HotkeyDown, 1, 0))
                InterlockedExchange(&HotkeyRequested, 1);
            result = 0;
            return true;
        }
        if (Overlay.BlocksInput() && (message == WM_INPUT || (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)))
        {
            if (message == WM_MOUSEMOVE)
                return Overlay.Message(window, message, wParam, lParam, result);
            result = message == WM_INPUT ? DefWindowProcA(window, message, wParam, lParam) : 0;
            return true;
        }
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    void Keyboard(HWND window)
    {
        const bool focused = window && GetForegroundWindow() == window;
        if (!focused)
            nativeHotkey.store(0, std::memory_order_relaxed);
        const auto held = nativeHotkey.load(std::memory_order_relaxed);
        const bool down = focused && (((held & 3) && (held & 4)) ||
            ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000)));
        if (down)
        {
            if (!InterlockedCompareExchange(&HotkeyDown, 1, 0))
                InterlockedExchange(&HotkeyRequested, 1);
        }
        else
            InterlockedExchange(&HotkeyDown, 0);
        if (InterlockedExchange(&HotkeyRequested, 0) && window && GetForegroundWindow() == window)
            Overlay.SetOpen(!Overlay.IsOpen());
    }

    bool BlocksInput()
    {
        return Overlay.BlocksInput();
    }

    void Draw(IDirect3DDevice9* device)
    {
        if (!device)
            return;
        static const bool page = []()
        {
            asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
            {
                static const auto buildTime = BuildTime();
                ui.Value("Version", rsc_ProductVersion);
                ui.Value("BuildTimeUtc", buildTime.c_str());
                ui.Value("Graphics API", "Direct3D 9");
                IDirect3DSurface9* surface = nullptr;
                if (CurrentDevice && SUCCEEDED(CurrentDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
                {
                    D3DSURFACE_DESC desc{};
                    surface->GetDesc(&desc);
                    surface->Release();
                    ui.Value("Resolution", std::to_string(desc.Width) + " x " + std::to_string(desc.Height));
                }
            });
            return true;
        }();
        CurrentDevice = device;
        Overlay.Present(device, ThisModule());
    }

    SafetyHookMid inputHook, eventsHook;
    SafetyHookInline pcHook;
    void* nextEvent = nullptr;

    template <typename T> T Read(const void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
    }

    void KeyboardEvent(void* event)
    {
        const auto window = GodfatherWindow::window;
        if (!window || GetForegroundWindow() != window)
            return;
        const auto source = Read<void*>(event, 4);
        if (!source || Read<int>(source, 8) != 1 || Read<int>(event, 12) != 1)
            return;
        const auto action = Read<int>(event, 8);
        if (action != 2 && action != 3)
            return;
        // Native keyboard events use DirectInput scan codes.
        const auto key = Read<unsigned>(event, 16);
        const unsigned bit = key == 0x1D ? 1 : key == 0x9D ? 2 : key == 0x44 ? 4 : 0;
        if (!bit)
            return;
        if (action == 2)
            nativeHotkey.fetch_or(bit, std::memory_order_relaxed);
        else
            nativeHotkey.fetch_and(~bit, std::memory_order_relaxed);
        Keyboard(window);
    }

    void MouseEvent(void* event)
    {
        const auto source = Read<void*>(event, 4);
        if (!source || Read<int>(source, 8) != 2)
            return;
        const auto action = Read<int>(event, 8);
        const auto kind = Read<int>(event, 12);
        const auto button = Read<uint32_t>(event, 16);
        if (kind == 1 && (action == 2 || action == 3) && button < 5)
            Overlay.MouseButton(int(button), action == 2);
        else if (kind == 2 && action == 4)
        {
            const auto table = Read<void**>(source, 0);
            const auto data = reinterpret_cast<void*(__thiscall*)(void*)>(table[1])(source);
            if (data)
                Overlay.MouseWheel(float(Read<int>(data, 8)) / WHEEL_DELTA);
        }
    }

    void Event(SafetyHookContext& context)
    {
        const auto event = reinterpret_cast<void*>(context.eax);
        KeyboardEvent(event);
        if (!BlocksInput())
            return;
        if (Read<int>(event, 8) >= 2)
        {
            MouseEvent(event);
            context.eip = reinterpret_cast<uintptr_t>(nextEvent);
        }
    }

    void __fastcall Poll(void* self, void*, int time)
    {
        pcHook.unsafe_thiscall<void>(self, time);
        if (BlocksInput())
            GodfatherInput::MutePc(self);
    }

    void Filter(SafetyHookContext& context)
    {
        if (BlocksInput())
            GodfatherInput::MutePc(reinterpret_cast<void*>(context.ebx));
    }

    void Install()
    {
        auto input = hook::pattern("8B 83 E0 04 00 00 85 C0 74 0C 8B C8 8B 11 6A 07 83 C3 40 53 FF 12");
        auto events = hook::pattern("8B 48 04 8B 49 08 83 F9 03 75 26");
        auto next = hook::pattern("8B 4B 3C 8B 01 FF 50 18 85 C0 75 A7");
        auto pc = hook::pattern("83 EC 08 53 56 8B F1 66 8B 86 F6 04 00 00 33 DB 66 89 86 F8 04 00 00 38 9E 44 05 00 00");
        if (!GodfatherWindow::createD3DHook)
            return;
        nextEvent = next.get_first();
        inputHook = safetyhook::create_mid(input.get_first(), Filter, SafetyHookMid::StartDisabled);
        eventsHook = safetyhook::create_mid(events.get_first(), Event, SafetyHookMid::StartDisabled);
        pcHook = safetyhook::create_inline(pc.get_first(), Poll, SafetyHookInline::StartDisabled);
        if (!inputHook || !eventsHook || !pcHook || !inputHook.enable() || !eventsHook.enable() || !pcHook.enable())
        {
            inputHook = {};
            eventsHook = {};
            pcHook = {};
            return;
        }
        GodfatherInput::blocked = BlocksInput;
        GodfatherWindow::blocksInput = BlocksInput;
        const auto message = GodfatherWindow::message;
        GodfatherWindow::message = [message](HWND window, UINT id, WPARAM wParam, LPARAM lParam, LRESULT& result)
        {
            const bool overlay = Message(window, id, wParam, lParam, result);
            const bool native = message && message(window, id, wParam, lParam, result);
            return overlay || native;
        };
        const auto reset = GodfatherWindow::beforeReset;
        GodfatherWindow::beforeReset = [reset]()
        {
            Overlay.BeforeReset();
            if (reset)
                reset();
        };
        const auto frame = GodfatherWindow::frame;
        GodfatherWindow::frame = [frame](IDirect3DDevice9* device)
        {
            if (frame)
                frame(device);
            Keyboard(GodfatherWindow::window);
            Draw(device);
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor") && !BlocksInput());
        };
    }
}

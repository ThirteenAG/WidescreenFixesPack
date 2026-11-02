module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "OverlayInput.hpp"
#include "Shared/CursorConfinement.hpp"
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")

export module DeveloperOverlay;

import Bindings;

export namespace SaboteurOverlay
{
    asi_overlay::D3D9Overlay Overlay;
    SafetyHookInline ProcedureHook;
    SafetyHookMid InputHook;
    GameRef<uintptr_t>* Players = nullptr;
    HMODULE Module = nullptr;
    IDirect3DDevice9* Device = nullptr;
    HWND Window = nullptr;
    std::atomic<bool> Hotkey{ false };
    std::atomic<bool> LegacyKeys{ false }, LegacyWheel{ false };
    std::array<bool, 238> PreviousKeys{};

    void NativeInput(uintptr_t player)
    {
        if (!Overlay.context || !Overlay.IsOpen() || !Window || GetForegroundWindow() != Window)
        {
            PreviousKeys.fill(false);
            return;
        }
        // DirectInput's exclusive mouse suppresses Win32 mouse messages. Feed its native
        // client-pixel cursor and physical buttons before the game input cache is cleared.
        asi_overlay::Overlay::Scope scope(Overlay);
        auto& io = ImGui::GetIO();
        const auto x = *reinterpret_cast<const float*>(player + 548);
        const auto y = *reinterpret_cast<const float*>(player + 552);
        if (std::isfinite(x) && std::isfinite(y))
            ImGui_ImplWin32_WndProcHandler(Window, WM_MOUSEMOVE, 0,
                MAKELPARAM(static_cast<SHORT>(x), static_cast<SHORT>(y)));
        const auto buttons = reinterpret_cast<const uint8_t*>(player + 268);
        for (int i = 0; i < 5; ++i)
            io.AddMouseButtonEvent(i, (buttons[i] & 0x80) != 0);
        const auto wheel = *reinterpret_cast<const LONG*>(player + 264);
        if (wheel && !LegacyWheel)
            io.AddMouseWheelEvent(0, float(wheel) / WHEEL_DELTA);
        if (!LegacyKeys)
        {
            const auto keys = reinterpret_cast<const uint8_t*>(player);
            for (unsigned scan = 1; scan < PreviousKeys.size(); ++scan)
            {
                const bool down = (keys[scan] & 0x80) != 0;
                if (down == PreviousKeys[scan])
                    continue;
                PreviousKeys[scan] = down;
                const bool extended = (scan & 0x80) != 0;
                const auto code = scan & 0x7F;
                const auto key = MapVirtualKeyW(code | (extended ? 0xE000 : 0), MAPVK_VSC_TO_VK_EX);
                if (key)
                    ImGui_ImplWin32_WndProcHandler(Window, down ? WM_KEYDOWN : WM_KEYUP, key,
                        1 | (code << 16) | (extended ? 1u << 24 : 0) | (down ? 0 : 3u << 30));
            }
            io.AddKeyEvent(ImGuiMod_Ctrl, (keys[0x1D] | keys[0x9D]) & 0x80);
            io.AddKeyEvent(ImGuiMod_Shift, (keys[0x2A] | keys[0x36]) & 0x80);
            io.AddKeyEvent(ImGuiMod_Alt, (keys[0x38] | keys[0xB8]) & 0x80);
        }
    }

    std::string BuildTime()
    {
        wchar_t path[32768]{};
        GetModuleFileNameW(Module, path, static_cast<DWORD>(std::size(path)));
        DWORD ignored = 0;
        const auto size = GetFileVersionInfoSizeW(path, &ignored);
        std::vector<uint8_t> bytes(size);
        wchar_t* text = nullptr;
        UINT length = 0;
        if (size && GetFileVersionInfoW(path, 0, size, bytes.data()) &&
            VerQueryValueW(bytes.data(), L"\\StringFileInfo\\040004b0\\BuildTimeUtc", reinterpret_cast<void**>(&text), &length) && text)
            return asi_settings::text::Narrow(text);
        return "Unknown";
    }

    LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_MOUSEWHEEL)
            LegacyWheel = true;
        if (message == WM_KEYDOWN || message == WM_KEYUP || message == WM_SYSKEYDOWN || message == WM_SYSKEYUP)
            LegacyKeys = true;
        LRESULT result = 0;
        if (Overlay.Message(window, message, wParam, lParam, result))
            return result;
        if ((message == WM_KEYDOWN || message == WM_KEYUP || message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || Hotkey))
            return 0;
        return ProcedureHook.unsafe_stdcall<LRESULT>(window, message, wParam, lParam);
    }

    void Frame(IDirect3DDevice9* device)
    {
        if (!device || !InputHook || !ProcedureHook)
            return;
        if (Device != device)
        {
            D3DDEVICE_CREATION_PARAMETERS parameters{};
            if (FAILED(device->GetCreationParameters(&parameters)))
                return;
            Window = parameters.hFocusWindow;
            Device = device;
        }
        const auto window = Window;
        const bool down = window && GetForegroundWindow() == window &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
        if (down && !Hotkey)
            Overlay.SetOpen(!Overlay.IsOpen());
        Hotkey = down;
        CursorConfinement::Suspend(Overlay.BlocksInput());
        Overlay.Present(device, Module);
    }

    void BeforeReset()
    {
        Overlay.BeforeReset();
    }

    bool BlocksInput()
    {
        return Overlay.BlocksInput();
    }

    void Install()
    {
        auto pattern = hook::pattern("A1 ? ? ? ? D9 44 24 0C 51 8D 0C 06 D9 1C 24 E8 ? ? ? ? 83 C7 01 81 C6 60 30 00 00");
        Players = SaboteurBindings::Players();
        InputHook = safetyhook::create_mid(pattern.get_first(21), [](SafetyHookContext& context)
        {
            const auto players = Players->get();
            if (players)
            {
                if (context.esi == 0)
                    NativeInput(players);
                SaboteurOverlayInput::Filter(players + context.esi, Overlay.BlocksInput());
            }
        });
        pattern = hook::pattern("8B 44 24 08 53 56 33 DB 33 F6 3D 00 01 00 00");
        ProcedureHook = safetyhook::create_inline(pattern.get_first(), Procedure);
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Install), &Module);
        asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
        {
            static const auto time = BuildTime();
            ui.Value("Version", rsc_ProductVersion);
            ui.Value("BuildTimeUtc", time.c_str());
            ui.Value("Graphics API", "Direct3D 9");
            IDirect3DSurface9* surface = nullptr;
            if (Device && SUCCEEDED(Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
            {
                D3DSURFACE_DESC description{};
                surface->GetDesc(&description);
                surface->Release();
                ui.Value("Resolution", std::to_string(description.Width) + " x " + std::to_string(description.Height));
            }
        });
        WFP::onBeforeReset() += BeforeReset;
        WFP::onShutdownEvent() += []()
        {
            Overlay.ShutdownRenderer();
            Device = nullptr;
        };
    }
}

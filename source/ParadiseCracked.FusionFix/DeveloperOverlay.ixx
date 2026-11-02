module;
#include <stdafx.h>
#include "Shared/RWGTA/UpdateCheck.hpp"
#include <commctrl.h>
#include <windowsx.h>
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"

export module DeveloperOverlay;
import ComVars;
import OverlayD3D7;
import Display;
import Camera;
import Hud;

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "version.lib")

export namespace ParadiseOverlay
{
    inline D3D7Overlay Overlay;
    inline bool HotkeyDown = false;
    inline bool InputSuspended = false;
    inline HWND AttachedWindow = nullptr;

    inline HMODULE Module()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Module), &module);
        return module;
    }

    inline std::string BuildTime()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(Module(), path, DWORD(std::size(path)));
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

    // The game owns mouse and keyboard through exclusive DirectInput: while the overlay (or a notice) needs them,
    // the game's devices are released, so Windows delivers the input to the window instead.
    inline void Suspend(bool suspended)
    {
        if (suspended == InputSuspended)
            return;
        InputSuspended = suspended;
        using Call = void(__thiscall*)(uint8_t*);
        for (auto* device : { &MouseInput, &KeyboardInput })
        {
            if (device->is_initialized() && device->get())
                (*reinterpret_cast<Call**>(device->get()))[suspended ? 4 : 3](device->get());
        }
    }

    inline bool GameKey(uint8_t scan)
    {
        return KeyboardInput.is_initialized() && KeyboardInput.get() && KeyboardInput.get()[532 + scan];
    }

    inline LPARAM OverlayPoint(LPARAM lParam)
    {
        // The overlay is laid out over the window's client area and drawn into the game's picture
        RECT client{};
        GetClientRect(ParadiseDisplay::Window, &client);
        const int width = ParadiseDisplay::Logical.get_ptr()[0], height = ParadiseDisplay::Logical.get_ptr()[1];
        if (client.right <= 0 || client.bottom <= 0 || width <= 0 || height <= 0)
            return lParam;
        const POINT game = ParadiseDisplay::ToGame({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
        const int x = int(std::lround(game.x * double(client.right) / width));
        const int y = int(std::lround(game.y * double(client.bottom) / height));
        return MAKELPARAM(x, y);
    }

    inline LRESULT CALLBACK Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP)
            && wParam == VK_F10)
            return 0;
        if (message == WM_DESTROY)
            RWGTA::UpdateCheck::OnExit();
        if (message == WM_NCDESTROY)
        {
            Overlay.ShutdownRenderer();
            RemoveWindowSubclass(window, Message, 0x5043);
            AttachedWindow = nullptr;
        }
        if (Overlay.IsOpen() && message >= WM_MOUSEMOVE && message <= WM_MBUTTONDBLCLK)
            lParam = OverlayPoint(lParam);
        LRESULT result = 0;
        if (Overlay.Message(window, message, wParam, lParam, result))
            return result;
        return DefSubclassProc(window, message, wParam, lParam);
    }

    inline void Attach(HWND window)
    {
        if (window && window != AttachedWindow && SetWindowSubclass(window, Message, 0x5043, 0))
        {
            AttachedWindow = window;
            WindowCursor::Attach(window);
        }
    }

    inline void ProcessFrame()
    {
        const auto window = ParadiseDisplay::Window;
        ParadiseCamera::Frame();
        ParadiseHud::Frame();
        RWGTA::UpdateCheck::Tick();
        Overlay.Present(ParadiseDisplay::Device.get(), window, Module());
        const bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) || GameKey(0x1D) || GameKey(0x9D);
        const bool key = (GetAsyncKeyState(VK_F10) & 0x8000) || GameKey(0x44);
        const bool down = window && GetForegroundWindow() == window && ctrl && key;
        if (down && !HotkeyDown && Overlay.Available())
            Overlay.SetOpen(!Overlay.IsOpen());
        HotkeyDown = down;
        Suspend(Overlay.BlocksInput());
        CursorConfinement::Suspend(Overlay.BlocksInput());
    }

    inline void Frame()
    {
        static std::once_flag gameInit;
        std::call_once(gameInit, []()
        {
            WFP::onGameInitEvent().executeAll();
        });
        WFP::onGameProcessEvent().executeAll();
    }

    class FrameEvents
    {
    public:
        FrameEvents()
        {
            WFP::onGameProcessEvent() += []()
            {
                ProcessFrame();
            };
        }
    } frameEvents;

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        WindowCursor::Shutdown();
        ExitProcess(code);
    }

    inline void Init()
    {
        WindowCursor::Initialize(FixSettings());
        auto input = hook::pattern("8B 0D ? ? ? ? 8B 01 FF 50 10 8B 0D ? ? ? ? 8B 01 FF 50 10 8B 4E 08");
        MouseInput.SetAddress(*input.get_first<uint8_t**>(2));
        KeyboardInput.SetAddress(*input.get_first<uint8_t**>(13));

        asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
        {
            static const auto build = BuildTime();
            ui.Value("Version", rsc_ProductVersion);
            ui.Value("BuildTimeUtc", build.c_str());
            ui.Value("Graphics API", "Direct3D 7");
            ui.Value("Resolution", std::format("{} x {}", ParadiseDisplay::Logical.get_ptr()[0], ParadiseDisplay::Logical.get_ptr()[1]).c_str());
        });

        RWGTA::UpdateCheck::Config config;
        config.pluginId = "ParadiseCracked.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []()
        {
            return ParadiseDisplay::MenuReady && !ParadiseDisplay::GameRequest && !ParadiseDisplay::MovieFrame
                && !Overlay.IsOpen() && GetForegroundWindow() == ParadiseDisplay::Window;
        };
        config.show = [](const std::wstring& line)
        {
            // The game's own notices are Windows message boxes too
            Suspend(true);
            MessageBoxW(ParadiseDisplay::Window, line.c_str(), L"Paradise Cracked Fusion Fix", MB_OK | MB_ICONINFORMATION);
            Suspend(false);
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));

        ParadiseDisplay::OnFrame = Frame;
        ParadiseDisplay::OnWindow = Attach;
    }
}

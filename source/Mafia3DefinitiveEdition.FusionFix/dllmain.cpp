#include "stdafx.h"
#include "FixSettings.hpp"
#include "RawMouse.hpp"
#include "include/WindowStyle.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

namespace BorderlessWindow
{
    // The game's windowed mode (FULLSCREEN off) is a framed window at the selected resolution. With the option on it
    // is a borderless window covering its monitor at the monitor's resolution instead. The game sets the window's style
    // and size through SetWindowLongA and SetWindowPos whenever the mode or the resolution changes; the window thread
    // applies a change of the option itself.
    HWND window = nullptr;
    bool gameWindowed = false;
    bool applied = false;
    bool placePending = false;
    bool placed = false;
    LONG framedStyle = 0;
    LONG framedWidth = 0;
    LONG framedHeight = 0;
    const int* enabled = nullptr;

    LONG BorderlessStyle(LONG style)
    {
        return (style & ~LONG(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) | WS_POPUP;
    }

    LONG WINAPI SetWindowLongAHook(HWND hWnd, int index, LONG value)
    {
        if (index == GWL_STYLE && hWnd)
        {
            window = hWnd;
            gameWindowed = !(value & WS_POPUP);
            applied = gameWindowed && *enabled;
            if (gameWindowed)
                framedStyle = value;
            if (applied)
                value = BorderlessStyle(value);
            placePending = true;
        }
        return SetWindowLongA(hWnd, index, value);
    }

    BOOL WINAPI SetWindowPosHook(HWND hWnd, HWND insertAfter, int x, int y, int cx, int cy, UINT flags)
    {
        if (hWnd == window && gameWindowed && !(flags & SWP_NOSIZE))
        {
            // The framed window's client size: the game's resolution
            auto frame = WindowStyle::FrameOf(DWORD(framedStyle), DWORD(GetWindowLongA(hWnd, GWL_EXSTYLE)), WindowStyle::DpiOf(hWnd));
            framedWidth = cx - frame.Width();
            framedHeight = cy - frame.Height();
            if (applied)
            {
                auto monitor = WindowStyle::MonitorRect(hWnd);
                x = monitor.left;
                y = monitor.top;
                cx = monitor.right - monitor.left;
                cy = monitor.bottom - monitor.top;
                flags &= ~UINT(SWP_NOMOVE);
            }
            placePending = true;
        }
        return SetWindowPos(hWnd, insertAfter, x, y, cx, cy, flags);
    }

    // Renderer: its resolution list (20-byte entries, width and height first), the selected entry and the display settings
    // with the resolution in use. Borderless renders at the monitor's resolution; the resolution selected before is
    // restored when the option is turned off.
    SafetyHookInline shRendererInit{};
    SafetyHookInline shSetResolution{};
    uint8_t* renderer = nullptr;
    int32_t selectedIndex = -1;
    int32_t forcedIndex = -1;

    int32_t ResolutionIndex(int32_t width, int32_t height)
    {
        auto first = *reinterpret_cast<uint8_t**>(renderer + 0x10);
        auto last = *reinterpret_cast<uint8_t**>(renderer + 0x18);
        for (auto entry = first; entry < last; entry += 20)
        {
            auto size = reinterpret_cast<int32_t*>(entry);
            if (size[0] == width && size[1] == height)
                return int32_t((entry - first) / 20);
        }
        return -1;
    }

    int32_t MonitorResolutionIndex()
    {
        auto monitor = WindowStyle::MonitorRect(window);
        return ResolutionIndex(monitor.right - monitor.left, monitor.bottom - monitor.top);
    }

    bool AtMonitorResolution()
    {
        auto monitor = WindowStyle::MonitorRect(window);
        auto display = *reinterpret_cast<uint8_t**>(renderer + 0xE8);
        return *reinterpret_cast<int32_t*>(display + 0x9C) == monitor.right - monitor.left &&
               *reinterpret_cast<int32_t*>(display + 0xA0) == monitor.bottom - monitor.top;
    }

    // Window creation from the display settings (position, resolution, fullscreen at +94h..+A4h): a borderless start
    // creates the renderer at the resolution of the monitor the window opens on
    SafetyHookInline shCreateWindow{};
    int32_t startupWidth = 0;
    int32_t startupHeight = 0;

    void __fastcall CreateGameWindow(void* app, uint8_t* display)
    {
        auto settings = reinterpret_cast<int32_t*>(display + 0x94);   // x, y, width, height, fullscreen
        if (*enabled && settings[4] == 0)
        {
            RECT rect{ settings[0], settings[1], settings[0] + settings[2], settings[1] + settings[3] };
            MONITORINFO info{ sizeof(info) };
            if (GetMonitorInfoW(MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST), &info))
            {
                startupWidth = settings[2];
                startupHeight = settings[3];
                settings[2] = info.rcMonitor.right - info.rcMonitor.left;
                settings[3] = info.rcMonitor.bottom - info.rcMonitor.top;
            }
        }
        shCreateWindow.unsafe_call(app, display);
    }

    void __fastcall RendererInit(uint8_t* self, void* context)
    {
        shRendererInit.unsafe_call(self, context);
        renderer = self;
    }

    void __fastcall SetResolution(uint8_t* self, int32_t index)
    {
        renderer = self;
        if (applied)
        {
            // A choice from the options: kept for later, the monitor's resolution stays
            int32_t monitorIndex = MonitorResolutionIndex();
            if (monitorIndex >= 0 && index != monitorIndex)
            {
                selectedIndex = index;
                index = monitorIndex;
            }
        }
        shSetResolution.unsafe_call(self, index);
    }

    // Called on the window thread once per frame
    void Frame()
    {
        if (!window)
            return;
        bool want = gameWindowed && *enabled;
        if (want != applied)
        {
            // The option changed: restyle; a framed window at the game's resolution is centered
            applied = want;
            SetWindowLongA(window, GWL_STYLE, (applied ? BorderlessStyle(framedStyle) : framedStyle) | (GetWindowLongA(window, GWL_STYLE) & WS_VISIBLE));
            SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            if (!applied && renderer && selectedIndex < 0 && startupWidth > 0)
                selectedIndex = ResolutionIndex(startupWidth, startupHeight);
            if (!applied && renderer && selectedIndex >= 0)
                shSetResolution.unsafe_call(renderer, selectedIndex);
            if (!applied)
            {
                selectedIndex = -1;
                if (framedWidth > 0 && framedHeight > 0)
                    WindowStyle::Place(window, framedWidth, framedHeight);
            }
            placePending = true;
        }
        if (applied && renderer && !AtMonitorResolution())
        {
            // Once per monitor resolution, in case the game settles on another size
            int32_t monitorIndex = MonitorResolutionIndex();
            if (monitorIndex >= 0 && monitorIndex != forcedIndex)
            {
                if (selectedIndex < 0)
                    selectedIndex = *reinterpret_cast<int32_t*>(renderer + 8);
                forcedIndex = monitorIndex;
                shSetResolution.unsafe_call(renderer, monitorIndex);
                placePending = true;
            }
        }
        if (!applied)
            forcedIndex = -1;
        if (!placePending)
            return;
        placePending = false;
        if (applied)
        {
            auto monitor = WindowStyle::MonitorRect(window);
            WindowStyle::Place(window, monitor.right - monitor.left, monitor.bottom - monitor.top);
            placed = true;
        }
        else if (placed)
        {
            WindowStyle::Unplace(window);
            placed = false;
        }
    }

    void Init()
    {
        enabled = &FixSettings().Int("MAIN.BorderlessWindowed");

        // Window sizes in pixels on monitors with display scaling
        WindowedModeWrapper::EnableDpiAwareness();

        IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
            std::forward_as_tuple("SetWindowLongA", SetWindowLongAHook),
            std::forward_as_tuple("SetWindowPos", SetWindowPosHook));

        auto pattern = hook::pattern("48 8B C4 55 53 56 57 48 8D A8 18 FF FF FF 48 81 EC C8 01 00 00 4C 89 68 10 48 8B F1 4C 89 70 18 45 33 ED");
        shCreateWindow = safetyhook::create_inline(pattern.get_first(), CreateGameWindow);

        pattern = hook::pattern("48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 4C 89 74 24 20 41 57 48 83 EC 40 4C 8B F2 48 8B F9 E8 ? ? ? ? 48 8B C8 4C 8B 00 41 FF 90 ? ? 00 00 4D 8B 06 49 8B CE 4C 8B F8 41 FF 50 08 48 89 87");
        shRendererInit = safetyhook::create_inline(pattern.get_first(), RendererInit);

        pattern = hook::pattern("48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 55 41 54 41 55 41 56 41 57 48 8D 6C 24 C9 48 81 EC A0 00 00 00 8B DA 4C 8B F1 E8");
        shSetResolution = safetyhook::create_inline(pattern.get_first(), SetResolution);
    }
}

namespace HighRateMouse
{
    // The game's DirectInput mouse state (DIMOUSESTATE2)
    struct MouseState
    {
        LONG x;
        LONG y;
        LONG z;
        BYTE buttons[8];
    };

    ptrdiff_t StateOffset = 0;

    SafetyHookInline PollHook{};
    bool __fastcall Poll(uint8_t* device)
    {
        bool ok = PollHook.fastcall<bool>(device);
        if (ok && RawMouse::Active())
        {
            auto totals = RawMouse::Take();
            auto state = reinterpret_cast<MouseState*>(device + StateOffset);
            state->x = totals.x;
            state->y = totals.y;
            state->z = totals.wheel;
            for (int i = 0; i < 8; i++)
                state->buttons[i] = (i < 5 && ((totals.down | totals.pressed) & (1 << i))) ? 0x80 : 0;
        }
        return ok;
    }

    SafetyHookInline RegisterHook{};
    BOOL WINAPI RegisterRawInputDevicesHook(PCRAWINPUTDEVICE devices, UINT count, UINT size)
    {
        if (!devices || size != sizeof(RAWINPUTDEVICE))
            return RegisterHook.stdcall<BOOL>(devices, count, size);
        auto list = RawMouse::Redirect(devices, count);
        BOOL result = RegisterHook.stdcall<BOOL>(list.data(), count, size);
        if (!result && RawMouse::Active())
        {
            RawMouse::Cancel();
            result = RegisterHook.stdcall<BOOL>(devices, count, size);
        }
        return result;
    }

    // The main thread's message pump removes messages until the queue is empty. Every legacy mouse move it removes makes
    // Windows process the input that queued up meanwhile, so with thousands of reports a second the queue rarely runs
    // empty and one pass can take tens of milliseconds. Windows keeps only the newest mouse move in the queue, so
    // ending the pass after one mouse move loses nothing; DirectInput's movement is separate and stays complete.
    BOOL WINAPI PeekMessageOnePass(LPMSG msg, HWND window, UINT filterMin, UINT filterMax, UINT remove)
    {
        static thread_local bool movedThisPass = false;
        static thread_local bool passStart = true;
        if (passStart)
        {
            passStart = false;
            BorderlessWindow::Frame();
        }
        if (movedThisPass)
        {
            movedThisPass = false;
            passStart = true;
            return FALSE;
        }
        BOOL result = PeekMessageA(msg, window, filterMin, filterMax, remove);
        movedThisPass = result && (remove & PM_REMOVE) && msg->message == WM_MOUSEMOVE;
        passStart = !result;
        return result;
    }

    void Init()
    {
        // C_Mouse device poll: GetDeviceState(sizeof(DIMOUSESTATE2), this + state)
        auto pattern = hook::pattern("48 89 5C 24 08 57 48 83 EC 20 33 C0 48 8B F9 48 89 81 ? ? ? ? BA 14 00 00 00");
        StateOffset = *pattern.get_first<int32_t>(18);
        PollHook = safetyhook::create_inline(pattern.get_first(), Poll);

        RawMouse::Start();
        RegisterHook = safetyhook::create_inline(GetProcAddress(GetModuleHandleW(L"user32.dll"), "RegisterRawInputDevices"), RegisterRawInputDevicesHook);

        // Window message pump: its PeekMessageA import slot
        pattern = hook::pattern("C7 44 24 20 01 00 00 00 FF 15 ? ? ? ? 85 C0 0F 84 ? ? ? ? 48 8D 4C 24 ? FF 15 ? ? ? ? 48 8D 4C 24 ? FF 15");
        auto call = pattern.get_first<uint8_t>(8);
        auto slot = reinterpret_cast<void**>(call + 6 + *reinterpret_cast<int32_t*>(call + 2));
        injector::WriteMemory(slot, reinterpret_cast<void*>(&PeekMessageOnePass), true);
    }
}

namespace SkipIntro
{
    // Splash screens state of the boot flow: on entering it shows the middleware logos page, its update then plays the
    // 2K and Hangar 13 logo video and the "do not turn off" save warning before it moves on to the profile loading. The
    // state now shows nothing and moves on at its first update.
    SafetyHookInline shSplashEnter{};
    SafetyHookInline shSplashMessage{};
    int32_t NextStateOffset = 0;

    void __fastcall SplashEnter(void* state)
    {
    }

    void* __fastcall SplashMessage(uint8_t* state, uint8_t* message)
    {
        // message type 0 = update
        if (*reinterpret_cast<int32_t*>(message + 0x10) == 0)
            return *reinterpret_cast<void**>(state + NextStateOffset);
        return shSplashMessage.unsafe_call<void*>(state, message);
    }

    void Init()
    {
        if (!FixSettings().Bool("MAIN.SkipIntro"))
            return;

        auto pattern = hook::pattern("40 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 8B 90 B0 00 00 00 33 C0 48 85 D2 48 8D 8A D0 02 00 00 48 0F 44 C8 45 33 C0 33 D2 48 8B 01 FF 50 18 C6 83");
        shSplashEnter = safetyhook::create_inline(pattern.get_first(), SplashEnter);

        // Message handler: update, or the failure state for message 1 (it follows the next state)
        pattern = hook::pattern("44 8B 42 10 45 85 C0 74 11 41 83 F8 01 74 03 33 C0 C3 48 8B 81 ? 00 00 00 C3 E9");
        NextStateOffset = *pattern.get_first<int32_t>(21) - 8;
        shSplashMessage = safetyhook::create_inline(pattern.get_first(), SplashMessage);
    }
}

void Init()
{
    BorderlessWindow::Init();
    HighRateMouse::Init();
    SkipIntro::Init();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        FixSettings();
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("48 89 5C 24 08 57 48 83 EC 20 33 C0 48 8B F9 48 89 81 ? ? ? ? BA 14 00 00 00"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}

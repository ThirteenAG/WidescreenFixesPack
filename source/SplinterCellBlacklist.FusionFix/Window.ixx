module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SplinterCell/Window.hpp"
#include "Shared/WindowCursor.hpp"
#include <intrin.h>
#include <ShlObj.h>

export module Window;

import ComVars;
import DeveloperOverlay;

// Window modes (UWindowsClient +B4h, WindowStyleFinal in videoSettings.ini): 0 windowed, 1 exclusive fullscreen, 2 borderless.
// The main window is made by UWindowsClient::CreateMainWindow, UWindowsViewport::OpenWindow sets the resolution (UD3DRenderDevice::SetRes)
// and SetRes resizes the window (UWindowsViewport::ResizeViewport). WM_SIZE sets the resolution to the new client size.
// The video options (LeadVideoOptions, 154h bytes) have a resolution for each mode: +84h + 4 * mode width, +94h + 4 * mode height.
// Changes are requested with sub_669D70 and applied by Lead_ApplyVideoOptions in the next frame, only the video options menu saves them.
// The game:
// - makes a window that can't be resized or maximized (only in the editor, sub_8BEDB0: GIsEditor...)
// - puts it at CW_USEDEFAULT, and back where it was when leaving fullscreen
// - makes it as big as the resolution, which can be bigger than the screen (the title bar and the bottom are off screen)
// - lists different resolutions for each mode (windowed: 16:9 ones, borderless: only the desktop) and greys out the list in borderless
// - always uses the desktop resolution in borderless and stretches the window over the monitor
// - goes to windowed when it can't present in exclusive fullscreen (alt-tab, sub_529110) and stays there
// - doesn't save a resized window
// Borderless uses the monitor's resolution for both rendering and the client area. A windowed one is centered (its client area on
// the monitor when it doesn't fit, the frame goes past the edges). Leaving exclusive fullscreen goes back to it when active again.
// The video options are saved whenever they're applied and when the window is resized, the game starts the way it was left.
namespace Window
{
    namespace Options
    {
        constexpr size_t Size = 0x154;
        constexpr ptrdiff_t Mode = 0x74;
        constexpr ptrdiff_t Width = 0x84;
        constexpr ptrdiff_t Height = 0x94;
    }

    uint8_t* pOptions = nullptr; // the applied video options
    void(__cdecl* RequestOptions)(void* options) = nullptr;
    void(__cdecl* SaveOptions)(void* options, int) = nullptr;

    int& OptionsMode(uint8_t* options) { return *reinterpret_cast<int*>(options + Options::Mode); }
    int& OptionsWidth(uint8_t* options, int mode) { return *reinterpret_cast<int*>(options + Options::Width + 4 * mode); }
    int& OptionsHeight(uint8_t* options, int mode) { return *reinterpret_cast<int*>(options + Options::Height + 4 * mode); }

    int lastMode = -1;          // of the last SetRes
    bool fullscreenLost = false; // exclusive fullscreen was left because the window isn't active

    void Subclass();

    SafetyHookInline shInitialRenderer{};
    std::atomic<bool> initialRenderer{ true }, initialSetRes{ true };
    std::atomic<bool> windowReady{ false };
    struct Preferences
    {
        int mode = -1, width = -1, height = -1;
    };
    Preferences initialPreferences;

    void WindowPreferences(int& mode, int& width, int& height)
    {
        wchar_t documents[MAX_PATH]{};
        if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documents)))
            return;
        const auto profile = std::filesystem::path(documents) / L"Ubisoft" / L"Blacklist" / L"videoSettings.ini";
        const auto savedMode = int(GetPrivateProfileIntW(L"WinDrv.WindowsClient", L"WindowStyleFinal", mode, profile.c_str()));
        if (savedMode < Windowed || savedMode > Borderless)
            return;
        mode = savedMode;
        const auto widthKey = mode == Windowed ? L"WindowedViewportX" : mode == Borderless ? L"FakeFullscreenViewportX" : L"FullscreenViewportX";
        const auto heightKey = mode == Windowed ? L"WindowedViewportY" : mode == Borderless ? L"FakeFullscreenViewportY" : L"FullscreenViewportY";
        const auto savedWidth = int(GetPrivateProfileIntW(L"WinDrv.WindowsClient", widthKey, width, profile.c_str()));
        const auto savedHeight = int(GetPrivateProfileIntW(L"WinDrv.WindowsClient", heightKey, height, profile.c_str()));
        if (savedWidth > 0 && savedHeight > 0)
        {
            width = savedWidth;
            height = savedHeight;
        }
    }

    void InitialWindowPreferences(int& mode, int& width, int& height)
    {
        if (initialPreferences.mode >= Windowed && initialPreferences.mode <= Borderless)
        {
            mode = initialPreferences.mode;
            if (initialPreferences.width > 0 && initialPreferences.height > 0)
            {
                width = initialPreferences.width;
                height = initialPreferences.height;
            }
        }
        else
            WindowPreferences(mode, width, height);
    }

    char __cdecl InitialRenderer(uint8_t* options, void* arguments)
    {
        if (options && initialRenderer.exchange(false))
        {
            auto mode = OptionsMode(options);
            if (mode >= Windowed && mode <= Borderless)
            {
                auto width = OptionsWidth(options, mode);
                auto height = OptionsHeight(options, mode);
                InitialWindowPreferences(mode, width, height);
                OptionsMode(options) = mode;
                OptionsWidth(options, mode) = width;
                OptionsHeight(options, mode) = height;
            }
        }
        return shInitialRenderer.ccall<char>(options, arguments);
    }

    // The work area / whole area of the monitor the window is on (the primary one before there's a window)
    MONITORINFO Monitor(HWND window)
    {
        MONITORINFO info = { sizeof(info) };
        auto monitor = window ? MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY) : MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        GetMonitorInfo(monitor, &info);
        return info;
    }

    // The frame around the client area of a window with this style
    RECT Frame(DWORD style, DWORD exStyle)
    {
        return SCWindow::Frame(WindowHandle, style, exStyle);
    }

    // A display mode of the monitor the window is on
    bool IsDisplayMode(int width, int height)
    {
        MONITORINFOEXA info = {};
        info.cbSize = sizeof(info);
        GetMonitorInfoA(MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTOPRIMARY), &info);
        DEVMODEA mode = { .dmSize = sizeof(DEVMODEA) };
        for (DWORD i = 0; EnumDisplaySettingsA(info.szDevice, i, &mode); i++)
        {
            if (int(mode.dmPelsWidth) == width && int(mode.dmPelsHeight) == height)
                return true;
        }
        return false;
    }

    // Centered in the area, the top left corner stays in it
    POINT Centered(const RECT& area, int width, int height)
    {
        return { std::max(area.left, area.left + ((area.right - area.left) - width) / 2),
                 std::max(area.top, area.top + ((area.bottom - area.top) - height) / 2) };
    }

    // Where the windowed window goes: centered in the work area when it fits, otherwise the client area is centered on the monitor
    // and the frame goes past the edges (at the desktop resolution it looks the same as borderless)
    POINT PlaceWindowed(HWND window, int width, int height, const RECT& frame)
    {
        auto info = Monitor(window);
        auto& work = info.rcWork;
        if (width <= work.right - work.left && height <= work.bottom - work.top)
            return Centered(work, width, height);
        auto& area = info.rcMonitor;
        auto clientWidth = width - (frame.right - frame.left);
        auto clientHeight = height - (frame.bottom - frame.top);
        return { area.left + ((area.right - area.left) - clientWidth) / 2 + frame.left,
                 area.top + ((area.bottom - area.top) - clientHeight) / 2 + frame.top };
    }

    // Saves the applied video options, with the mode the player picked
    void Save()
    {
        if (!pOptions || !SaveOptions || !windowReady.load())
            return;
        const auto mode = OptionsMode(pOptions);
        if (mode < Windowed || mode > Borderless || mode != lastMode ||
            OptionsWidth(pOptions, mode) <= 0 || OptionsHeight(pOptions, mode) <= 0)
            return;
        std::vector<uint8_t> options(pOptions, pOptions + Options::Size);
        if (fullscreenLost)
            OptionsMode(options.data()) = ExclusiveFullscreen;
        SaveOptions(options.data(), 0);
    }

    // UD3DRenderDevice::SetRes(viewport, width, height, mode, refresh, adapter)
    SafetyHookInline shSetRes{};
    SafetyHookInline shValidateResolution{};
    void __fastcall ValidateResolution(uintptr_t device, void* edx, uintptr_t viewport, int* width, int* height,
        int* mode, unsigned int* refresh, int* format, int* depth, int* samples, int* aspect)
    {
        const int requestedWidth = *width, requestedHeight = *height;
        const bool windowed = *mode == Windowed && requestedWidth > 0 && requestedHeight > 0;
        shValidateResolution.fastcall(device, edx, viewport, width, height, mode, refresh, format, depth, samples, aspect);
        // A stale graphics profile also replaces valid windowed dimensions with a desktop-derived default.
        if (windowed && *mode == Windowed)
        {
            *width = requestedWidth;
            *height = requestedHeight;
        }
    }

    int __fastcall SetRes(uintptr_t device, void* edx, uintptr_t viewport, int width, int height, int mode, int refresh, int adapter)
    {
        auto newMode = (mode == -1 || mode == 4) ? *reinterpret_cast<int*>(device + 176) : mode;
        if (initialSetRes.exchange(false))
        {
            InitialWindowPreferences(newMode, width, height);
            mode = newMode;
        }
        if (newMode == Borderless)
        {
            auto area = Monitor(WindowHandle).rcMonitor;
            width = area.right - area.left;
            height = area.bottom - area.top;
            if (pOptions)
            {
                OptionsWidth(pOptions, Borderless) = width;
                OptionsHeight(pOptions, Borderless) = height;
            }
        }
        lastMode = newMode;
        auto result = shSetRes.fastcall<int>(device, edx, viewport, width, height, mode, refresh, adapter);
        if (result && viewport)
            windowReady.store(true);
        Subclass();
        return result;
    }

    // D3DVideoRenderer (Bink): the video textures and the buffers the frames are decoded to are as big as the screen (at least 1920x1088),
    // when the device is reset (the window is resized) the textures are made again for the new size, but not the buffers (keepBuffers),
    // the next frame is copied with the new size and overruns them. They're made for the size the buffers have instead.
    SafetyHookInline shCreateVideoTextures{};
    char __cdecl CreateVideoTextures(uintptr_t device, char keepBuffers)
    {
        static int bufferWidth = 0, bufferHeight = 0;
        auto& width = *reinterpret_cast<int*>(device + 8576);
        auto& height = *reinterpret_cast<int*>(device + 8580);
        if (!keepBuffers || !bufferWidth)
        {
            bufferWidth = width;
            bufferHeight = height;
            return shCreateVideoTextures.ccall<char>(device, keepBuffers);
        }
        auto [w, h] = std::pair(width, height);
        width = bufferWidth;
        height = bufferHeight;
        auto result = shCreateVideoTextures.ccall<char>(device, keepBuffers);
        width = w;
        height = h;
        return result;
    }

    // ResizeViewport puts the windowed window back where it was before when the resolution is set (not when the window is resized, WM_SIZE),
    // it's centered
    BOOL WINAPI MoveWindowHook(HWND window, int x, int y, int width, int height, BOOL repaint)
    {
        if (window == WindowHandle)
        {
            SCWindow::SquareCorners(window);
            auto position = PlaceWindowed(window, width, height, Frame(GetWindowLongA(window, GWL_STYLE), GetWindowLongA(window, GWL_EXSTYLE)));
            x = position.x;
            y = position.y;
        }
        return MoveWindow(window, x, y, width, height, repaint);
    }

    // Keep the native monitor placement for borderless.
    BOOL WINAPI SetWindowPosHook(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
    {
        if (window == WindowHandle)
            SCWindow::SquareCorners(window);
        return SetWindowPos(window, after, x, y, width, height, flags);
    }

    // sub_669D70: when the game leaves exclusive fullscreen because the window isn't active, it's kept as the mode to go back to
    SafetyHookInline shRequestOptions{};
    void __cdecl RequestOptionsHook(uint8_t* options)
    {
        if (OptionsMode(options) == Borderless)
        {
            auto area = Monitor(WindowHandle).rcMonitor;
            OptionsWidth(options, Borderless) = area.right - area.left;
            OptionsHeight(options, Borderless) = area.bottom - area.top;
        }
        if (pOptions && lastMode == ExclusiveFullscreen && OptionsMode(options) == Windowed && OptionsMode(pOptions) == ExclusiveFullscreen && GetForegroundWindow() != WindowHandle)
            fullscreenLost = true;
        else if (GetForegroundWindow() == WindowHandle)
            fullscreenLost = false;
        shRequestOptions.ccall(options);
    }

    // sub_670000(lists, mode, a3): the menu's lists for a mode, the first one is the resolutions (width, height, refresh rate)
    struct Resolution { int width, height, refresh; };
    int(__fastcall* AddResolution)(uintptr_t list, void* edx, Resolution* resolution) = nullptr;
    SafetyHookInline shModeLists{};
    int __cdecl ModeLists(uintptr_t lists, int mode, int a3)
    {
        auto result = shModeLists.ccall<int>(lists, mode == Borderless ? Borderless : ExclusiveFullscreen, a3);
        if (mode == Borderless)
            return result;
        if (!pOptions)
            return result;
        for (auto m : { Windowed })
        {
            Resolution resolution = { OptionsWidth(pOptions, m), OptionsHeight(pOptions, m), 0 };
            auto data = *reinterpret_cast<Resolution**>(lists);
            auto count = *reinterpret_cast<int*>(lists + 4);
            if (resolution.width <= 0 || resolution.height <= 0 ||
                std::any_of(data, data + count, [&](const Resolution& r) { return r.width == resolution.width && r.height == resolution.height; }))
                continue;
            AddResolution(lists, nullptr, &resolution);
            data = *reinterpret_cast<Resolution**>(lists);
            count = *reinterpret_cast<int*>(lists + 4);
            std::stable_sort(data, data + count, [](const Resolution& a, const Resolution& b) { return std::tie(a.width, a.height) < std::tie(b.width, b.height); });
        }
        // the display modes are there for each refresh rate, which only exclusive fullscreen picks
        if (mode != ExclusiveFullscreen)
        {
            auto data = *reinterpret_cast<Resolution**>(lists);
            auto& count = *reinterpret_cast<int*>(lists + 4);
            std::stable_sort(data, data + count, [](const Resolution& a, const Resolution& b) { return std::tie(a.width, a.height) < std::tie(b.width, b.height); });
            count = int(std::unique(data, data + count, [](const Resolution& a, const Resolution& b) { return a.width == b.width && a.height == b.height; }) - data);
        }
        return result;
    }

    WNDPROC GameWndProc = nullptr;
    LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        static bool sizing = false;
        if (message == WM_KEYDOWN && wParam == VK_F1 && !(lParam & (1LL << 30)) && NativeMenuOpen.load())
        {
            NativeHelpRequested.store(true);
            return 0;
        }
        LRESULT overlayResult = 0;
        const bool consumed = DeveloperOverlay::Message(window, message, wParam, lParam, overlayResult);
        CursorConfinement::Suspend(DeveloperOverlay::BlocksInput());
        if (consumed)
            return overlayResult;
        auto result = CallWindowProcA(GameWndProc, window, message, wParam, lParam);
        switch (message)
        {
        case WM_GETMINMAXINFO:
            // the windowed window can be bigger than the screen with its frame (at the desktop resolution)
            reinterpret_cast<MINMAXINFO*>(lParam)->ptMaxTrackSize = { GetSystemMetrics(SM_CXVIRTUALSCREEN) * 2, GetSystemMetrics(SM_CYVIRTUALSCREEN) * 2 };
            break;
        case WM_ACTIVATEAPP:
            // back to exclusive fullscreen
            if (wParam && fullscreenLost && pOptions)
            {
                std::vector<uint8_t> options(pOptions, pOptions + Options::Size);
                OptionsMode(options.data()) = ExclusiveFullscreen;
                fullscreenLost = false;
                RequestOptions(options.data());
            }
            break;
        case WM_ENTERSIZEMOVE:
            sizing = true;
            break;
        case WM_EXITSIZEMOVE:
            sizing = false;
            [[fallthrough]];
        case WM_SIZE:
            // the windowed resolution is the client size the window was resized to
            if (!sizing && pOptions && lastMode == Windowed && OptionsMode(pOptions) == Windowed && !IsIconic(window) && !IsZoomed(window) &&
                (message == WM_EXITSIZEMOVE || wParam == SIZE_RESTORED))
            {
                RECT client = {};
                GetClientRect(window, &client);
                auto width = int(client.right - client.left);
                auto height = int(client.bottom - client.top);
                if (width > 0 && height > 0 && (width != OptionsWidth(pOptions, Windowed) || height != OptionsHeight(pOptions, Windowed)))
                {
                    std::vector<uint8_t> options(pOptions, pOptions + Options::Size);
                    OptionsWidth(options.data(), Windowed) = width;
                    OptionsHeight(options.data(), Windowed) = height;
                    RequestOptions(options.data());
                }
            }
            break;
        }
        return result;
    }

    void Subclass()
    {
        static HWND subclassed = nullptr;
        if (!WindowHandle || subclassed == WindowHandle || !IsWindow(WindowHandle))
            return;
        GameWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WndProc)));
        subclassed = WindowHandle;
        WindowCursor::Attach(WindowHandle);

        // the windowed window can open behind other windows, it's brought to the front once
        if (lastMode == Windowed)
        {
            ShowWindow(WindowHandle, SW_SHOW);
            SetForegroundWindow(WindowHandle);
            BringWindowToTop(WindowHandle);
        }
    }
}

export void InitWindow()
{
    SCWindow::EnableDpi();
    WindowCursor::Initialize(FixSettings());
    using namespace Window;

    WindowPreferences(initialPreferences.mode, initialPreferences.width, initialPreferences.height);

    // Bootstrap creates a device before the normal video preferences are loaded.
    auto initial = find_pattern(
        "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 81 EC 5C 01 00 00 A1 ? ? ? ? 33 C5 89 45 F0 53 56 57 50 8D 45 F4 64 A3 00 00 00 00 8B 45 0C 8B 75 08 33 DB 68 ? ? ? ? 89 75 B4 89 45 C8",
        "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 81 EC 4C 01 00 00 A1 ? ? ? ? 33 C5 89 45 F0 53 56 57 50 8D 45 F4 64 A3 00 00 00 00 8B 45 0C 8B 75 08 33 DB 68 ? ? ? ? 89 75 B4 89 45 C8");
    shInitialRenderer = safetyhook::create_inline(initial.get_first(), InitialRenderer);

    // Resizable window with a maximize box: the editor style check in CreateMainWindow, OpenWindow, ResizeViewport and EndFullscreen
    auto pattern = hook::pattern("BE 00 00 CF 10 E8 ? ? ? ? 85 C0 75 05 BE 00 00 CA 10");
    injector::WriteMemory<uint8_t>(pattern.get_first(12), 0xEB, true); // jnz -> jmp

    for (const auto& [signature, offset] :
        { std::pair{ "8B 8E 2C 08 00 00 8B 51 04 8B 1D ? ? ? ? 83 E0 F7 50 6A EC 52 FF D3 81 E7 FF FF 30 7F 81 CF 00 00 CF 00 E8 ? ? ? ? 85 C0 75 ? 81 E7 FF FF FA FF", 43 },
          std::pair{ "FF 86 3C 08 00 00 8B 51 04 8B 1D ? ? ? ? 83 E0 F7 50 6A EC 52 FF D3 81 E7 FF FF 30 7F 81 CF 00 00 CF 00 E8 ? ? ? ? 85 C0 75 ? 81 E7 FF FF FA FF", 43 },
          std::pair{ "BF 00 00 CF 00 89 7D DC E8 ? ? ? ? 85 C0 75 ? 81 E7 FF FF FA FF", 15 } })
        injector::WriteMemory<uint8_t>(hook::pattern(signature).get_first(offset), 0xEB, true); // jnz -> jmp

    // CreateMainWindow: x, y (CW_USEDEFAULT windowed) and the window size in eax, ecx, the client at [ebp-14h]
    pattern = hook::pattern("BA 00 00 00 80 8B FA 8B 1D");
    static auto CreateMainWindowPosition = safetyhook::create_mid(pattern.get_first(7), [](SafetyHookContext& regs)
    {
        auto client = *reinterpret_cast<uintptr_t*>(regs.ebp - 0x14);
        if (*reinterpret_cast<int*>(client + 0xB4) != Windowed)
            return;
        auto position = PlaceWindowed(nullptr, regs.eax, regs.ecx, Frame(WS_OVERLAPPEDWINDOW, 0));
        regs.edx = position.x;
        regs.edi = position.y;
    });

    pattern = find_pattern("55 8B EC 83 EC 2C 83 7D 0C 00 53 56 57 8B F1 7F 09 8B 86 B4 00 00 00", "55 8B EC 83 EC 2C A1 ? ? ? ? 33 C5 89 45 FC 83 7D 0C 00 8B 45 08 53");
    shSetRes = safetyhook::create_inline(pattern.get_first(), SetRes);

    pattern = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B D9 89 5D F0 33 F6 39 35 ? ? ? ?");
    shValidateResolution = safetyhook::create_inline(pattern.get_first(), ValidateResolution);

    IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
        std::forward_as_tuple("MoveWindow", MoveWindowHook),
        std::forward_as_tuple("SetWindowPos", SetWindowPosHook)
    );
    CursorConfinement::OriginalClip.store(ClipCursor);
    IATHook::Replace(GetModuleHandleW(nullptr), "USER32.DLL",
        std::forward_as_tuple("ClipCursor", CursorConfinement::NativeClip));

    pattern = find_pattern("55 8B EC 83 EC 08 80 3D ? ? ? ? 00 57 0F 85 ? ? ? ? E8", "55 8B EC 83 EC 14 80 3D ? ? ? ? 00 53 56 57 0F 85");
    shCreateVideoTextures = safetyhook::create_inline(pattern.get_first(), CreateVideoTextures);

    // The applied video options (copied by sub_669D00) and the videoSettings.ini writer (sub_66AF40, called by the video options menu)
    pattern = hook::pattern("B9 55 00 00 00 BE ? ? ? ? 8B F8 F3 A5");
    pOptions = *pattern.get_first<uint8_t*>(6);

    pattern = hook::pattern("89 B3 10 02 00 00 E8 ? ? ? ? 8D 8D ? ? ? ? 56 51 E8");
    if (!pattern.empty())
        SaveOptions = reinterpret_cast<decltype(SaveOptions)>(injector::GetBranchDestination(pattern.get_first(19)).as_int());
    else
    {
        pattern = hook::pattern("89 B3 10 02 00 00 E8 ? ? ? ? 8D 4C 24 ? 56 51 E8");
        SaveOptions = reinterpret_cast<decltype(SaveOptions)>(injector::GetBranchDestination(pattern.get_first(17)).as_int());
    }

    pattern = hook::pattern("55 8B EC 83 3D ? ? ? ? 00 75 ? 83 3D ? ? ? ? 00 56 8B 75 08 74");
    RequestOptions = reinterpret_cast<decltype(RequestOptions)>(pattern.get_first());
    shRequestOptions = safetyhook::create_inline(pattern.get_first(), RequestOptionsHook);
    RequestOptions = shRequestOptions.original<decltype(RequestOptions)>();

    // Lead_ApplyVideoOptions: the options were applied and copied to the applied ones
    pattern = hook::pattern("B9 55 00 00 00 8B F3 BF ? ? ? ? F3 A5");
    static auto OptionsApplied = safetyhook::create_mid(pattern.get_first(14), [](SafetyHookContext& regs)
    {
        Subclass();
        Save();
    });

    // Keep the native desktop-only list for borderless. Windowed lists also include a custom client size.
    pattern = hook::pattern("51 8B 4D 08 89 55 E0 89 45 E4 E8");
    AddResolution = reinterpret_cast<decltype(AddResolution)>(injector::GetBranchDestination(pattern.get_first(10)).as_int());
    pattern = hook::pattern("55 8B EC 83 EC 30 53 33 DB 56 57 8B 7D 08 89 5F 04");
    shModeLists = safetyhook::create_inline(pattern.get_first(), ModeLists);

    // Changing the mode in the menu (sub_1ACAC30) keeps the resolution instead of using the mode's default one (edx, eax),
    // exclusive fullscreen only when the display has it. The menu's resolution is at +20h of the menu options (esi) in the DX11 exe,
    // in globals in the DX9 one, the new mode is edi / esi.
    static bool modeInEdi = false;
    pattern = hook::pattern("89 56 20 89 46 24 83 FF 01");
    if (!pattern.empty())
    {
        modeInEdi = true;
        injector::MakeNOP(pattern.get_first(), 6, true);
    }
    else
    {
        pattern = hook::pattern("89 15 ? ? ? ? A3 ? ? ? ? 83 FE 01 75");
        MenuWidth.SetAddress(*pattern.get_first<int*>(2));
        MenuHeight.SetAddress(*pattern.get_first<int*>(7));
        injector::MakeNOP(pattern.get_first(), 11, true);
    }
    static auto MenuModeResolution = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto& width = MenuWidth.is_initialized() ? MenuWidth.get() : *reinterpret_cast<int*>(regs.esi + 0x20);
        auto& height = MenuHeight.is_initialized() ? MenuHeight.get() : *reinterpret_cast<int*>(regs.esi + 0x24);
        auto mode = int(modeInEdi ? regs.edi : regs.esi);
        if (mode == Borderless)
        {
            auto area = Monitor(WindowHandle).rcMonitor;
            width = area.right - area.left;
            height = area.bottom - area.top;
        }
        else if (mode == ExclusiveFullscreen && !IsDisplayMode(width, height))
        {
            width = int(regs.edx);
            height = int(regs.eax);
        }
    });

    // HWND
    pattern = hook::pattern("8B 8E ? ? ? ? 8B 41 04 85 C0");
    static auto GetHWND = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WindowHandle = (HWND)regs.eax;
    });

    // WindowStyle
    pattern = hook::pattern("83 3D ? ? ? ? ? 0F 85 ? ? ? ? A1 ? ? ? ? 3B 46 08");
    WindowStyle.SetAddress(*pattern.get_first<int*>(2));

    // Resolution
    pattern = hook::pattern("A3 ? ? ? ? 8B 8E ? ? ? ? 89 0D ? ? ? ? 85 DB 74 13 83 BE");
    if (!pattern.empty())
    {
        ViewportResolutionWidth.SetAddress(*pattern.get_first<int*>(1));
        ViewportResolutionHeight.SetAddress(*pattern.get_first<int*>(13));
    }
    else
    {
        pattern = hook::pattern("89 0D ? ? ? ? 8B 96 ? ? ? ? 89 15 ? ? ? ? 85 DB 74 13");
        ViewportResolutionWidth.SetAddress(*pattern.get_first<int*>(2));
        ViewportResolutionHeight.SetAddress(*pattern.get_first<int*>(14));
    }
}

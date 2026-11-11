module;

#include <stdafx.h>
#include <dshow.h>
#include "FixSettings.hpp"

#pragma comment(lib, "strmiids.lib")

export module Video;

import ComVars;
import input;
import SettingsFile;
import Window;

// Movies (intro.mpg and the others played through DirectShow) and the startup window: whole movie at its own aspect,
// windows placed for the window mode

namespace MaxPayneVideo
{
    HWND Window = nullptr;
    IVideoWindow* Renderer = nullptr;
    IBasicVideo* Video = nullptr;
    RECT LastClient{};
    SafetyHookMid PrepareHook, BeginHook, EndHook;
    HWND StartupWindow = nullptr;
    bool StartupSizing = false;
    decltype(&CreateWindowExA) NativeCreateWindow = CreateWindowExA;
    decltype(&ShowWindow) NativeMfcShowWindow = ShowWindow;
    decltype(&SetWindowPos) NativeSetWindowPos = SetWindowPos;
    decltype(&CreateWindowExW) NativeMovieCreateWindow = CreateWindowExW;
    decltype(&ShowWindow) NativeMovieShowWindow = ShowWindow;
    decltype(&SetWindowPos) NativeMovieSetWindowPos = SetWindowPos;
    bool MovieReady = false;

    int SavedSize(const char* name, int fallback)
    {
        if (FixSettings().Bool("MAIN.PortableSettings") && !CWinApp::SettingsPath.empty())
            return static_cast<int>(
                GetPrivateProfileIntA("Video Settings", name, fallback, CWinApp::SettingsPath.string().c_str()));
        DWORD value = fallback;
        DWORD size = sizeof(value);
        RegGetValueA(HKEY_CURRENT_USER, "Software\\Remedy Entertainment\\Max Payne\\Video Settings", name,
                     RRF_RT_REG_DWORD, nullptr, &value, &size);
        return static_cast<int>(value);
    }

    bool IsStartupWindow(HWND window)
    {
        if (!StartupSizing || !WindowMode::firstMode || !window || GetParent(window))
            return false;
        if (window == StartupWindow)
            return true;
        char title[128]{}, className[64]{};
        GetWindowTextA(window, title, sizeof(title));
        GetClassNameA(window, className, sizeof(className));
        return _strnicmp(title, "Max Payne", 9) == 0 && strcmp(className, "#32770") != 0;
    }

    void FitStartupWindow(HWND window)
    {
        const int width = std::clamp(SavedSize("Display Width", 640), 640, 32768);
        const int height = std::clamp(SavedSize("Display Height", 480), 480, 32768);
        auto [x, y, frameWidth, frameHeight] = WindowMode::GetCenteredWindowRect(window, width, height);
        ::SetWindowPos(window, nullptr, x, y, width + frameWidth, height + frameHeight,
                       SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    HWND WINAPI CreateStartupWindow(DWORD extended, LPCSTR className, LPCSTR title, DWORD style,
        int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        const bool game = StartupSizing && !parent && !(style & WS_CHILD) && title && !IS_INTRESOURCE(title) &&
                          _strnicmp(title, "Max Payne", 9) == 0 && className && !IS_INTRESOURCE(className) &&
                          strcmp(className, "#32770") != 0;
        if (game)
        {
            const int clientWidth = std::clamp(SavedSize("Display Width", 640), 640, 32768);
            const int clientHeight = std::clamp(SavedSize("Display Height", 480), 480, 32768);
            RECT frame{ 0, 0, clientWidth, clientHeight };
            using AdjustForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
            using GetDpi = UINT(WINAPI*)();
            const auto user32 = GetModuleHandleW(L"user32.dll");
            const auto adjust = reinterpret_cast<AdjustForDpi>(GetProcAddress(user32, "AdjustWindowRectExForDpi"));
            const auto getDpi = reinterpret_cast<GetDpi>(GetProcAddress(user32, "GetDpiForSystem"));
            if (!(adjust && getDpi && adjust(&frame, style, menu != nullptr, extended, getDpi())))
                AdjustWindowRectEx(&frame, style, menu != nullptr, extended);
            const auto desktop = WindowMode::GetDesktopRect(nullptr);
            x = desktop.left + (desktop.right - desktop.left - clientWidth) / 2 + frame.left;
            y = desktop.top + (desktop.bottom - desktop.top - clientHeight) / 2 + frame.top;
            width = frame.right - frame.left;
            height = frame.bottom - frame.top;
        }
        const auto result = NativeCreateWindow(extended, className, title, style, x, y, width, height, parent, menu, instance, parameter);
        if (game)
            StartupWindow = result;
        return result;
    }

    BOOL ShowStartupWindow(HWND window, int command, decltype(&ShowWindow) original)
    {
        if (IsStartupWindow(window))
        {
            StartupWindow = window;
            FitStartupWindow(window);
            if (command == SW_SHOWMAXIMIZED || command == SW_SHOWDEFAULT)
                command = SW_SHOWNORMAL;
        }
        return original(window, command);
    }

    BOOL WINAPI MfcShowWindow(HWND window, int command) { return ShowStartupWindow(window, command, NativeMfcShowWindow); }

    bool IsMovieWindow(HWND window)
    {
        char title[64]{};
        GetWindowTextA(window, title, sizeof(title));
        return StartupSizing && strcmp(title, "ActiveMovie Window") == 0;
    }

    HWND WINAPI CreateMovieWindow(DWORD extended, LPCWSTR className, LPCWSTR title, DWORD style,
        int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        if (StartupSizing && title && !IS_INTRESOURCE(title) && wcscmp(title, L"ActiveMovie Window") == 0)
            style &= ~WS_VISIBLE;
        return NativeMovieCreateWindow(extended, className, title, style, x, y, width, height, parent, menu, instance, parameter);
    }

    BOOL WINAPI ShowMovieWindow(HWND window, int command)
    {
        if (IsMovieWindow(window))
        {
            if (!MovieReady)
                command = SW_HIDE;
            else
            {
                FitStartupWindow(window);
                if (command == SW_SHOWMAXIMIZED || command == SW_SHOWDEFAULT)
                    command = SW_SHOWNORMAL;
            }
        }
        return NativeMovieShowWindow(window, command);
    }

    BOOL WINAPI SetMovieWindowPos(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
    {
        if (IsMovieWindow(window))
        {
            if (!MovieReady)
                flags &= ~SWP_SHOWWINDOW;
            if (!(flags & SWP_NOSIZE))
            {
                const int clientWidth = std::clamp(SavedSize("Display Width", 640), 640, 32768);
                const int clientHeight = std::clamp(SavedSize("Display Height", 480), 480, 32768);
                auto [left, top, frameWidth, frameHeight] = WindowMode::GetCenteredWindowRect(window, clientWidth, clientHeight);
                x = left; y = top; width = clientWidth + frameWidth; height = clientHeight + frameHeight;
                flags &= ~SWP_NOMOVE;
            }
        }
        return NativeMovieSetWindowPos(window, after, x, y, width, height, flags);
    }

    BOOL WINAPI SetStartupWindowPos(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
    {
        if (IsStartupWindow(window) && !(flags & SWP_NOSIZE))
        {
            const int clientWidth = std::clamp(SavedSize("Display Width", 640), 640, 32768);
            const int clientHeight = std::clamp(SavedSize("Display Height", 480), 480, 32768);
            auto [left, top, frameWidth, frameHeight] = WindowMode::GetCenteredWindowRect(window, clientWidth, clientHeight);
            x = left; y = top; width = clientWidth + frameWidth; height = clientHeight + frameHeight;
            flags &= ~SWP_NOMOVE;
        }
        return NativeSetWindowPos(window, after, x, y, width, height, flags);
    }

    template<typename T>
    void ReplaceStartupImport(HMODULE module, const char* name, T replacement, T& original)
    {
        auto base = reinterpret_cast<uintptr_t>(module);
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        auto imports = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        for (; imports->Name; ++imports)
        {
            if (_stricmp(reinterpret_cast<const char*>(base + imports->Name), "USER32.dll") != 0)
                continue;
            auto names = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imports->OriginalFirstThunk);
            auto addresses = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imports->FirstThunk);
            for (size_t i = 0; names[i].u1.AddressOfData; ++i)
            {
                if (IMAGE_SNAP_BY_ORDINAL(names[i].u1.Ordinal))
                    continue;
                auto import = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names[i].u1.AddressOfData);
                if (strcmp(reinterpret_cast<const char*>(import->Name), name) != 0)
                    continue;
                auto slot = reinterpret_cast<T*>(&addresses[i].u1.Function);
                DWORD protect;
                VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &protect);
                original = *slot;
                *slot = replacement;
                VirtualProtect(slot, sizeof(*slot), protect, &protect);
                return;
            }
        }
        std::terminate();
    }

    export void InitStartupWindow()
    {
        StartupSizing = WindowMode::nWindowMode == WindowMode::Windowed && !FixSettings().Bool("MAIN.SkipIntro");
        if (!StartupSizing)
            return;
        auto mfc = GetModuleHandleW(L"MFC42.dll");
        ReplaceStartupImport(mfc, "CreateWindowExA", &CreateStartupWindow, NativeCreateWindow);
        ReplaceStartupImport(mfc, "ShowWindow", &MfcShowWindow, NativeMfcShowWindow);
        ReplaceStartupImport(mfc, "SetWindowPos", &SetStartupWindowPos, NativeSetWindowPos);
    }

    void PrepareMovie(SafetyHookContext&)
    {
        if (!StartupSizing)
            return;
        static bool installed = false;
        if (installed)
            return;
        auto quartz = GetModuleHandleW(L"quartz.dll");
        if (!quartz)
            std::terminate();
        ReplaceStartupImport(quartz, "CreateWindowExW", &CreateMovieWindow, NativeMovieCreateWindow);
        ReplaceStartupImport(quartz, "ShowWindow", &ShowMovieWindow, NativeMovieShowWindow);
        ReplaceStartupImport(quartz, "SetWindowPos", &SetMovieWindowPos, NativeMovieSetWindowPos);
        installed = true;
    }

    void Update()
    {
        if (!Window || !Video || !Renderer || !IsWindow(Window))
            return;
        RECT client{};
        GetClientRect(Window, &client);
        int width = client.right, height = client.bottom;
        if (width <= 0 || height <= 0)
            return;
        long videoWidth = 0, videoHeight = 0;
        if (FAILED(Video->GetVideoSize(&videoWidth, &videoHeight)) || videoWidth <= 0 || videoHeight <= 0)
            return;
        const bool original = FixSettings().Int("MAIN.FMVWidescreenMode") == 0;
        const double areaWidth = original ? std::min(double(width), double(height) * 4.0 / 3.0) : double(width);
        const double areaHeight = original ? std::min(double(height), double(width) * 3.0 / 4.0) : double(height);
        const double scale = std::min(areaWidth / videoWidth, areaHeight / videoHeight);
        long destWidth = std::max(1L, std::lround(videoWidth * scale));
        long destHeight = std::max(1L, std::lround(videoHeight * scale));
        long x = (width - destWidth) / 2;
        long y = (height - destHeight) / 2;
        if (!EqualRect(&client, &LastClient))
        {
            if (SUCCEEDED(Renderer->SetWindowPosition(0, 0, width, height)) &&
                SUCCEEDED(Video->SetSourcePosition(0, 0, videoWidth, videoHeight)) &&
                SUCCEEDED(Video->SetDestinationPosition(x, y, destWidth, destHeight)))
            {
                LastClient = client;
            }
            return;
        }
        // The game can set the destination again from the desktop size after the window was made smaller
        long oldX = 0, oldY = 0, oldWidth = 0, oldHeight = 0;
        if (FAILED(Video->GetDestinationPosition(&oldX, &oldY, &oldWidth, &oldHeight)) ||
            oldX != x || oldY != y || oldWidth != destWidth || oldHeight != destHeight)
            Video->SetDestinationPosition(x, y, destWidth, destHeight);
    }

    void Begin(SafetyHookContext& context)
    {
        auto renderer = *reinterpret_cast<IVideoWindow**>(context.ebp - 0x14);
        auto video = *reinterpret_cast<IBasicVideo**>(context.ebp - 0x28);
        auto window = *reinterpret_cast<HWND*>(context.ebp - 0xB8);
        if (!renderer || !video || !IsWindow(window))
            return;
        renderer->put_FullScreenMode(OAFALSE);
        Window = window;
        Renderer = renderer;
        Video = video;
        LastClient = {};
        bool windowed = WindowMode::nWindowMode == WindowMode::Windowed;
        DWORD style = windowed ? WS_OVERLAPPEDWINDOW : WS_POPUP;
        SetWindowLongW(window, GWL_STYLE, style);
        if (windowed)
        {
            int width = std::max(640, SavedSize("Display Width", 640));
            int height = std::max(480, SavedSize("Display Height", 480));
            auto [x, y, frameWidth, frameHeight] = WindowMode::GetCenteredWindowRect(window, width, height);
            SetWindowPos(window, nullptr, x, y, width + frameWidth, height + frameHeight,
                         SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
        else
        {
            MONITORINFO monitor{sizeof(monitor)};
            if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
            {
                auto& rect = monitor.rcMonitor;
                SetWindowPos(window, nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                             SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
        }
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        if (auto dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
        {
            if (auto set = reinterpret_cast<SetAttribute>(GetProcAddress(dwm, "DwmSetWindowAttribute")))
            {
                DWORD preference = 1;
                set(window, 33, &preference, sizeof(preference));
            }
            FreeLibrary(dwm);
        }
        renderer->put_BorderColor(0);
        MovieReady = true;
        renderer->put_Visible(OATRUE);
        Update();
    }

    BOOL WINAPI PeekMessage(LPMSG message, HWND window, UINT first, UINT last, UINT remove)
    {
        Update();
        return VideoPeekMessageA(message, window, first, last, remove);
    }

    export void Init()
    {
        auto begin = hook::pattern("FF 91 90 00 00 00 8B 45 DC 53 53 6A 01");
        auto prepare = hook::pattern("8B 45 E8 8B 10 53 8D 8D 20 FD FF FF 51 50 FF 52 34");
        auto end = hook::pattern("8B 45 EC 8B 08 53 50 FF 51 4C 8B 45 E0");
        auto peek = hook::pattern("8B 35 ? ? ? ? 8B 3D ? ? ? ? 38 5D 0B 0F 85");
        PrepareHook = safetyhook::create_mid(prepare.get_first(), PrepareMovie);
        BeginHook = safetyhook::create_mid(begin.get_first(-8), Begin);
        EndHook = safetyhook::create_mid(end.get_first(), [](SafetyHookContext&)
        {
            Window = nullptr;
            Renderer = nullptr;
            Video = nullptr;
            StartupSizing = false;
        });
        static auto peekMessage = &PeekMessage;
        injector::WriteMemory(peek.get_first(2), &peekMessage, true);
    }
}

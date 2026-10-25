#pragma once
#include "../WindowCursor.hpp"

namespace TonyDisplay
{
    int Mode = 0;
    int Width = 0, Height = 0;
    SafetyHookMid MovieHook;

    bool IsLogo(const char* filename)
    {
        if (!filename)
            return false;
        std::string_view name(filename);
        if (auto slash = name.find_last_of("/\\"); slash != std::string_view::npos)
            name.remove_prefix(slash + 1);
        if (name.size() > 4 && _strnicmp(name.data() + name.size() - 4, ".bik", 4) == 0)
            name.remove_suffix(4);
        constexpr std::string_view logos[] = { "ATVI", "Beenox", "Beenox_Shift", "Dolby_Digital", "Intel_intro", "NSlogo", "New_Aspyr_firelogo", "gearbox" };
        for (auto logo : logos)
        {
            if (name.size() == logo.size() && _strnicmp(name.data(), logo.data(), name.size()) == 0)
                return true;
        }
        return false;
    }

    BOOL WINAPI SetWindowPosHook(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
    {
        if (!Mode || window != WindowedModeWrapper::GameHWND || (flags & (SWP_NOMOVE | SWP_NOSIZE)) == (SWP_NOMOVE | SWP_NOSIZE))
            return SetWindowPos(window, after, x, y, width, height, flags);
        WindowedModeWrapper::desiredClientWidth = Width;
        WindowedModeWrapper::desiredClientHeight = Height;
        WindowedModeWrapper::afterCreateWindow();
        if (Mode == 2)
            WindowedModeWrapper::CenterWindowPosition(Width, Height);
        return SetWindowPos(window, after, 0, 0, 0, 0, (flags | SWP_NOMOVE | SWP_NOSIZE) & ~SWP_FRAMECHANGED);
    }

    LONG WINAPI SetWindowLongA_Hook(HWND window, int index, LONG value)
    {
        return Mode ? WindowedModeWrapper::SetWindowLongA_Hook(window, index, value) : SetWindowLongA(window, index, value);
    }

    LONG WINAPI SetWindowLongW_Hook(HWND window, int index, LONG value)
    {
        return Mode ? WindowedModeWrapper::SetWindowLongW_Hook(window, index, value) : SetWindowLongW(window, index, value);
    }

    BOOL WINAPI AdjustWindowRectHook(LPRECT rect, DWORD style, BOOL menu)
    {
        return Mode ? WindowedModeWrapper::AdjustWindowRect_Hook(rect, style, menu) : AdjustWindowRect(rect, style, menu);
    }

    BOOL WINAPI ShowWindowHook(HWND window, int command)
    {
        if (Mode && window == WindowedModeWrapper::GameHWND)
        {
            if (command == SW_MAXIMIZE)
                command = SW_SHOWNORMAL;
            else if (command == SW_MINIMIZE || command == SW_SHOWMINIMIZED || command == SW_SHOWMINNOACTIVE)
                return IsWindowVisible(window);
        }
        return ShowWindow(window, command);
    }

    void SquareCorners(HWND window)
    {
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static auto setAttribute = []() -> SetAttribute
        {
            auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return module ? reinterpret_cast<SetAttribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
        }();
        const int preference = 1;
        if (setAttribute && window)
            setAttribute(window, 33, &preference, sizeof(preference));
    }

    HWND WINAPI CreateWindowExA_Hook(DWORD exStyle, LPCSTR className, LPCSTR title, DWORD style, int x, int y, int width, int height,
        HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        auto window = Mode
            ? WindowedModeWrapper::CreateWindowExA_Hook(exStyle, className, title, style, x, y, width, height, parent, menu, instance, parameter)
            : CreateWindowExA(exStyle, className, title, style, x, y, width, height, parent, menu, instance, parameter);
        if (window && !parent && !(style & WS_CHILD))
        {
            SquareCorners(window);
            WindowCursor::Attach(window);
        }
        return window;
    }

    HWND WINAPI CreateWindowExW_Hook(DWORD exStyle, LPCWSTR className, LPCWSTR title, DWORD style, int x, int y, int width, int height,
        HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        auto window = Mode
            ? WindowedModeWrapper::CreateWindowExW_Hook(exStyle, className, title, style, x, y, width, height, parent, menu, instance, parameter)
            : CreateWindowExW(exStyle, className, title, style, x, y, width, height, parent, menu, instance, parameter);
        if (window && !parent && !(style & WS_CHILD))
        {
            SquareCorners(window);
            WindowCursor::Attach(window);
        }
        return window;
    }

    void Window(int mode, int width, int height)
    {
        Mode = mode;
        Width = width;
        Height = height;
        WindowedModeWrapper::EnableDpiAwareness();
        WindowedModeWrapper::bBorderlessWindowed = Mode == 2;
        WindowedModeWrapper::bStretchWindow = Mode == 2;
        WindowedModeWrapper::desiredClientWidth = Width;
        WindowedModeWrapper::desiredClientHeight = Height;
        IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
            std::forward_as_tuple("CreateWindowExA", CreateWindowExA_Hook),
            std::forward_as_tuple("CreateWindowExW", CreateWindowExW_Hook));
        if (!Mode)
            return;
        IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
            std::forward_as_tuple("SetWindowLongA", SetWindowLongA_Hook),
            std::forward_as_tuple("SetWindowLongW", SetWindowLongW_Hook),
            std::forward_as_tuple("AdjustWindowRect", AdjustWindowRectHook),
            std::forward_as_tuple("SetWindowPos", SetWindowPosHook),
            std::forward_as_tuple("ShowWindow", ShowWindowHook));
    }

    void Intro(const char* signature, int callOffset, int resumeOffset)
    {
        auto pattern = hook::pattern(signature);
        if (pattern.size() != 1)
        {
            OutputDebugStringA("Tony Hawk: intro movie pattern unavailable.\n");
            return;
        }
        static const uintptr_t resume = reinterpret_cast<uintptr_t>(pattern.get_first(resumeOffset));
        MovieHook = safetyhook::create_mid(pattern.get_first(callOffset), [](SafetyHookContext& regs)
        {
            if (IsLogo(*reinterpret_cast<const char**>(regs.esp)))
                regs.eip = resume;
        });
    }
}

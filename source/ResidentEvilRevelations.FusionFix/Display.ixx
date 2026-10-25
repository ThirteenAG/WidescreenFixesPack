module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module Display;

import ComVars;

// The resolution list, the render resolution and the window.

namespace
{
    HWND StyledWindow = nullptr;

    void ApplyWindowStyle(HWND window)
    {
        if (window == StyledWindow)
            return;
        DWORD process = 0;
        RECT client{};
        GetWindowThreadProcessId(window, &process);
        if (process != GetCurrentProcessId() || GetWindow(window, GW_OWNER) ||
            (GetWindowLong(window, GWL_STYLE) & WS_CHILD) || !GetClientRect(window, &client) ||
            client.right < 320 || client.bottom < 200)
            return;
        StyledWindow = window;
        WindowedModeWrapper::GameHWND = window;
        WindowedModeWrapper::SetWindowLongA_Hook(window, GWL_STYLE, GetWindowLong(window, GWL_STYLE));
    }

    BOOL WINAPI ShowWindowHook(HWND window, int command)
    {
        const auto result = ShowWindow(window, command);
        if (command != SW_HIDE && IsWindowVisible(window))
            ApplyWindowStyle(window);
        return result;
    }

    BOOL CALLBACK EnumWindowsProc(HWND window, LPARAM)
    {
        if (IsWindowVisible(window))
            ApplyWindowStyle(window);
        return TRUE;
    }
}

class Display
{
public:
    Display()
    {
        WFP::onInitEvent() += []()
        {
            // Per monitor DPI awareness, so the game gets real pixels on scaled desktops
            using SetContext = BOOL(WINAPI*)(HANDLE);
            if (auto set = reinterpret_cast<SetContext>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext")))
                set(reinterpret_cast<HANDLE>(-4));
            else
                SetProcessDPIAware();

            // The resolution list: the game offers a fixed list; the fix gives it every mode of the monitor
            struct ResList
            {
                uint32_t stringPtr;
                uint32_t index;
            };
            static std::vector<std::string> strList;
            static std::vector<ResList> resList;
            GetResolutionsList(strList);
            for (size_t i = 0; i < strList.size(); i++)
            {
                uint32_t w = 0, h = 0;
                if (sscanf_s(strList[i].c_str(), "%ux%u", &w, &h) == 2 && w >= 640 && h >= 480)
                    resList.push_back({ (uint32_t)strList[i].data(), static_cast<uint32_t>(i) });
            }
            if (resList.empty())
            {
                strList.emplace_back("640x480");
                resList.push_back({ (uint32_t)strList.back().data(), static_cast<uint32_t>(strList.size() - 1) });
            }

            auto listRefs = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? EB 08 8D A4 24 00 00 00 00 90 83 BE");
            auto listRefs2 = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? 8B FF");
            if (PatternAvailable(listRefs, "ResidentEvilRevelations.FusionFix: Display") && PatternAvailable(listRefs2, "ResidentEvilRevelations.FusionFix: Display"))
            {
                WriteGameValue(listRefs.get_first(4), resList.data());
                WriteGameValue(listRefs.get_first(8 + 4), resList.size());
                WriteGameValue(listRefs2.get_first(4), resList.data());
                WriteGameValue(listRefs2.get_first(8 + 4), resList.size());
            }

            // The render resolution. The game's Aspect option fits a fixed aspect into the screen; the fix always uses
            // the screen's own (the option's DEFAULT).
            struct CaptureSite { const char* pattern; bool ebpEdi; };
            for (auto site : { CaptureSite{ "0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 8E", false }, CaptureSite{ "0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 96", false }, CaptureSite{ "0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 AE", true } })
            {
                hook::pattern(site.pattern).for_each_result([&](hook::pattern_match match)
                {
                    injector::MakeNOP(match.get<void>(0), 12, true);
                    static std::vector<SafetyHookMid> hooks;
                    if (site.ebpEdi)
                        hooks.emplace_back(safetyhook::create_mid(match.get<void>(0), [](SafetyHookContext& regs) { nResX = regs.ebp; nResY = regs.edi; }));
                    else
                        hooks.emplace_back(safetyhook::create_mid(match.get<void>(0), [](SafetyHookContext& regs) { nResX = regs.ecx; nResY = regs.edx; }));
                });
            }

            // MAIN.WindowMode: the window when the game's own Full Screen option is off; 1 = with a frame, 2 = borderless.
            // A framed window as big as the screen is placed with its frame outside the screen; no rounded corners.
            WindowedModeWrapper::bBorderlessWindowed = FixSettings().Int("MAIN.WindowMode") != 1;
            IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
                std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
                std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                std::forward_as_tuple("AdjustWindowRectEx", WindowedModeWrapper::AdjustWindowRectEx_Hook),
                std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook),
                std::forward_as_tuple("ShowWindow", ShowWindowHook)
            );

            // Adopt an already visible window; later visibility changes use ShowWindow.
            EnumWindows(EnumWindowsProc, 0);

        };
    }
} Display;

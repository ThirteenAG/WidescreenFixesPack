module;

#include <stdafx.h>
#include <d3d9.h>
#include <atomic>
#include <cmath>
#include "FixSettings.hpp"
#include "Shared/ResidentEvil/IdleCursor.hpp"
#include "LEDEffects.h"

export module System;

import ComVars;

// Start-up, window and quality-of-life options: no logos at start (MAIN.SkipIntro), the window of the game's
// windowed mode (MAIN.WindowMode), door animations skipped (MAIN.DoorSkip), the mouse cursor hidden when it doesn't
// move.

namespace
{
    int WindowMode = 0;

    int MonitorRefreshRate()
    {
        static HWND window = nullptr;
        static HMONITOR monitor = nullptr;
        static ULONGLONG lastCheck = 0;
        static int refreshRate = 60;
        auto foreground = GetForegroundWindow();
        DWORD process = 0;
        GetWindowThreadProcessId(foreground, &process);
        if (process == GetCurrentProcessId()) window = foreground;
        const auto currentMonitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
        const auto now = GetTickCount64();
        if (currentMonitor != monitor || now - lastCheck >= 1000)
        {
            monitor = currentMonitor;
            lastCheck = now;
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            refreshRate = GetMonitorInfoW(monitor, &info) &&
                EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ?
                std::clamp<int>(mode.dmDisplayFrequency, 30, 1000) : 60;
        }
        return refreshRate;
    }

    DWORD WINAPI ReadDisplayProfile(LPCSTR section, LPCSTR key, LPCSTR fallback, LPSTR output, DWORD capacity, LPCSTR filename)
    {
        const char* basename = filename;
        if (filename)
        {
            for (auto current = filename; *current; ++current)
            {
                if (*current == '\\' || *current == '/')
                    basename = current + 1;
            }
        }
        if (section && key && filename && _stricmp(section, "DISPLAY") == 0 &&
            _stricmp(basename, "config.ini") == 0)
        {
            std::string value;
            if (_stricmp(key, "FullScreen") == 0)
                value = WindowMode == 0 ? "ON" : "OFF";
            else if (_stricmp(key, "Resolution") == 0)
            {
                char saved[256]{};
                const auto savedLength = GetPrivateProfileStringA(section, key, "", saved, DWORD(std::size(saved)), filename);
                if (WindowMode == 1 && savedLength && AddWindowedResolution)
                    AddWindowedResolution(saved);
                if (WindowMode == 2 || !savedLength)
                {
                    auto [width, height] = GetDesktopRes();
                    width = std::clamp(width, 640, 8192);
                    height = std::clamp(height, 480, 8192);
                    value = std::to_string(width) + "x" + std::to_string(height);
                }
            }
            if (!value.empty())
            {
                if (!output || capacity == 0)
                    return 0;
                const auto length = std::min<size_t>(value.size(), capacity - 1);
                memcpy(output, value.data(), length);
                output[length] = '\0';
                return static_cast<DWORD>(length);
            }
        }
        return GetPrivateProfileStringA(section, key, fallback, output, capacity, filename);
    }


}

class System
{
public:
    System()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();

            static std::atomic<bool> pauseOnFocusLoss{ settings.Bool("MAIN.PauseOnFocusLoss") };
            settings.OnChange("MAIN.PauseOnFocusLoss", []()
            {
                pauseOnFocusLoss.store(FixSettings().Bool("MAIN.PauseOnFocusLoss"), std::memory_order_relaxed);
            });
            static SafetyHookMid FocusHook;
            auto focus = hook::pattern("80 79 20 00 75 ? 80 B9 51 25 00 00 00 75 ? 6A 01 FF D6");
            FocusHook = safetyhook::create_mid(focus.get_first(4), [](SafetyHookContext& regs)
            {
                // The second native check is WM_ACTIVATEAPP state; the first permits background updates.
                if (pauseOnFocusLoss.load(std::memory_order_relaxed))
                    regs.eflags |= uint32_t{ 1 << 6 };
                else
                    regs.eflags &= ~uint32_t{ 1 << 6 };
            });
            settings.Availability("MAIN.PauseOnFocusLoss", [](std::string_view) { return bool(FocusHook); });

            static std::atomic<int> fpsLimit{ settings.Int("MAIN.FPSLimit") };
            settings.OnChange("MAIN.FPSLimit", []()
            {
                fpsLimit.store(FixSettings().Int("MAIN.FPSLimit"), std::memory_order_relaxed);
            });
            static SafetyHookMid FrameRateHook;
            auto clock = hook::pattern("F3 0F 10 4B 38 F3 0F 5E 4B 3C 66 0F 5A C0 F3 0F 11 4C 24 ? F3 0F 5C C8");
            FrameRateHook = safetyhook::create_mid(clock.get_first(10), [](SafetyHookContext& regs)
            {
                const auto requested = fpsLimit.load(std::memory_order_relaxed);
                const auto limit = requested == -1 ? MonitorRefreshRate() : requested == 0 ? 0 : std::clamp(requested, 30, 1000);
                const auto rate = *reinterpret_cast<const float*>(regs.ebx + 0x38);
                // Change only the variable clock's wait threshold; elapsed time, carry and fixed clocks stay native.
                if (std::isfinite(rate) && rate > 0.0f)
                    regs.xmm1.f32[0] = limit == 0 ? 0.0f : rate / limit;
            });
            settings.Availability("MAIN.FPSLimit", [](std::string_view) { return bool(FrameRateHook); });

            // The logo scene (aLogo) waits (state 0), shows the division and copyright logos (state 1) and then changes
            // to the title scene (state 2). Starting it at state 2 goes to the title at once.
            if (settings.Bool("MAIN.SkipIntro"))
            {
                auto pattern = hook::pattern("56 8B F1 8B 46 30 83 E8 00 74 ? 48 74 ? 48 75 ? 8B 0D");
                static SafetyHookMid LogoHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
                {
                    static bool bSkipped = false;
                    if (!bSkipped && regs.eax == 0)
                    {
                        bSkipped = true;
                        regs.eax = 2;
                        *reinterpret_cast<uint32_t*>(regs.esi + 0x30) = 2;
                    }
                });

                // The title's save-icon notice is an OK-only dialog. Arm only for
                // that call and let the dialog's own confirmation and close run.
                static bool noticePending = false;
                static auto NoticeCreated = safetyhook::create_mid(hook::pattern("6A 01 6A 00 E8 ? ? ? ? C7 86 7C 02 00 00 0E 00 00 00").get_first(9), [](SafetyHookContext&)
                {
                    noticePending = true;
                });
                // This call is the successful startup load notice, also OK-only.
                static auto LoadNoticeCreated = safetyhook::create_mid(hook::pattern("6A 03 6A 0A 6A 12 E8 ? ? ? ? C7 86 7C 02 00 00 1F 00 00 00").get_first(11), [](SafetyHookContext&)
                {
                    noticePending = true;
                });
                static auto NoticeConfirm = safetyhook::create_mid(hook::pattern("85 81 6C 0B 00 00 74 38 F3 0F 10 86 74 02 00 00").get_first(6), [](SafetyHookContext& regs)
                {
                    if (noticePending && *reinterpret_cast<float*>(regs.esi + 0x274) >= *reinterpret_cast<float*>(regs.esi + 0x278))
                    {
                        noticePending = false;
                        regs.eflags &= ~0x40u;
                    }
                });
            }

            static std::atomic<int> nHideMouseCursorAfterMs{ settings.Int("MAIN.HideMouseCursorAfterMs") };
            settings.OnChange("MAIN.HideMouseCursorAfterMs", []()
            {
                nHideMouseCursorAfterMs.store(FixSettings().Int("MAIN.HideMouseCursorAfterMs"), std::memory_order_relaxed);
            });
            // Keep the hooks installed so a timeout of zero can be enabled while playing.
            static std::atomic<bool> cursorIdle{ false };
            {
                auto pattern = hook::pattern("80 BE ? ? ? ? ? 74 13 FF B6");
                // The native cursor refresh stays intact. Hide only its shape, and restore its current handle after movement.
                static auto MouseCursorHook = safetyhook::create_mid(pattern.get_first(28), [](SafetyHookContext& regs)
                {
                    auto window = *reinterpret_cast<HWND*>(regs.esi + 0x12C);
                    auto region = *reinterpret_cast<HWND*>(regs.esi + 0x2664);
                    auto foreground = GetForegroundWindow();
                    POINT point{};
                    bool active = foreground && (foreground == window || foreground == region) && GetCursorPos(&point);
                    static IdleCursor::State state;
                    switch (state.Update(point.x, point.y, GetTickCount64(), active, nHideMouseCursorAfterMs.load(std::memory_order_relaxed)))
                    {
                        case IdleCursor::Action::Hide:
                            cursorIdle.store(true, std::memory_order_relaxed);
                            SetCursor(nullptr);
                            break;
                        case IdleCursor::Action::Restore:
                            cursorIdle.store(false, std::memory_order_relaxed);
                            SetCursor(*reinterpret_cast<HCURSOR*>(regs.esi + 0x2680));
                            break;
                        default:
                            break;
                    }
                });

                // Menus draw their own pointer sprite at the mouse position; while idle, move it out of view. The next
                // movement places it back.
                auto pointer = hook::pattern("83 EC 08 53 8B D9 56 8D B3 60 02 00 00 56 FF 15 ? ? ? ? 8B CB E8 ? ? ? ? 80 BB 54 02 00 00 00 0F 84 ? ? ? ? 83 BB 78 02 00 00 00");
                auto start = reinterpret_cast<uintptr_t>(pointer.get_first());
                auto unlock = hook::range_pattern(start, start + 0x140, "56 FF 15 ? ? ? ? 5E 5B 83 C4 08 C3").get_first();
                static auto PointerHook = safetyhook::create_mid(unlock, [](SafetyHookContext& regs)
                {
                    const auto node = *reinterpret_cast<uint8_t**>(regs.ebx + 0x278);
                    if (!cursorIdle.load(std::memory_order_relaxed) || !*reinterpret_cast<uint8_t*>(regs.ebx + 0x254) || !node)
                        return;
                    *reinterpret_cast<float*>(node + 160) = -4096.0f;
                    *reinterpret_cast<float*>(node + 164) = -4096.0f;
                    *reinterpret_cast<uint32_t*>(node + 84) |= 0x10000;
                });
            }

            if (settings.Bool("MAIN.DoorSkip"))
            {
                injector::MakeNOP(hook::get_pattern("0F 84 CC 00 00 00 8B 47 3C"), 6, true);
                auto pattern = hook::pattern("F3 0F 11 47 ? 81 49 ? ? ? ? ? EB 07");
                struct DoorkSkipHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *reinterpret_cast<float*>(regs.edi + 0x2C) = -1.0f;
                    }
                }; injector::MakeInline<DoorkSkipHook>(pattern.get_first(0));
            }

            {
                WindowMode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
                WindowedModeWrapper::bBorderlessWindowed = WindowMode == 2;
                IATHook::Replace(GetModuleHandleA(nullptr), "KERNEL32.dll",
                    std::forward_as_tuple("GetPrivateProfileStringA", ReadDisplayProfile));
                if (WindowMode != 0)
                {
                    IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                        std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
                        std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
                        std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                        std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                        std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                        std::forward_as_tuple("AdjustWindowRectEx", WindowedModeWrapper::AdjustWindowRectEx_Hook),
                        std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)
                    );
                }
            }

        };
    }
} System;

module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

// Frame limit, cursor confinement, developer overlay and update check on the game's device; focus handling in the
// game's window.

export module Renderer;

import Window;
import DeveloperOverlay;
import ComVars;

export namespace SecondSightRenderer
{
    // Update notice: one line under the game's logo in the menus, in the game's own text
    inline std::string notice;
    inline std::string noticeVersion;
    inline bool noticeUsesHub = false;
    inline std::atomic<bool> updatesEnabled{ false };
    inline std::atomic<bool> noticeLanguageChanged{ false };
    inline SafetyHookMid NoticeHook;
    inline int32_t(__cdecl* SetFont)(int32_t) = nullptr;
    inline void(__cdecl* MeasureText)(float*, float*, const char*, int32_t) = nullptr;
    inline void(__cdecl* DrawText)(float, float, const char*, uint32_t, uint32_t) = nullptr;

    inline std::wstring FormatNotice()
    {
        using RWGTA::UpdateCheck::detail::Replace;
        auto& settings = FixSettings();
        auto& texts = settings.GetTexts();
        auto line = texts.Resolve(noticeUsesHub ? "@update.availablehub" : "@update.availablesite");
        line = Replace(line, "{name}", texts.Resolve(settings.GetDescription().name));
        return asi_settings::text::Widen(Replace(line, "{version}", noticeVersion));
    }

    inline void CacheNotice(std::wstring_view line)
    {
        const int length = WideCharToMultiByte(1252, 0, line.data(), int(line.size()), nullptr, 0, nullptr, nullptr);
        notice.resize(size_t(length));
        if (length)
            WideCharToMultiByte(1252, 0, line.data(), int(line.size()), notice.data(), length, nullptr, nullptr);
    }

    // After the logo is drawn; its draw call's arguments are still on the stack: left, top, right, bottom
    inline void DrawNotice(SafetyHookContext& context)
    {
        if (!updatesEnabled.load(std::memory_order_relaxed))
            return;
        if (!noticeVersion.empty() && noticeLanguageChanged.load(std::memory_order_relaxed) &&
            noticeLanguageChanged.exchange(false, std::memory_order_acquire))
            CacheNotice(RWGTA::UpdateCheck::ReplacePage(FormatNotice()));
        if (notice.empty())
            return;
        const auto args = reinterpret_cast<const int32_t*>(context.esp);
        constexpr uint32_t alpha = 0x7F;
        const auto font = SetFont(24);
        float width = 0.0f, height = 0.0f;
        MeasureText(&width, &height, notice.c_str(), 0);
        DrawText(float(args[0]), float(args[3]) + height * 0.25f, notice.c_str(), 0x7F7F7F00 | alpha, alpha);
        SetFont(font);
    }

    // MAIN.TextLanguage: Automatic follows the language of Windows
    inline void SelectLanguage()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (language.empty() || language == "auto")
        {
            switch (PRIMARYLANGID(GetUserDefaultUILanguage()))
            {
                case LANG_FRENCH: language = "fr"; break;
                case LANG_GERMAN: language = "de"; break;
                case LANG_ITALIAN: language = "it"; break;
                case LANG_SPANISH: language = "es"; break;
                default: language = "en"; break;
            }
        }
        FixSettings().GetTexts().Select(language);
        noticeLanguageChanged.store(true, std::memory_order_release);
    }

    inline std::atomic<int> requestedFrameLimit{ 0 };
    inline std::atomic<bool> pauseOnFocusLoss{ false };
    inline std::atomic<bool> confineCursor{ false };
    inline UINT refreshActivity = 0;

    inline void UpdatePauseOnFocusLoss()
    {
        pauseOnFocusLoss.store(FixSettings().Bool("MAIN.PauseOnFocusLoss"), std::memory_order_relaxed);
        if (const auto window = SecondSightWindow::Window.load(); window && refreshActivity && SecondSightWindow::Mode)
            PostMessageW(window, refreshActivity, 0, 0);
    }

    inline void UpdateCursorConfinement()
    {
        confineCursor.store(FixSettings().Bool("MAIN.ConfineCursor"), std::memory_order_relaxed);
    }

    inline void UpdateFrameLimit()
    {
        requestedFrameLimit.store(FixSettings().Int("MAIN.FPSLimit"), std::memory_order_relaxed);
    }

    inline void ApplyFrameLimit(HWND target)
    {
        const int requested = requestedFrameLimit.load(std::memory_order_relaxed);
        if (requested >= 0)
        {
            if (D3D9Options::frameLimit.load(std::memory_order_relaxed) != requested)
                D3D9Options::frameLimit.store(requested, std::memory_order_relaxed);
            return;
        }
        static HMONITOR previous = nullptr;
        static ULONGLONG lastCheck = 0;
        static int refresh = 60;
        const auto monitor = MonitorFromWindow(target, MONITOR_DEFAULTTOPRIMARY);
        const auto now = GetTickCount64();
        if (monitor != previous || now - lastCheck >= 2000)
        {
            previous = monitor;
            lastCheck = now;
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            refresh = GetMonitorInfoW(monitor, &info) &&
                EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ?
                int(mode.dmDisplayFrequency) : 60;
        }
        if (D3D9Options::frameLimit.load(std::memory_order_relaxed) != refresh)
            D3D9Options::frameLimit.store(refresh, std::memory_order_relaxed);
    }

    // Gamepad: the PC game reads only the keyboard and mouse. The bundled Xidi turns the pad into the game's keys and
    // mouse (Xidi.ini); the game never opens a pad itself, so Xidi's controller polling is started here.
    inline void InitGamepad()
    {
        const auto xidi = LoadLibraryW(L"Xidi.32.dll");
        if (!xidi)
            return;
        using SendVibration = bool(*)(short, unsigned short, unsigned short);
        if (const auto sendVibration = reinterpret_cast<SendVibration>(GetProcAddress(xidi, "XidiSendVibration")))
            sendVibration(0, 0, 0);
    }

    inline void Init()
    {
        InitGamepad();
        if (SecondSightWindow::Mode)
        {
            refreshActivity = RegisterWindowMessageW(L"SecondSight.FusionFix.RefreshActivity");
            UpdatePauseOnFocusLoss();
            FixSettings().OnChange("MAIN.PauseOnFocusLoss", UpdatePauseOnFocusLoss);
        }
        SecondSightWindow::Message = [](HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
        {
            if (refreshActivity && message == refreshActivity)
            {
                // The game's activation handler pauses its timer, rendering and audio together.
                if (!IsIconic(window))
                {
                    const bool active = !pauseOnFocusLoss.load(std::memory_order_relaxed) || GetForegroundWindow() == window;
                    result = DefSubclassProc(window, WM_ACTIVATEAPP, active, 0);
                }
                return true;
            }
            if (SecondSightOverlay::Message(window, message, wParam, lParam, result))
                return true;
            if (message == WM_DESTROY)
                RWGTA::UpdateCheck::OnExit();
            if (refreshActivity && ((message == WM_SHOWWINDOW && wParam) || (message == WM_SIZE && wParam == SIZE_RESTORED)))
                PostMessageW(window, refreshActivity, 0, 0);
            if (SecondSightWindow::Mode && message == WM_ACTIVATEAPP && !wParam)
            {
                CursorConfinement::Release();
                if (!pauseOnFocusLoss.load(std::memory_order_relaxed))
                {
                    result = 0;
                    return true;
                }
            }
            return false;
        };

        UpdateFrameLimit();
        FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
        CursorConfinement::SetEnabled(false);
        UpdateCursorConfinement();
        FixSettings().OnChange("MAIN.ConfineCursor", UpdateCursorConfinement);
        D3D9Options::limitDevice = [](IDirect3DDevice9*)
        {
            return D3D9Options::frameLimit.load(std::memory_order_relaxed) > 0;
        };
        D3D9Options::frame = [](IDirect3DDevice9* device)
        {
            const auto window = SecondSightWindow::Window.load();
            WFP::onGameProcessEvent().executeAll();
            static bool cursorEnabled = false;
            const bool shouldConfine = confineCursor.load(std::memory_order_relaxed) && !SecondSightOverlay::BlocksInput();
            if (cursorEnabled != shouldConfine)
            {
                CursorConfinement::SetEnabled(shouldConfine);
                cursorEnabled = shouldConfine;
            }
            CursorConfinement::Frame(window);
            ApplyFrameLimit(window);
            RWGTA::UpdateCheck::Tick();
            SecondSightOverlay::Present(device, window);
        };
        D3D9Options::beforeReset = SecondSightOverlay::BeforeReset;
        if (SecondSightWindow::Mode == 1)
        {
            D3D9Options::presentParameters = [](D3DPRESENT_PARAMETERS* params, HWND focusWindow)
            {
                const auto gameWindow = SecondSightWindow::Window.load();
                if (!params->Windowed || !gameWindow || (focusWindow && focusWindow != gameWindow) ||
                    (params->hDeviceWindow && params->hDeviceWindow != gameWindow))
                    return;
                const auto width = DeviceWidth.get(), height = DeviceHeight.get();
                if (width && height)
                {
                    // A secondary DXUT device reset can use the work-area size even when the game kept the chosen size.
                    params->BackBufferWidth = width;
                    params->BackBufferHeight = height;
                }
            };
        }

        RWGTA::UpdateCheck::Config update;
        update.pluginId = "SecondSight.FusionFix";
        update.fileVersion = rsc_FileVersion;
        update.settings = &FixSettings();
        update.playable = []() { return true; };
        update.format = [](const std::string& version, bool hub)
        {
            noticeVersion = version;
            noticeUsesHub = hub;
            return FormatNotice();
        };
        update.show = CacheNotice;
        RWGTA::UpdateCheck::Init(std::move(update));
        const auto updateNoticeVisibility = []()
        {
            updatesEnabled.store(FixSettings().Bool("MAIN.CheckForUpdates"), std::memory_order_relaxed);
        };
        updateNoticeVisibility();
        FixSettings().OnChange("MAIN.CheckForUpdates", updateNoticeVisibility);
        SelectLanguage();
        FixSettings().OnChange("MAIN.TextLanguage", SelectLanguage);

        auto font = hook::pattern("8B 4C 24 04 A1 ? ? ? ? 8B 15 ? ? ? ? 89 0D ? ? ? ? 6B C9 1C");
        auto measure = hook::pattern("83 EC 50 A1 ? ? ? ? 33 44 24 50 33 D2 89 44 24 4C A1 ? ? ? ? 83 E0 01");
        auto text = hook::pattern("8B 44 24 14 8B 4C 24 10 8B 54 24 0C 50 8B 44 24 0C 51 8B 4C 24 0C 52 6A");
        auto logo = hook::pattern("68 60 7F 7F 7F C1 E5 04 55 C1 E3 04 53 6A 00 6A 00 57 56 6A 18 6A 20 E8 ? ? ? ? 8B 44 24 6C");
        SetFont = reinterpret_cast<decltype(SetFont)>(font.get_first());
        MeasureText = reinterpret_cast<decltype(MeasureText)>(measure.get_first());
        DrawText = reinterpret_cast<decltype(DrawText)>(text.get_first());
        NoticeHook = safetyhook::create_mid(logo.get_first(28), &DrawNotice);
        D3D9Options::HookCreation();
    }
}

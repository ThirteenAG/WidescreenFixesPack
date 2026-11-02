#include "stdafx.h"
#include "FixSettings.hpp"
#include <atomic>
#include "Shared/WidescreenFixConflict.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

import Display;
import NativeMenu;
import DeveloperOverlay;
import ComVars;

bool USMOverlayBlocksInput()
{
    return USMOverlay::Overlay.BlocksInput();
}

bool USMPadMenuShortcutsAvailable()
{
    return USMMenu::PadShortcutsEnabled;
}

namespace USMXidi
{
    std::wstring ProxyPath;

    HMODULE WINAPI Load(LPCSTR systemPath)
    {
        if (!ProxyPath.empty())
        {
            const auto proxy = LoadLibraryW(ProxyPath.c_str());
            if (proxy)
            {
                if (GetProcAddress(proxy, "DirectInput8Create"))
                    return proxy;
                FreeLibrary(proxy);
            }
        }
        return LoadLibraryA(systemPath);
    }

    void Init()
    {
        auto load = hook::pattern("83 C4 0C 8D 84 24 18 01 00 00 50 FF 15 ? ? ? ? 3B C3 A3 ? ? ? ? 74 13 68");
        std::array<wchar_t, 32768> executable{};
        const auto length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
        if (!length || length >= executable.size())
            return;
        const auto directory = std::filesystem::path(executable.data()).parent_path();
        std::error_code error;
        const auto proxy = directory / "dinput8.dll";
        if (!std::filesystem::is_regular_file(directory / "Xidi.32.dll", error) ||
            !std::filesystem::is_regular_file(proxy, error))
            return;
        ProxyPath = proxy.wstring();
        // The game loads an absolute system path, bypassing local DirectInput proxies.
        injector::MakeCALL(load.get_first(11), Load, true);
        injector::MakeNOP(load.get_first(16), 1, true);
    }
}

namespace USMLanguage
{
    SafetyHookInline ReadHook;
    std::filesystem::path GameDirectory;
    int Selected = -1;
    std::atomic<int> Active{ 0 };

    int Index(std::string_view value)
    {
        constexpr std::string_view codes[] = { "en", "fr", "de", "es", "it" };
        for (size_t index = 0; index < std::size(codes); ++index)
            if (value == codes[index]) return static_cast<int>(index);
        return -1;
    }

    bool Installed(std::string_view value)
    {
        if (value == "auto")
            return true;
        const auto index = Index(value);
        if (index < 0)
            return false;
        constexpr char suffixes[] = { 'e', 'f', 'g', 's', 'i' };
        std::error_code error;
        return std::filesystem::is_regular_file(GameDirectory / "data" /
            (std::string("usm_lt") + suffixes[index] + ".usm"), error);
    }

    int __fastcall Read(const char* settings, void*, const char* key, int fallback)
    {
        if (Selected >= 0 && key && _stricmp(key, "Settings\\Language") == 0)
        {
            Active.store(Selected);
            return Selected;
        }
        const auto result = ReadHook.thiscall<int>(settings, key, fallback);
        if (key && _stricmp(key, "Settings\\Language") == 0)
            Active.store(result >= 0 && result < 5 ? result : 0);
        return result;
    }

    void Init()
    {
        std::array<wchar_t, 32768> path{};
        const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length && length < path.size())
            GameDirectory = std::filesystem::path(path.data()).parent_path();
        auto reader = hook::pattern("83 EC 08 8D 04 24 50 8D 54 24 08 52 8B 54 24 14 8D 44 24 14 50 52 C7 44 24 10 04 00 00 00 E8 ? ? ? ? 84 C0 74 0B");
        if (!GameDirectory.empty())
        {
            auto hook = safetyhook::InlineHook::create(reader.get_first(), reinterpret_cast<void*>(&Read));
            if (hook)
                ReadHook = std::move(*hook);
        }
        auto& settings = FixSettings();
        settings.Availability("MAIN.Language", [](std::string_view value)
        {
            return value == "auto" || (ReadHook && Installed(value));
        }, []()
        {
            return USMMenu::Words("language.unavailable");
        });
        const auto selected = settings.String("MAIN.Language");
        Selected = ReadHook && Installed(selected) ? Index(selected) : -1;
        USMMenu::GameLanguage = []()
        {
            constexpr std::string_view codes[] = { "en", "fr", "de", "es", "it" };
            return std::string(codes[Active.load()]);
        };
    }
}

namespace USMUpdates
{
    void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    void Init()
    {
        const bool available = USMMenu::LoopHook && USMMenu::EventHook && USMMenu::DestroyHook;
        FixSettings().Availability("MAIN.CheckForUpdates", [available](std::string_view value)
        {
            return value == "0" || available;
        });
        if (!available)
            return;
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "UltimateSpiderMan.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return USMMenu::Manager && !USMMenu::Opened; };
        config.format = [](const std::string&, bool hub)
        {
            return asi_settings::text::Widen(USMMenu::Words(hub ? "update.availablehub" : "update.availablesite"));
        };
        config.show = [](const std::wstring& text)
        {
            USMMenu::Click(USMMenu::Tab, 2, 0);
            USMMenu::Notice = asi_settings::text::Narrow(text);
            USMMenu::Refresh();
        };
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}

std::atomic<bool> PauseOnFocusLoss{ true };

SafetyHookInline shWindowProc;
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_ACTIVATEAPP)
        WFP::onActivateApp().executeAll(wParam != 0);
    LRESULT result = 0;
    if (USMOverlay::Message(window, message, wParam, lParam, result))
        return result;
    if (USMMenu::Message(message, wParam, lParam))
        return 0;
    return shWindowProc.stdcall<LRESULT>(window, message, wParam, lParam);
}

void Init()
{
    hook::pattern pattern;
    FixSettings();
    USMXidi::Init();
    USMLanguage::Init();
    asi_settings::LiveLink::Start();
    USMMenu::Init();
    USMUpdates::Init();
    USMOverlay::Init();
    USMMenu::Render = USMOverlay::Frame;
    WFP::onShutdownEvent() += []() { USMOverlay::Overlay.ShutdownRenderer(); };
    PauseOnFocusLoss.store(FixSettings().Bool("MAIN.PauseOnFocusLoss"));
    FixSettings().OnChange("MAIN.PauseOnFocusLoss", []()
    {
        PauseOnFocusLoss.store(FixSettings().Bool("MAIN.PauseOnFocusLoss"));
    });
    auto focus = hook::pattern("A0 ? ? ? ? 84 C0 0F 84 ? ? ? ? A0 ? ? ? ? 84 C0 75 08 6A 01");
    static const auto continueFrame = reinterpret_cast<uintptr_t>(focus.get_first(13));
    static auto FocusPauseHook = safetyhook::create_mid(focus.get_first(5), [](SafetyHookContext& regs)
    {
        // Only bypass the update-loop pause; native focus and input stay unchanged.
        if (!PauseOnFocusLoss.load())
            regs.eip = continueFrame;
    });
    pattern = hook::pattern("A1 ? ? ? ? 53 50 FF 15"); // 0x581CBB + 1
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));


    pattern = hook::pattern("53 8B 5C 24 0C 81 FB 01 01 00 00 55 8B 6C 24 18"); // 0x5941A0
    shWindowProc = safetyhook::create_inline(pattern.get_first(), WindowProc);


    // Only the final nglFlip scene: offscreen radar and intermediate scenes
    // must not receive post-processing or apply gamma multiple times per frame.
    pattern = hook::pattern("A1 ? ? ? ? 8B 10 50 FF 92 A8 00 00 00 A0"); // 0x76E99E + 1
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onEndScene().executeAll();
        USMOverlay::Frame(Direct3DDevice.get(), hWnd.get());
    });


    pattern = hook::pattern("53 56 E8 ? ? ? ? 6A 01 E8 ? ? ? ? A0 ? ? ? ? 33 DB 83 C4 04"); // 0x76E800
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        USMOverlay::Overlay.BeforeReset();
        WFP::onBeforeReset().executeAll();
    });


    pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 85 C0 74 ? 68 ? ? ? ? 68 ? ? ? ? 8B C8 E8 ? ? ? ? EB ? 33 C0 6A"); //0x5AC1E0
    static auto WinMainHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onInitEvent().executeAll();
    });


    // WinMain's per-frame call to the game update/render routine.
    pattern = hook::pattern("E8 ? ? ? ? FF D5"); //0x5AD495
    static const auto resumeFrame = reinterpret_cast<uintptr_t>(pattern.get_first(5));
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        static std::once_flag of;
        std::call_once(of, []()
        {
            WFP::onGameInitEvent().executeAll();
        });

        WFP::onGameProcessEvent().executeAll();
        if (USMWindow::ResourcesLost)
            regs.eip = resumeFrame;
    });


}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        auto startup = hook::pattern("81 EC EC 03 00 00 68");
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, startup);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}

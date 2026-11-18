module;
#include <stdafx.h>
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "FixSettings.hpp"

export module Updates;
import Camera;
import Frame;


export namespace KnightRiderUpdates
{
    inline SafetyHookInline MainHook;
    inline bool MenuReady = false;

    inline int __fastcall Main(uint8_t* state, void*, int frame)
    {
        if (KnightRiderFrame::InputBlocked && KnightRiderFrame::InputBlocked())
            return 0;
        static std::once_flag gameInit;
        std::call_once(gameInit, []()
        {
            WFP::onGameInitEvent().executeAll();
        });
        WFP::onGameProcessEvent().executeAll();
        MenuReady = *reinterpret_cast<int*>(state + 16) == 10;
        RWGTA::UpdateCheck::Tick();
        return MainHook.thiscall<int>(state, frame);
    }

    inline void OnExit()
    {
        RWGTA::UpdateCheck::OnExit();
    }

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init()
    {
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "KnightRider.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []()
        {
            return MenuReady && GetForegroundWindow() == WindowedModeWrapper::GameHWND;
        };
        config.format = [](const std::string& latest, bool hub)
        {
            auto texts = FixSettings().GetTexts();
            std::string language = FixSettings().String("MAIN.TextLanguage");
            if (language.empty() || language == "auto")
            {
                wchar_t locale[LOCALE_NAME_MAX_LENGTH]{};
                language = GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH) ? asi_settings::text::Narrow(locale) : "en";
            }
            texts.Select(language);
            auto line = texts.Resolve(hub ? "@update.availablehub" : "@update.availablesite");
            for (const auto& [key, value] : { std::pair<std::string, std::string>{ "{name}", texts.Resolve(FixSettings().GetDescription().name) },
                std::pair<std::string, std::string>{ "{version}", latest } })
            {
                if (const auto at = line.find(key); at != std::string::npos)
                    line.replace(at, key.size(), value);
            }
            return asi_settings::text::Widen(line);
        };
        config.show = [](const std::wstring& line)
        {
            // The engine's startup and error notices use the same Windows message-box path.
            MessageBoxW(WindowedModeWrapper::GameHWND, line.c_str(), L"Knight Rider Fusion Fix", MB_OK | MB_ICONINFORMATION);
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        auto main = hook::pattern("56 8B F1 57 8B 7C 24 0C 8B 46 10 48 83 F8 0F 0F 87");
        MainHook = safetyhook::create_inline(main.get_first(), Main);
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
    }
}

class MainLoopEvents
{
public:
    MainLoopEvents()
    {
        WFP::onGameProcessEvent() += []()
        {
            KnightRiderCamera::BeginFrame();
        };
    }
} mainLoopEvents;

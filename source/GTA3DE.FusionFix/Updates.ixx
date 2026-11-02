module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import Build;
import Unreal;

// MAIN.CheckForUpdates: see Shared/RWGTA/UpdateCheck.hpp. The notice is the game's help box.
namespace
{
    const int* gameState = nullptr;
    const uint8_t* cutsceneRunning = nullptr;
    void (*showHelp)(FString*, bool) = nullptr;
    SafetyHookInline textGetHook;
    std::wstring noticeText;
    constexpr wchar_t noticeKey[] = L"FF_UPDATE";

    // CText::Get: the help box shows texts by key
    const wchar_t* TextGet(FString* key, void* table)
    {
        if (key && key->Data && std::wcscmp(key->Data, noticeKey) == 0)
            return noticeText.c_str();
        return textGetHook.ccall<const wchar_t*>(key, table);
    }
}

class Updates
{
public:
    Updates()
    {
        WFP::onInitEvent() += []()
        {
            gameState = reinterpret_cast<const int*>(GameStateAddress());
            showHelp = reinterpret_cast<decltype(showHelp)>(HelpMessageAddress());
            if (const auto running = hook::pattern("0F B6 15 ? ? ? ? F3 44 0F 10 0D ? ? ? ? F3 44 0F 10 05").get_first<uint8_t>(3))
                cutsceneRunning = reinterpret_cast<const uint8_t*>(injector::ReadRelativeOffset(running).as_int());

            RWGTA::UpdateCheck::Config config;
            config.pluginId = "GTA3DE.FusionFix";
            config.fileVersion = rsc_FileVersion;
            config.settings = &FixSettings();
            config.playable = []()
            {
                return *gameState == GS_PLAYING_GAME && !*cutsceneRunning;
            };
            config.show = [](const std::wstring& line)
            {
                noticeText = line;
                if (!textGetHook)
                    if (const auto textGet = TextGetAddress())
                        textGetHook = safetyhook::create_inline(textGet, TextGet);
                if (!textGetHook)
                    return;
                static std::wstring key = noticeKey;
                FString message;
                message.Data = key.data();
                message.Count = message.Max = static_cast<int>(key.size() + 1);
                showHelp(&message, false);
            };
            RWGTA::UpdateCheck::Init(std::move(config));

            WFP::onGameProcessEvent() += []()
            {
                RWGTA::UpdateCheck::Tick();
            };
            WFP::onShutdownEvent() += []()
            {
                RWGTA::UpdateCheck::OnExit();
            };
        };
    }
} Updates;

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
    void (*showHelp)(FString*, bool, int, int, int) = nullptr;
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
            cutsceneRunning = reinterpret_cast<const uint8_t*>(injector::ReadRelativeOffset(hook::pattern("0F B6 15 ? ? ? ? 84 D2 75 ? 0F B6 05 ? ? ? ? 48 69 C8 58 01 00 00").get_first<uint8_t>(3)).as_int());

            RWGTA::UpdateCheck::Config config;
            config.pluginId = "GTAVCDE.FusionFix";
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
                    textGetHook = safetyhook::create_inline(TextGetAddress(), TextGet);
                static std::wstring key = noticeKey;
                FString message;
                message.Data = key.data();
                message.Count = message.Max = static_cast<int>(key.size() + 1);
                showHelp(&message, false, 0, 0, 0);
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

#include "stdafx.h"
#include "FixSettings.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

namespace rdrFlashIntroHandler
{
    struct FlashIntroHandler
    {
        char unk[17];
        char bLegalsCompleted;
    };

    SafetyHookInline FlashIntroHandlerHook{};
    void __fastcall Update(FlashIntroHandler* smp_Instance)
    {
        // The startup logos count as finished; the title screen follows right away
        smp_Instance->bLegalsCompleted = 1;
    }
}

void Init()
{
    bool bSkipIntro = FixSettings().Bool("MAIN.SkipIntro");

    if (bSkipIntro)
    {
        // rdrFlashIntroHandler::Update
        auto pattern = hook::pattern("40 56 57 41 57 48 83 EC 30 48 89 6C 24 58 45 33 FF 40 32 ED 48 8B F9 81 79 08 E8 03 00 00");
        rdrFlashIntroHandler::FlashIntroHandlerHook = safetyhook::create_inline(pattern.get_first(), rdrFlashIntroHandler::Update);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        FixSettings();
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}

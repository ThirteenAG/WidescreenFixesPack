#include "stdafx.h"
#include <utility/Scan.hpp>

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
        smp_Instance->bLegalsCompleted = 1;
    }
}

void Init()
{
    CIniReader iniReader("");
    bool bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;

    if (bSkipIntro)
    {
        auto LegalsCompletedRef = utility::find_function_from_string_ref(GetModuleHandleW(NULL), "LegalsCompleted");
        if (LegalsCompletedRef)
        {
            auto func_start = utility::find_function_start_with_call(LegalsCompletedRef.value());
            if (func_start)
                rdrFlashIntroHandler::FlashIntroHandlerHook = safetyhook::create_inline(func_start.value(), rdrFlashIntroHandler::Update);
        }
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init);
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

    }
    return TRUE;
}

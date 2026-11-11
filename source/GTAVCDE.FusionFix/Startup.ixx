module;

#include "stdafx.h"

export module Startup;

import Build;
import Settings;
import Unreal;

namespace
{
    SafetyHookInline legalScreenHook;
    bool startupComplete = false;
    const int* gameState = nullptr;
    const uint8_t* loginFlags = nullptr;
    bool(*resumeGame)(void*) = nullptr;
    uintptr_t ShouldShow(void* screen, void* frame, uint8_t* result)
    {
        const auto native = legalScreenHook.ccall<uintptr_t>(screen, frame, result);
        if (result)
            *result = 0;
        return native;
    }
}

class StartupModule
{
public:
    StartupModule()
    {
        WFP::onInitEvent() += []()
        {
            if (Settings.skipIntro)
                legalScreenHook = safetyhook::create_inline(LegalScreenAddress(), ShouldShow);
            if (!Settings.skipMenu)
                return;
            const auto update = InterfaceUpdateAddress();
            const auto resume = ResumeGameAddress();
            gameState = reinterpret_cast<const int*>(GameStateAddress());
            loginFlags = reinterpret_cast<const uint8_t*>(LoginFlagsAddress());
            resumeGame = reinterpret_cast<bool(*)(void*)>(resume);
            static auto updateHook = safetyhook::create_mid(update,
                [](SafetyHookContext& context)
            {
                if (startupComplete)
                    return;
                // Ask every frame until the game leaves the front end: early requests are ignored
                if (*gameState == GS_FRONTEND && (*loginFlags & 2) != 0)
                    resumeGame(reinterpret_cast<void*>(context.rcx));
                else if (*gameState > GS_FRONTEND)
                    startupComplete = true;
            });
        };
    }
} StartupModuleInstance;

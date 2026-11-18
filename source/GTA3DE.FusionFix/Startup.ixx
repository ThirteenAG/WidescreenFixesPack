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

    // Resuming without any save leaves the front end with no player and crashes the game
    bool AnySaveExists()
    {
        PWSTR documents = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &documents)))
            return true;
        const auto root = std::filesystem::path(documents) / L"Rockstar Games" / L"GTA III Definitive Edition";
        CoTaskMemFree(documents);
        const auto hasSave = [](const std::filesystem::path& folder)
        {
            std::error_code error;
            for (const auto& entry : std::filesystem::directory_iterator(folder, error))
            {
                const auto name = entry.path().filename().wstring();
                if (name.starts_with(L"GTA3sf") && entry.path().extension() == L".sav" && entry.is_regular_file(error) && entry.file_size(error) > 0)
                    return true;
            }
            return false;
        };
        if (hasSave(root / L"SaveGames"))
            return true;
        std::error_code error;
        for (const auto& profile : std::filesystem::directory_iterator(root / L"Profiles", error))
            if (profile.is_directory(error) && hasSave(profile.path()))
                return true;
        return false;
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
            if (!Settings.skipMenu || !AnySaveExists())
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

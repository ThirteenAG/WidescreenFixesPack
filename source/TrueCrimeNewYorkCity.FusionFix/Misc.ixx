module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Misc;

import ComVars;
import Overlay;
import WidescreenHUD;

int32_t nLanguage;
int32_t __cdecl SetLanguage(LPCSTR lpValueName)
{
    return nLanguage;
}

namespace TCDataPath
{
    // The game's install-path routine reserves 32 DWORDs for the ANSI path.
    using NativePath = std::array<char, 128>;
    NativePath directory{};

    std::optional<NativePath> Encode(const std::wstring& path)
    {
        const bool utf8 = GetACP() == CP_UTF8;
        const DWORD flags = utf8 ? WC_ERR_INVALID_CHARS : WC_NO_BEST_FIT_CHARS;
        BOOL substituted = FALSE;
        auto replacement = utf8 ? nullptr : &substituted;
        const int length = WideCharToMultiByte(CP_ACP, flags, path.c_str(), -1, nullptr, 0, nullptr, replacement);
        if (substituted || length <= 1 || length > int(directory.size()))
            return std::nullopt;

        NativePath encoded{};
        if (WideCharToMultiByte(CP_ACP, flags, path.c_str(), -1, encoded.data(), int(encoded.size()), nullptr, replacement) != length || substituted)
            return std::nullopt;
        return encoded;
    }

    std::optional<NativePath> Resolve(const std::filesystem::path& executable)
    {
        if (!executable.has_parent_path())
            return std::nullopt;
        const auto data = (executable.parent_path() / L"Data").wstring();
        if (const auto encoded = Encode(data))
            return encoded;

        const DWORD size = GetShortPathNameW(data.c_str(), nullptr, 0);
        if (!size)
            return std::nullopt;
        std::wstring shortPath(size, L'\0');
        const DWORD length = GetShortPathNameW(data.c_str(), shortPath.data(), size);
        if (!length || length >= size)
            return std::nullopt;
        shortPath.resize(length);
        return Encode(shortPath);
    }

    std::optional<NativePath> ResolveExecutable()
    {
        constexpr DWORD MaxPath = 32768;
        for (DWORD capacity = MAX_PATH;; capacity = std::min(capacity * 2, MaxPath))
        {
            std::wstring path(capacity, L'\0');
            const DWORD size = GetModuleFileNameW(nullptr, path.data(), capacity);
            if (!size)
                return std::nullopt;
            if (size < capacity)
            {
                path.resize(size);
                return Resolve(path);
            }
            if (capacity == MaxPath)
                return std::nullopt;
        }
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            nLanguage = settings.Int("MAIN.Language");
            bool bHighResolutionShadows = settings.Bool("MISC.HighResolutionShadows");

            const auto data = TCDataPath::ResolveExecutable();
            if (!data)
            {
                MessageBoxW(nullptr, L"The game's data path is too long or contains characters the game cannot read. "
                    L"Move the game to a shorter path using basic Latin characters and try again.",
                    L"True Crime: New York City Fusion Fix", MB_OK | MB_ICONERROR);
                ExitProcess(ERROR_BAD_PATHNAME);
            }
            TCDataPath::directory = *data;

            auto pattern = hook::pattern("89 88 ? ? ? ? 0F B6 8A ? ? ? ? 88 88 ? ? ? ? 0F B6 8A ? ? ? ? 88 88 ? ? ? ? 0F B6 8A ? ? ? ? 88 88 ? ? ? ? 0F B6 8A ? ? ? ? 88 88 ? ? ? ? 0F B6 8A");
            static auto CacheMissionIDs = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
            {
                CurrentCaseName = *(int*)(regs.eax + 0xD9C);
                CurrentMissionName = *(int*)(regs.eax + 0xDA0);
            });

            // One request to fusionfix.io per start when MAIN.CheckForUpdates is on; FusionFixHub opens on its update
            // page when the game closes.
            RWGTA::UpdateCheck::Config config;
            config.pluginId = "TrueCrimeNewYorkCity.FusionFix";
            config.fileVersion = rsc_FileVersion;
            config.settings = &FixSettings();
            config.playable = []() { return pD3DDevice && !bCutscene && HudPlaying() && !TCOverlay::BlocksInput(); };
            config.format = TCOverlay::FormatUpdateText;
            config.show = TCOverlay::ShowUpdate;
            RWGTA::UpdateCheck::Init(std::move(config));

            // Load the data from the folder next to the exe instead of the registry install path
            pattern = hook::pattern("B9 20 00 00 00 8D 7C 24 18 F3 AB 8D 44 24 0C"); //0x496F27
            struct RegHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = 0x20;
                    regs.edi = (regs.esp + 0x18);

                    std::memcpy(reinterpret_cast<void*>(regs.edi), TCDataPath::directory.data(), TCDataPath::directory.size());
                }
            }; injector::MakeInline<RegHook>(pattern.get_first(0), pattern.get_first(11));
            injector::MakeJMP(pattern.get_first(11), hook::pattern("8D 44 24 18 8D 50 01").count(2).get(1).get<uintptr_t>(0), true); //0x496FD8

            if (nLanguage >= 0)
            {
                // The registry read behind both language getters
                auto pattern = hook::pattern("E8 ? ? ? ? 8B 04 85 ? ? ? ? 83 C4 04 C3"); //0x495E95
                injector::MakeJMP(injector::GetBranchDestination(pattern.get_first()), SetLanguage, true);
            }

            if (bHighResolutionShadows)
            {
                auto pattern = hook::pattern("68 00 01 00 00 68 00 01 00 00 6A 06 B9 ? ? ? ? E8 ? ? ? ? 83 3D ? ? ? ? ? 75 ? 83 3D ? ? ? ? ? 75 ? B9 ? ? ? ? E8 ? ? ? ? 6A 01");
                injector::WriteMemory(pattern.get_first(1), 256 * 3, true);
                injector::WriteMemory(pattern.get_first(6), 256 * 3, true);

                pattern = hook::pattern("68 00 01 00 00 68 00 01 00 00 6A 06 8D 8C 24");
                injector::WriteMemory(pattern.get_first(1), 256 * 3, true);
                injector::WriteMemory(pattern.get_first(6), 256 * 3, true);

                pattern = hook::pattern("68 00 01 00 00 68 00 01 00 00 6A 06 B9 ? ? ? ? E8 ? ? ? ? 83 3D ? ? ? ? ? 75 ? 83 3D ? ? ? ? ? 75 ? B9 ? ? ? ? E8 ? ? ? ? 51");
                injector::WriteMemory(pattern.get_first(1), 256 * 3, true);
                injector::WriteMemory(pattern.get_first(6), 256 * 3, true);
            }

            // Shadows distance fix
            {
                pattern = hook::pattern("68 ? ? ? ? 0F 59 E1");
                injector::WriteMemory(pattern.get_first(1), 10.0f * 10.0f, true);
            }
        };

        WFP::onGameProcessEvent() += []()
        {
            RWGTA::UpdateCheck::Tick();
        };
    }
} Misc;

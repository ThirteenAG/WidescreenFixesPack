module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Misc;

import ComVars;
import x_helpersmfc;

void SetStartupSave(std::string_view path)
{
    if (path.empty() || path.size() >= MAX_PATH)
        return;
    auto destination = szLoadGameAtStartup.get_ptr();
    std::memcpy(destination, path.data(), path.size());
    destination[path.size()] = '\0';
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            static bool bUseGameFolderForSavegames = settings.Bool("MISC.UseGameFolderForSavegames");
            if (bUseGameFolderForSavegames)
            {
                auto pattern = hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 40 04 68");
                injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x85, true); //0x41D14C
            }

            // Losing the focus pauses the game, unless it keeps running in the background
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 8B CF C6 87 82 00 00 00 00");
                static auto pKillFocusCall = (uintptr_t)pattern.get_first(0); //0x404935
                static auto KillProgramFocusHook = safetyhook::create_mid(pKillFocusCall, [](SafetyHookContext& regs)
                {
                    static auto& nPauseOnFocusLoss = FixSettings().Int("MAIN.PauseOnFocusLoss");
                    if (!nPauseOnFocusLoss)
                        regs.eip = pKillFocusCall + 5;
                });
            }

            // The native loop waits while unfocused. A loading screen must keep advancing even if focus was lost earlier.
            {
                auto pattern = hook::pattern("8A 86 DB 00 00 00 84 C0 75 ? E8 ? ? ? ? 84 C0 74");
                static auto LoadingFocusHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
                {
                    if (!(regs.eax & 0xFFu) && IsLoading())
                        regs.eax = (regs.eax & ~0xFFu) | 1u;
                });
            }

            static int32_t nLoadSaveSlot = settings.Int("MISC.LoadSaveSlot");
            if (nLoadSaveSlot == -2 || nLoadSaveSlot == -3 || (nLoadSaveSlot >= 0 && nLoadSaveSlot <= 999))
            {
                szLoadGameAtStartup.SetAddress(*hook::get_pattern<char*>("BA ? ? ? ? 2B D1 8A 01 88 04 0A", 1));
                if (!szLoadGameAtStartup)
                {
                    auto pattern = hook::pattern("E8 ? ? ? ? 8B 48 04 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8D 45 18 50");
                    static auto AfxGetModuleState = injector::GetBranchDestination(pattern.get_first(0), true); //0x4D2DB2
                    static auto GetProfileStringA = injector::GetBranchDestination(pattern.get_first(27), true); //0x4D2ECC
                    static auto aLastSavedGameF = *pattern.get_first<char*>(14); // 4E8F44
                    static auto aSaveGame = *pattern.get_first<char*>(19); // 4E62D4
                    static auto aSavegames = *hook::get_pattern<char*>("8B 15 ? ? ? ? 89 17 A1 ? ? ? ? 8B 55 10 89 47 04", 2);
                    static auto MaxPayne2Saveg = *hook::get_pattern<char*>("8B 15 ? ? ? ? 89 17 A1 ? ? ? ? 89 47 04", 2);
                    pattern = hook::pattern("89 97 ? ? ? ? FF 15 ? ? ? ? FF 15 ? ? ? ? E8 ? ? ? ? 8D 50 01");
                    struct SaveGameHook
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            *(uint32_t*)(regs.edi + 0xE8) = regs.edx;

                            if (nLoadSaveSlot == -2)
                            {
                                void* pStr = nullptr;
                                auto _this = injector::stdcall<void* ()>::call(AfxGetModuleState);
                                injector::thiscall<void(void* _this, void* out, char const* a1, char const* a2, char const* a3)>::call(GetProfileStringA, *(void**)((uint32_t)_this + 4), &pStr, (char const*)aSaveGame, (char const*)aLastSavedGameF, "");
                                if (pStr)
                                    SetStartupSave(static_cast<const char*>(pStr));
                            }
                            else
                            {
                                std::filesystem::path directory;
                                if (bUseGameFolderForSavegames)
                                    directory = GetExeModulePath() / aSavegames;
                                else
                                {
                                    char documents[MAX_PATH]{};
                                    if (!SHGetSpecialFolderPathA(nullptr, documents, CSIDL_PERSONAL, FALSE))
                                        return;
                                    directory = std::filesystem::path(documents) / MaxPayne2Saveg;
                                }

                                WIN32_FIND_DATAA entry{};
                                const auto search = FindFirstFileA((directory / "*.mp2s").string().c_str(), &entry);
                                if (search == INVALID_HANDLE_VALUE)
                                    return;

                                std::string selected;
                                FILETIME newest{};
                                do
                                {
                                    if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                        continue;
                                    if (nLoadSaveSlot >= 0)
                                    {
                                        const auto stem = std::filesystem::path(entry.cFileName).stem().string();
                                        if (stem.size() != 11 || _strnicmp(stem.c_str(), "savegame", 8) != 0)
                                            continue;
                                        int slot = -1;
                                        const auto end = stem.data() + stem.size();
                                        const auto parsed = std::from_chars(stem.data() + 8, end, slot);
                                        if (parsed.ec != std::errc{} || parsed.ptr != end || slot != nLoadSaveSlot)
                                            continue;
                                        selected = entry.cFileName;
                                        break;
                                    }
                                    if (selected.empty() || CompareFileTime(&entry.ftLastWriteTime, &newest) > 0)
                                    {
                                        selected = entry.cFileName;
                                        newest = entry.ftLastWriteTime;
                                    }
                                } while (FindNextFileA(search, &entry));
                                FindClose(search);
                                if (!selected.empty())
                                    SetStartupSave((directory / selected).string());
                            }
                        }
                    }; injector::MakeInline<SaveGameHook>(pattern.get_first(0), pattern.get_first(6)); //0x4187C3
                }
            }

            //savegame date format
            static auto fmt = settings.String("MISC.SaveStringFormat");
            auto pattern = hook::pattern("68 ? ? ? ? 8D 54 24 18 68 ? ? ? ? 52"); //41CA8D
            injector::WriteMemory(pattern.get_first(1), fmt.data(), true);

            // Skip intro: the logo movie (startup.bik) at startup is skipped the way the -skipstartup switch does it
            if (settings.Bool("MAIN.SkipIntro"))
            {
                pattern = hook::pattern("E8 ? ? ? ? 84 C0 75 3E 8B 8F A8 00 00 00"); // MaxPayneCommandLine::skipStartup()
                injector::WriteMemory<uint8_t>(pattern.get_first(7), 0xEB, true); // jnz -> jmp //0x4188EA
            }
        };
    }
} Misc;

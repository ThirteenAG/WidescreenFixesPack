module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Misc;

import ComVars;
import x_helpersmfc;

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
                                std::string_view LastSavedGameFilename{ (char*)pStr };
                                if (!LastSavedGameFilename.empty())
                                    injector::WriteMemoryRaw(szLoadGameAtStartup.get_ptr(), (void*)LastSavedGameFilename.data(), LastSavedGameFilename.size(), true);
                            }
                            else
                            {
                                char buffer[MAX_PATH];
                                if (bUseGameFolderForSavegames)
                                {
                                    GetModuleFileNameA(NULL, buffer, MAX_PATH);
                                    *strrchr(buffer, '\\') = '\0';
                                    strcat_s(buffer, "\\");
                                    strcat_s(buffer, aSavegames);
                                    strcat_s(buffer, "\\");
                                }
                                else
                                {
                                    SHGetSpecialFolderPathA(0, buffer, 5, false);
                                    strcat_s(buffer, "\\");
                                    strcat_s(buffer, MaxPayne2Saveg);
                                    strcat_s(buffer, "\\");
                                }

                                auto nSaveNum = -1;
                                std::string SFPath(buffer);

                                WIN32_FIND_DATAA fd{};
                                HANDLE File = FindFirstFileA(std::string(SFPath + "*.mp2s").c_str(), &fd);
                                FILETIME LastWriteTime{};
                                std::string LastSavedFilename;

                                if (File != INVALID_HANDLE_VALUE)
                                {
                                    do
                                    {
                                        std::string str(fd.cFileName);
                                        auto n = str.find_first_of("0123456789");

                                        if (nLoadSaveSlot >= 0)
                                        {
                                            if (n != std::string::npos)
                                            {
                                                nSaveNum = std::atoi(&str[n]);
                                                if (nSaveNum == nLoadSaveSlot)
                                                {
                                                    SFPath += str;
                                                    injector::WriteMemoryRaw(szLoadGameAtStartup.get_ptr(), SFPath.data(), SFPath.size(), true);
                                                    return;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            if (n != std::string::npos && (LastSavedFilename.empty() || CompareFileTime(&fd.ftLastWriteTime, &LastWriteTime) >= 0))
                                            {
                                                LastWriteTime = fd.ftLastWriteTime;
                                                LastSavedFilename = str;
                                            }
                                        }
                                    } while (FindNextFileA(File, &fd));
                                    FindClose(File);
                                }

                                if (!LastSavedFilename.empty() && nLoadSaveSlot < 0)
                                {
                                    SFPath += LastSavedFilename;
                                    injector::WriteMemoryRaw(szLoadGameAtStartup.get_ptr(), SFPath.data(), SFPath.size(), true);
                                }
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

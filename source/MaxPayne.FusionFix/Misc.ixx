module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <stdexcept>

export module Misc;

import ComVars;
import e2mfc;
import input;

// Fugitive's adaptive difficulty rates each level by the player's deaths, average health and play
// time in it, counted on the player character, and moves the difficulty a step up or down when the
// level ends. Loading a save creates the player character anew with these at 0, and dying always
// ends in loading one, so deaths were never counted and health and time only covered the stretch
// since the last load: the difficulty went up after nearly every level and stayed at the hardest
// step. The counters now carry over loads within the same level.
export namespace AdaptiveDifficulty
{
    struct LevelPerformance
    {
        uint8_t* pLevel = nullptr; // MaxPayne_GameMode::LEVEL_SETTINGS of the level the counters are from
        int32_t nDeaths = 0;
        float fHealthSum = 0.0f;
        float fPlayTime = 0.0f;
        float fPlayTimeFraction = 0.0f;
    } Performance;

    // Every frame the game view renders, a death included
    void Track(uint8_t* pGameMode)
    {
        auto pPlayer = MaxPayne_GameMode::GetPlayerCharacter(pGameMode);
        auto pLevel = *(uint8_t**)(pGameMode + MaxPayne_GameMode::LEVEL_SETTINGS);
        if (!pPlayer || !pLevel)
            return;

        Performance.pLevel = pLevel;
        Performance.nDeaths = *(int32_t*)(pPlayer + X_Character::DEATHS);
        Performance.fHealthSum = *(float*)(pPlayer + X_Character::HEALTH_SUM);
        Performance.fPlayTime = *(float*)(pPlayer + X_Character::PLAY_TIME);
        Performance.fPlayTimeFraction = *(float*)(pPlayer + X_Character::PLAY_TIME_FRACTION);
    }

    SafetyHookInline shLoad = {};
    bool __fastcall load(uint8_t* pGameMode, void* edx, void* pMemoryFile)
    {
        auto performance = Performance; // before loading tears the level down
        bool bResult = shLoad.unsafe_fastcall<bool>(pGameMode, edx, pMemoryFile);

        auto pPlayer = MaxPayne_GameMode::GetPlayerCharacter(pGameMode);
        if (pPlayer && performance.pLevel && performance.pLevel == *(uint8_t**)(pGameMode + MaxPayne_GameMode::LEVEL_SETTINGS))
        {
            *(int32_t*)(pPlayer + X_Character::DEATHS) = performance.nDeaths;
            *(float*)(pPlayer + X_Character::HEALTH_SUM) = performance.fHealthSum;
            *(float*)(pPlayer + X_Character::PLAY_TIME) = performance.fPlayTime;
            *(float*)(pPlayer + X_Character::PLAY_TIME_FRACTION) = performance.fPlayTimeFraction;
        }

        return bResult;
    }
}

// Losing the focus pauses the game, unless it keeps running in the background
void(__fastcall* KillProgramFocus)(void* pApp, void* edx) = nullptr;
void(__fastcall* SetProgramFocus)(void* pApp, void* edx) = nullptr;
bool bSkipFocusGain = false;
void __fastcall KillProgramFocusHook(void* pApp, void* edx)
{
    static auto& nPauseOnFocusLoss = FixSettings().Int("MAIN.PauseOnFocusLoss");
    bSkipFocusGain = !nPauseOnFocusLoss;
    if (nPauseOnFocusLoss)
        KillProgramFocus(pApp, edx);
}

void __fastcall SetProgramFocusHook(void* pApp, void* edx)
{
    // Run native focus return only when its matching focus-loss handling ran.
    if (!std::exchange(bSkipFocusGain, false))
        SetProgramFocus(pApp, edx);
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
                auto pattern = hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 48 04 68 ? ? ? ? 56 89");
                injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x85, true); //0x40FCAB
            }

            // Focus lost, see KillProgramFocusHook
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 5E C2 08 00");
                if (pattern.size() != 2)
                    throw std::runtime_error("Focus signatures not found");
                SetProgramFocus = (decltype(SetProgramFocus))injector::MakeCALL(pattern.get(0).get<uintptr_t>(0), SetProgramFocusHook, true).get();
                KillProgramFocus = (decltype(KillProgramFocus))injector::MakeCALL(pattern.get(1).get<uintptr_t>(0), KillProgramFocusHook, true).get(); //0x40D29B
            }

            static int32_t nLoadSaveSlot = settings.Int("MISC.LoadSaveSlot");
            if (nLoadSaveSlot == -2 || nLoadSaveSlot == -3 || (nLoadSaveSlot >= 0 && nLoadSaveSlot <= 999))
            {
                szLoadGameAtStartup.SetAddress(*hook::get_pattern<char*>("BF ? ? ? ? F3 A5 8B C8 83 E1 03 F3 A4 5F 5E C3", 1));
                if (!szLoadGameAtStartup)
                {
                    auto pattern = hook::pattern("E8 ? ? ? ? 8B 40 04 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8D 55 10 52");
                    static auto AfxGetModuleState = injector::GetBranchDestination(pattern.get_first(0), true); //0x76F77E
                    static auto GetProfileStringA = injector::GetBranchDestination(pattern.get_first(29), true); //0x76F7B4
                    static auto aLastSavedGameF = *pattern.get_first<char*>(14); // 8680EC
                    static auto aSaveGame = *pattern.get_first<char*>(19); // 866FDC
                    static auto aSavegames = *hook::get_pattern<char*>("BF ? ? ? ? F2 AE F7 D1 2B F9 8B D1 83 C9 FF 8B F7 8B FB F2 AE 8B 45 08 8B CA C1 E9 02 4F", 1);
                    static auto MaxPayne2Saveg = *hook::get_pattern<char*>("BF ? ? ? ? 75 05 BF ? ? ? ? 83 C9 FF 33 C0 F2 AE F7 D1 2B F9 8B D1", 1);
                    pattern = hook::pattern("89 87 ? ? ? ? E8 ? ? ? ? A2 ? ? ? ? E8");
                    struct SaveGameHook
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            *(uint32_t*)(regs.edi + 0xCC) = regs.eax;

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

                                WIN32_FIND_DATAA fd;
                                HANDLE File = FindFirstFileA(std::string(SFPath + "*.mps").c_str(), &fd);
                                FILETIME LastWriteTime = fd.ftLastWriteTime;

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
                                            if (CompareFileTime(&fd.ftLastWriteTime, &LastWriteTime) >= 0)
                                            {
                                                LastWriteTime = fd.ftLastWriteTime;
                                                std::string str(fd.cFileName);
                                                if (n != std::string::npos)
                                                {
                                                    nSaveNum = std::atoi(&str[n]);
                                                }
                                            }
                                        }
                                    } while (FindNextFileA(File, &fd));
                                    FindClose(File);
                                }

                                if (nSaveNum >= 0 && nLoadSaveSlot < 0)
                                {
                                    char buffer[12];
                                    sprintf_s(buffer, "%03d", nSaveNum);
                                    SFPath += fd.cFileName;
                                    auto offset = SFPath.end() - std::strlen("000.mps");
                                    SFPath.replace(offset, offset + 3, buffer);
                                    injector::WriteMemoryRaw(szLoadGameAtStartup.get_ptr(), SFPath.data(), SFPath.size(), true);
                                }
                            }
                        }
                    }; injector::MakeInline<SaveGameHook>(pattern.get_first(0), pattern.get_first(6)); //0x415EBD
                }
            }

            auto pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 81 EC 14 02 00 00 53 55 33 DB 56 57 8B E9"); // MaxPayne_GameMode::load
            AdaptiveDifficulty::shLoad = safetyhook::create_inline(pattern.get_first(), AdaptiveDifficulty::load); //0x454230

            // Loading screens cover the 4:3 area and leave the previous frame on the sides
            pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 8B 09 FF 15 ? ? ? ? 8B 15 ? ? ? ? 8B 0A FF 15"); // X_ProgressBar::updateProgressBar, before P_Driver::endScene
            static auto X_ProgressBarupdateProgressBarHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
            {
                RefreshScreenResolution();
                auto nWidth = static_cast<int32_t>(Screen.fHudOffsetReal);
                DrawPillarboxBars(nWidth, nWidth);
                UpdateVibration();
            }); //582A18

            //screenshots aspect ratio
            static float fScreenShotHeight = 0.0f;
            pattern = hook::pattern("89 93 ? ? ? ? D9 05 ? ? ? ? D9 05 ? ? ? ? D9 9B");
            static auto off_6301B6 = pattern.get_first(32);
            struct SaveScrHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.ebx + 0x10E) = regs.edx;
                    if (!fScreenShotHeight && Screen.fAspectRatio)
                    {
                        fScreenShotHeight = **(float**)off_6301B6 * ((4.0f / 3.0f) / Screen.fAspectRatio);
                        injector::WriteMemory(off_6301B6, &fScreenShotHeight, true);
                    }
                }
            }; injector::MakeInline<SaveScrHook>(pattern.get_first(0), pattern.get_first(6)); //630196

            //savegame date format
            static auto fmt = settings.String("MISC.SaveStringFormat");
            pattern = hook::pattern("68 ? ? ? ? 8D 54 24 24 68 ? ? ? ? 52"); //411091
            injector::WriteMemory(pattern.get_first(1), fmt.data(), true);

            // Skip intro: the logo movie (intro.mpg) at startup is skipped the way the -skipstartup switch does it
            if (settings.Bool("MAIN.SkipIntro"))
            {
                pattern = hook::pattern("E8 ? ? ? ? 84 C0 75 41 E8 ? ? ? ? 85 C0 74 38 E8"); // MaxPayneCommandLine::skipStartup()
                injector::WriteMemory<uint8_t>(pattern.get_first(7), 0xEB, true); // jnz -> jmp //0x41678D
            }
        };
    }
} Misc;

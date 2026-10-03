#include "stdafx.h"

import ComVars;
import e2mfc;
import e2_d3d8_driver_mfc;
import x_basicmodesmfc;
import x_gameobjectsmfc;
import x_helpersmfc;
import x_modesmfc;
import sndmfc;
import x_inputmfc;
import xidi;
import PostFX;

SafetyHookInline shsub_404B20 = {};
int __fastcall sub_404B20(int* CWnd, void* edx, char a2)
{
    // border size to 0
    CWnd[27] = 0;
    CWnd[28] = 0;

    HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX info = { sizeof(MONITORINFOEX) };
    GetMonitorInfo(monitor, &info);
    DEVMODE devmode = {};
    devmode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
    DWORD DesktopX = devmode.dmPelsWidth;
    DWORD DesktopY = devmode.dmPelsHeight;

    // center window position
    CWnd[29] = (int)(((float)DesktopX / 2.0f) - ((float)CWnd[25] / 2.0f));
    CWnd[30] = (int)(((float)DesktopY / 2.0f) - ((float)CWnd[26] / 2.0f));

    return shsub_404B20.unsafe_fastcall<int>(CWnd, edx, a2);
}

void ReadSettings()
{
    CIniReader iniReader("");
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));
    Screen.fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    if (Screen.fFOVFactor <= 0.0f) { Screen.fFOVFactor = 1.0f; }
    Screen.bGraphicNovelMode = iniReader.ReadInteger("MAIN", "GraphicNovelMode", 1) != 0;
    Cinematic::nCutsceneBorders = std::clamp(iniReader.ReadInteger("MAIN", "CutsceneBorders", Cinematic::Both), (int32_t)Cinematic::Off, (int32_t)Cinematic::Both);
    Cinematic::bNoBorderAnimation = iniReader.ReadInteger("MAIN", "NoCutsceneBorderAnimation", 0) != 0;
}

void Init()
{
    CIniReader iniReader("");
    static bool bUseGameFolderForSavegames = iniReader.ReadInteger("MISC", "UseGameFolderForSavegames", 0) != 0;
    if (bUseGameFolderForSavegames)
    {
        auto pattern = hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 40 04 68");
        injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x85, true); //0x41D14C
    }

    bool bAltTab = iniReader.ReadInteger("MISC", "AllowAltTabbingWithoutPausing", 0) != 0;
    if (bAltTab)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 8B CF C6 87 82 00 00 00 00");
        injector::MakeNOP(pattern.get_first(0), 5, true); //0x404935
    }

    static int32_t nLoadSaveSlot = iniReader.ReadInteger("MISC", "LoadSaveSlot", -1);
    if (nLoadSaveSlot == -2 || nLoadSaveSlot == -3 || (nLoadSaveSlot >= 0 && nLoadSaveSlot <= 999))
    {
        static auto unk_556860 = *hook::get_pattern<void*>("BA ? ? ? ? 2B D1 8A 01 88 04 0A", 1);
        if (*(uint8_t*)unk_556860 == 0)
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
                            injector::WriteMemoryRaw(unk_556860, (void*)LastSavedGameFilename.data(), LastSavedGameFilename.size(), true);
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
                        HANDLE File = FindFirstFileA(std::string(SFPath + "*.mp2s").c_str(), &fd);
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
                                            injector::WriteMemoryRaw(unk_556860, SFPath.data(), SFPath.size(), true);
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
                            char buffer[5]; sprintf(buffer, "%03d", nSaveNum);
                            SFPath += fd.cFileName;
                            auto offset = SFPath.end() - std::strlen("000.mp2s");
                            SFPath.replace(offset, offset + 3, buffer);
                            injector::WriteMemoryRaw(unk_556860, SFPath.data(), SFPath.size(), true);
                        }
                    }
                }
            }; injector::MakeInline<SaveGameHook>(pattern.get_first(0), pattern.get_first(6)); //0x4187C3
        }
    }

    // MP_GameMode render function, where the game view gets rendered
    auto pattern = hook::pattern("C6 86 48 01 00 00 01 FF 15 ? ? ? ? 8B 8E A0 10 00 00 FF 15");
    static auto MP_GameModeRenderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) //0x451B93
    {
        MP_GameMode::pInstance = (uint8_t*)regs.esi;
        bGameViewRendered = true;
        Cinematic::UpdateBorders(MP_GameMode::pInstance);
    });

    // MP_GameMode destructor. The game mode is deleted on quit, while the progress bar still draws.
    pattern = hook::pattern("C7 06 ? ? ? ? C7 46 19 ? ? ? ? C7 86 BE 00 00 00 ? ? ? ? C7 44 24 24 19 00 00 00");
    static auto MP_GameModeDestructorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) //0x46A412
    {
        if ((uint8_t*)regs.esi == MP_GameMode::pInstance)
            MP_GameMode::pInstance = nullptr;
    });

    // Cutscene borders: every height multiplier read in MP_HUDMode's render function, which moves
    // subtitles clear of the letterbox bars and then draws them
    pattern = hook::pattern("8B 8E AE 00 00 00 33 DB 89 5C 24 ? E8"); // subtitle offset check
    injector::MakeCALL(pattern.get_first(12), Cinematic::GetBordersHeightMultiplier, true); //0x488A01
    pattern = hook::pattern("8B 8E AE 00 00 00 E8 ? ? ? ? D8 2D ? ? ? ? 8B 8E 07 02 00 00"); // subtitle offset
    injector::MakeCALL(pattern.get_first(6), Cinematic::GetBordersHeightMultiplier, true); //0x488A1D
    pattern = hook::pattern("FF 52 0C 8B 8E AE 00 00 00 E8"); // subtitle offset restore check
    injector::MakeCALL(pattern.get_first(9), Cinematic::GetBordersHeightMultiplier, true); //0x488B23
    pattern = hook::pattern("8B 8E AE 00 00 00 E8 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 05 0F 8A ? ? ? ? 8B 15"); // bars check
    injector::MakeCALL(pattern.get_first(6), Cinematic::GetBordersHeightMultiplier, true); //0x488C05
    pattern = hook::pattern("8B 8E AE 00 00 00 D9 5C 24 10 E8"); // bar height
    injector::MakeCALL(pattern.get_first(10), Cinematic::GetBordersHeightMultiplier, true); //0x488C68

    // MP_HUDMode's render function updates the fade layer right before drawing it
    pattern = hook::pattern("8B 8E F0 02 00 00 D9 1C 24 E8");
    static auto MaxPayne_HUDFadeLayerHook = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs) //0x4889F0
    {
        if (regs.ecx)
            MaxPayne_HUDFadeLayer::pSprite = *(uint8_t**)(regs.ecx + 0xAD);
    });

    // Graphic novels: key toggles between the original framing and the whole page as large as the screen allows
    pattern = hook::pattern("56 57 8B F1 E8 ? ? ? ? 8B 7C 24 0C 8D 46 37 50 8D 4F 38 E8"); // MaxPayne_GraphicNovelPage::show
    MaxPayne_GraphicNovelPage::shShow = safetyhook::create_inline(pattern.get_first(), MaxPayne_GraphicNovelPage::show); //0x4862D0

    // Graphic novel cursor bounds, see UpdateCursorBounds
    pattern = hook::pattern("52 68 00 00 20 44 E8 ? ? ? ? 51 D9 1C 24 6A 00 E8 ? ? ? ? D9 5C 24 ? 8B 44 24 ? 50 68 00 00 F0 43 E8 ? ? ? ? 51 D9 1C 24 6A 00 E8"); // MP_GraphicNovelMode::update
    injector::MakeCALL(pattern.get_first(6), ClampCursorRight, true); //0x484FB5
    injector::MakeCALL(pattern.get_first(17), ClampCursorLeft, true);
    injector::MakeCALL(pattern.get_first(36), ClampCursorBottom, true);
    injector::MakeCALL(pattern.get_first(47), ClampCursorTop, true);

    InitPostFX();

    // Post-processing (pain and bullet time): the scene is warped with a grid covering the render target
    pattern = hook::pattern("8B 8E 9C 00 00 00 52 68 ? ? ? ? E8");
    static auto PostProcessWarpHook = safetyhook::create_mid(pattern.get_first(12), [](SafetyHookContext& regs) //0x47AB87
    {
        KeepOriginalProjection(*(void**)(regs.ecx + 0x10));
    });

    //savegame date format
    static auto fmt = iniReader.ReadString("MISC", "SaveStringFormat", "%a, %b %d %Y, %H:%M");
    pattern = hook::pattern("68 ? ? ? ? 8D 54 24 18 68 ? ? ? ? 52"); //41CA8D
    injector::WriteMemory(pattern.get_first(1), fmt.data(), true);

    bool BorderlessWindowedMode = iniReader.ReadInteger("MAIN", "BorderlessWindowedMode", 1) != 0;
    if (BorderlessWindowedMode)
    {
        pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC 14 53 55");
        shsub_404B20 = safetyhook::create_inline(pattern.get_first(), sub_404B20);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        ReadSettings();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 40 04 68"));
        CallbackHandler::RegisterCallback(L"E2MFC.dll", InitE2MFC);
        CallbackHandler::RegisterCallback(L"X_GameObjectsMFC.dll", InitX_GameObjectsMFC);
        CallbackHandler::RegisterCallback(L"X_ModesMFC.dll", InitX_ModesMFC);
        CallbackHandler::RegisterCallback(L"X_HelpersMFC.dll", InitX_HelpersMFC);
        CallbackHandler::RegisterCallback(L"E2_D3D8_DRIVER_MFC.dll", []() { InitE2_D3D8_DRIVER_MFC(); InitPostFXDriver(); });
        CallbackHandler::RegisterModuleUnloadCallback(L"E2_D3D8_DRIVER_MFC.dll", []() { BorderlessWindowedHook.reset(); shDllMainHook.reset(); ShutdownPostFXDriver(); });
        CallbackHandler::RegisterCallback(L"X_BasicModesMFC.dll", InitX_BasicModesMFC);
        CallbackHandler::RegisterCallback(L"sndmfc.dll", InitSNDMFC);
        CallbackHandler::RegisterCallback(L"X_Inputmfc.dll", InitInput);
        CallbackHandler::RegisterCallback(L"Xidi.32.dll", InitXidi);
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

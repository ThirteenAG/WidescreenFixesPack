#include "stdafx.h"

import ComVars;
import e2mfc;
import e2_d3d8_driver_mfc;
import x_basicmodesmfc;
import x_gameobjectsmfc;
import x_helpersmfc;
import x_modesmfc;

SafetyHookInline shsub_404B20 = {};
int __fastcall sub_404B20(int* CWnd, void* edx, char a2)
{
    CWnd[27] = 0;
    CWnd[28] = 0;
    return shsub_404B20.unsafe_fastcall<int>(CWnd, edx, a2);
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

    static int32_t nCutsceneBorders = iniReader.ReadInteger("MAIN", "CutsceneBorders", 1);
    if (nCutsceneBorders)
    {
        auto f = [](uintptr_t _this, uintptr_t edx) -> float
        {
            if (nCutsceneBorders > 1)
                return *(float*)(*(uintptr_t*)_this + 4692) * (1.0f / ((4.0f / 3.0f) / Screen.fAspectRatio));
            else
                return 1.0f;
        };
        auto pattern = hook::pattern("E8 ? ? ? ? D8 2D ? ? ? ? D8");
        injector::MakeCALL(pattern.get_first(), static_cast<float(__fastcall*)(uintptr_t, uintptr_t)>(f), true); //0x488C68
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

    auto pattern = hook::pattern("8B 4C 24 18 51 8B 4F 04 FF 15");
    struct X_CharacterSetSniperZoomOnHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = *(uint32_t*)(regs.esp + 0x18);
            regs.ecx = *(uint32_t*)(regs.edi + 0x04);
            if (regs.edx == 1)
                Screen.bIsSniperZoomOn = true;
            else
                Screen.bIsSniperZoomOn = false;
        }
    }; injector::MakeInline<X_CharacterSetSniperZoomOnHook>(pattern.get_first(0), pattern.get_first(8)); //428FB2
    injector::WriteMemory<uint8_t>(pattern.get_first(6), 0x52i8, true); //push edx

    pattern = hook::pattern("8B 8B AD 00 00 00 DF E0 F6 C4 41");
    struct FadingHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.ebx + 0xAD);
            if (*(float*)(regs.esp + 0x34) == 0.0f)
                Screen.bIsFading = false;
            else
                Screen.bIsFading = true;
        }
    }; injector::MakeInline<FadingHook>(pattern.get_first(0), pattern.get_first(6)); //4872C1 

    pattern = hook::pattern("C7 44 24 ? ? ? ? ? 75 07 8A 46 41 84 C0");
    struct SkyboxHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esp + 0x18) = 0;
            Screen.bIsSkybox = true;
        }
    }; injector::MakeInline<SkyboxHook1>(pattern.get_first(0), pattern.get_first(8));

    pattern = hook::pattern("C7 44 24 ? ? ? ? ? FF 15 ? ? ? ? 8B 4C 24 10 5E 64 89 0D ? ? ? ? 83 C4 18 C2 08 00");
    struct SkyboxHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.esp + 0x18) = -1;
            Screen.bIsSkybox = false;
        }
    }; injector::MakeInline<SkyboxHook2>(pattern.get_first(0), pattern.get_first(8));

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
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 40 04 68"));
        CallbackHandler::RegisterCallback(L"E2MFC.dll", InitE2MFC);
        CallbackHandler::RegisterCallback(L"X_GameObjectsMFC.dll", InitX_GameObjectsMFC);
        CallbackHandler::RegisterCallback(L"X_ModesMFC.dll", InitX_ModesMFC);
        CallbackHandler::RegisterCallback(L"X_HelpersMFC.dll", InitX_HelpersMFC);
        CallbackHandler::RegisterCallback(L"E2_D3D8_DRIVER_MFC.dll", InitE2_D3D8_DRIVER_MFC);
        CallbackHandler::RegisterModuleUnloadCallback(L"E2_D3D8_DRIVER_MFC.dll", []() { EndSceneHook.reset(); BorderlessWindowedHook.reset(); shDllMainHook.reset(); });
        CallbackHandler::RegisterCallback(L"X_BasicModesMFC.dll", InitX_BasicModesMFC);
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
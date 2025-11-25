#include "stdafx.h"
#include "dxsdk\dx8\d3d8.h"
#include "dxsdk\d3dvtbl.h"
#include <shlobj.h>

import ComVars;
import e2mfc;
import e2_d3d8_driver_mfc;
import rlmfc;
import sndmfc;

void Init()
{
    CIniReader iniReader("");
    static bool bUseGameFolderForSavegames = iniReader.ReadInteger("MISC", "UseGameFolderForSavegames", 0) != 0;
    if (bUseGameFolderForSavegames)
    {
        auto pattern = hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 48 04 68 ? ? ? ? 56 89");
        injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x85, true); //0x40FCAB
    }

    bool bAltTab = iniReader.ReadInteger("MISC", "AllowAltTabbingWithoutPausing", 0) != 0;
    if (bAltTab)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 5E C2 08 00");
        injector::MakeNOP(pattern.count(2).get(1).get<uintptr_t>(0), 5, true); //0x40D29B
    }

    static int32_t nCutsceneBorders = iniReader.ReadInteger("MAIN", "CutsceneBorders", 1);
    if (nCutsceneBorders)
    {
        auto f = [](uintptr_t _this, uintptr_t edx) -> float
        {
            if (nCutsceneBorders > 1)
                return *(float*)(_this + 12) * (1.0f / ((4.0f / 3.0f) / Screen.fAspectRatio));
            else
                return 1.0f;
        };
        auto pattern = hook::pattern("E8 ? ? ? ? EB ? D9 05 ? ? ? ? 8B CF");
        injector::MakeCALL(pattern.get_first(), static_cast<float(__fastcall*)(uintptr_t, uintptr_t)>(f), true); //0x4565B8
    }

    static int32_t nLoadSaveSlot = iniReader.ReadInteger("MISC", "LoadSaveSlot", -1);
    if (nLoadSaveSlot == -2 || nLoadSaveSlot == -3 || (nLoadSaveSlot >= 0 && nLoadSaveSlot <= 999))
    {
        static auto unk_8A6490 = *hook::get_pattern<void*>("BF ? ? ? ? F3 A5 8B C8 83 E1 03 F3 A4 5F 5E C3", 1);
        if (*(uint8_t*)unk_8A6490 == 0)
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
                            injector::WriteMemoryRaw(unk_8A6490, (void*)LastSavedGameFilename.data(), LastSavedGameFilename.size(), true);
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
                                            injector::WriteMemoryRaw(unk_8A6490, SFPath.data(), SFPath.size(), true);
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
                            auto offset = SFPath.end() - std::strlen("000.mps");
                            SFPath.replace(offset, offset + 3, buffer);
                            injector::WriteMemoryRaw(unk_8A6490, SFPath.data(), SFPath.size(), true);
                        }
                    }
                }
            }; injector::MakeInline<SaveGameHook>(pattern.get_first(0), pattern.get_first(6)); //0x415EBD
        }
    }

    //FOV
    static bool bRestoreCutsceneFOV = iniReader.ReadInteger("MAIN", "RestoreCutsceneFOV", 0) != 0;
    auto pattern = hook::pattern("A0 ? ? ? ? 84 C0 0F 85 ? ? ? ? 8B 86");
    X_Crosshair::sm_bCameraPathRunning.SetAddress(*pattern.get_first<bool*>(1));

    static auto FOVHook = [](uintptr_t _this, uintptr_t edx) -> float
    {
        if (bRestoreCutsceneFOV && X_Crosshair::sm_bCameraPathRunning && !Screen.bDrawBordersForCameraOverlay)
        {
            Screen.fFieldOfView = *(float*)(_this + 0x58);
            return Screen.fFieldOfView;
        }

        float f = AdjustFOV(*(float*)(_this + 0x58) * 57.295776f, Screen.fAspectRatio) * Screen.fFOVFactor;
        Screen.fFieldOfView = f / 57.295776f;
        return Screen.fFieldOfView;
    };

    pattern = hook::pattern("E8 ? ? ? ? D8 8B 3C 12"); //0x50B9E0
    auto sub_50B9E0 = injector::GetBranchDestination(pattern.get_first(), true);
    pattern = hook::pattern("E8 ? ? ? ?");
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        auto addr = pattern.get(i).get<uint32_t>(0);
        auto dest = injector::GetBranchDestination(addr, true);
        if (dest == sub_50B9E0)
            injector::MakeCALL(addr, static_cast<float(__fastcall*)(uintptr_t, uintptr_t)>(FOVHook), true);
    }

    pattern = hook::pattern("D8 4C 24 38 D9 54 24 38 D8 1D");
    struct FOVCheck
    {
        void operator()(injector::reg_pack& regs)
        {
            float f = *(float*)(regs.esp + 0x38);
            _asm {fmul dword ptr[f]}
            _asm {fst dword ptr[f]}
            *(float*)(regs.esp + 0x38) = (f > 3.14f) ? 3.14f : f;
        }
    }; injector::MakeInline<FOVCheck>(pattern.get_first(0), pattern.get_first(8)); // 0x4563D4

    //pattern = hook::pattern("E8 ? ? ? ? D9 5C 24 14 8B CF E8"); // 0x45650D
    //injector::MakeCALL(pattern.get_first(0), sub_50B9E0, true); // restoring cutscene FOV

    //auto CutsceneFOVHook = [](uintptr_t _this, uintptr_t edx) -> float
    //{
    //    return *(float*)(_this + 88) + ((((Screen.fHudOffsetReal / Screen.fWidth)) / *(float*)(_this + 88)) * 2.0f);
    //};
    //injector::MakeCALL(pattern.get_first(0), static_cast<float(__fastcall*)(uintptr_t, uintptr_t)>(CutsceneFOVHook), true);

    pattern = hook::pattern("C6 87 ? ? ? ? ? E8 ? ? ? ? 8B 4D F4");
    struct CameraOverlayHook
    {
        void operator()(injector::reg_pack& regs)
        {
            Screen.bDrawBordersForCameraOverlay = false;
            *(uint8_t*)(regs.edi + 0x14E) = 1;

            auto a1 = *(uint32_t*)(regs.esp + 0x10);
            auto a2 = *(uint32_t*)(regs.esp + 0x14);
            auto a3 = *(uint32_t*)(regs.esp + 0x18);
            auto a4 = *(uint32_t*)(regs.esp + 0x1C);

            //what happens here is check for some camera coordinates or angles
            if ((a1 == 0x3FE842CF && a4 == 0x3FE842CF) ||											//1.81 https://i.imgur.com/A7wRrgk.gifv
                (a1 == 0x3FC00000 && a2 == 0x4096BEF4 && a3 == 0xC003936E && a4 == 0x3FC00000) ||   //1.5 https://i.imgur.com/ouRpysL.jpg
                (a1 == 0xBFAAE30E && a2 == 0xBFC2B1AA && a3 == 0x3EC2E382 && a4 == 0xBFAAE30E) ||   //-1.33505 https://i.imgur.com/JGNdm6y.jpg
                (a1 == 0x403F7470 && a2 == 0xC067ED50 && a3 == 0x40424DE0 && a4 == 0x403F7470)      // 2.99148  https://i.imgur.com/hj5FsXp.png
                )
            {
                Screen.bDrawBordersForCameraOverlay = true;
            }
        }
    }; injector::MakeInline<CameraOverlayHook>(pattern.get_first(0), pattern.get_first(7)); // 0x672EB1

    pattern = hook::pattern("05 40 01 00 00 84 C9 89 50 24");
    struct X_ProgressBarUpdateProgressBarHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax += 0x140;
            Screen.bDrawBorders = true;
        }
    }; injector::MakeInline<X_ProgressBarUpdateProgressBarHook>(pattern.get_first(0)); //5829D1

    //screenshots aspect ratio
    static float fScreenShotHeight = 0.0f;
    pattern = hook::pattern("89 93 ? ? ? ? D9 05 ? ? ? ? D9 05 ? ? ? ? D9 9B");
    static auto off_6301B6 = pattern.get_first(32);
    struct SaveScrHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebx + 0x10E) = regs.edx;
            if (!fScreenShotHeight)
            {
                fScreenShotHeight = **(float**)off_6301B6 * ((4.0f / 3.0f) / Screen.fAspectRatio);
                injector::WriteMemory(off_6301B6, &fScreenShotHeight, true);
            }
        }
    }; injector::MakeInline<SaveScrHook>(pattern.get_first(0), pattern.get_first(6)); //630196

    //savegame date format
    static auto fmt = iniReader.ReadString("MISC", "SaveStringFormat", "%a, %b %d %Y, %H:%M");
    pattern = hook::pattern("68 ? ? ? ? 8D 54 24 24 68 ? ? ? ? 52"); //411091
    injector::WriteMemory(pattern.get_first(1), fmt.data(), true);

    // Workaround for safetyhook hanging
    //IATHook::Replace(GetModuleHandleA("msvcrt.dll"), "KERNEL32.DLL", std::forward_as_tuple("ExitProcess", static_cast<void(__stdcall*)(UINT)>([](UINT uExitCode)
    //{
    //    TerminateProcess(GetCurrentProcess(), uExitCode);
    //})));
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 48 04 68 ? ? ? ? 56 89"));
        CallbackHandler::RegisterCallback(L"E2MFC.dll", InitE2MFC);
        CallbackHandler::RegisterCallback(L"E2_D3D8_DRIVER_MFC.dll", InitE2_D3D8_DRIVER_MFC);
        CallbackHandler::RegisterModuleUnloadCallback(L"E2_D3D8_DRIVER_MFC.dll", []() { EndSceneHook.reset(); shDllMainHook.reset(); });
        CallbackHandler::RegisterCallback(L"rlmfc.dll", InitRLMFC);
        CallbackHandler::RegisterCallback(L"sndmfc.dll", InitSNDMFC);
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
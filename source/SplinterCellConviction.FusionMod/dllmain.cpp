#include "stdafx.h"
#include <LEDEffects.h>

float gVisibility = 1.0f;
int32_t gBlacklistIndicators = 0;
bool bDisableBlackAndWhiteFilter = false;
bool bBlacklistControlScheme = true;

bool (WINAPI* GetOverloadedFilePathA)(const char* lpFilename, char* out, size_t out_size) = nullptr;

void OverrideForBlacklistControls(const char* path)
{
    static const std::string blacklistINI = "blacklist.ini";
    static const std::string LocalizationMenus = "Localization\\Menus";

    auto path_view = std::string_view(path);
    auto dest = const_cast<char*>(path);
    if (path_view == "ActionScheme.ini")
    {
        if (GetOverloadedFilePathA(blacklistINI.c_str(), nullptr, 0))
            strcpy_s(dest, blacklistINI.length() + 1, blacklistINI.c_str());
    }
    else if (path_view.contains(LocalizationMenus))
    {
        auto replace_view = [](std::string_view str, std::string_view pattern, std::string_view replacement) -> auto
        {
            return str | std::views::split(pattern) | std::views::join_with(replacement);
        };

        auto new_path = replace_view(path_view, LocalizationMenus, "Blacklist\\Menus") | std::ranges::to<std::string>();
        if (GetOverloadedFilePathA(new_path.c_str(), nullptr, 0))
            strcpy_s(dest, new_path.length() + 1, new_path.c_str());
    }
}

injector::hook_back<void (__cdecl*)(float a1, float a2, int a3)> hb_8330DB;
void __cdecl sub_8330DB(float a1, float a2, int a3)
{
    gVisibility = a1;

    if (!bDisableBlackAndWhiteFilter)
        return hb_8330DB.fun(a1, a2, a3);
}

injector::hook_back<void(__cdecl*)(void* a1, int a2, int a3)> hb_100177B7;
void __cdecl sub_100177B7(void* a1, int a2, int a3)
{
    if (gBlacklistIndicators) //enables bloom in dark areas
    {
        if (gVisibility == 1.0f)
            return hb_100177B7.fun(a1, a2, a3);
        else
            return;
    }
    return hb_100177B7.fun(a1, a2, a3);
}

// Distortion during movement (sonar)
injector::hook_back<void(__fastcall*)(void* self, int edx)> hb_1002581C;
void __fastcall sub_1002581C(void* self, int edx)
{
    //return hb_1002581C.fun(self, edx);
}

SafetyHookInline sub_78C666_hook{};
int __cdecl sub_78C666(char* a1, int a2)
{
    return sub_78C666_hook.ccall<int>(a1, a2);
}

namespace FFileManagerArc
{
    SafetyHookInline shLookup{};
    void* __fastcall Lookup(void* pFFileManagerArc, void* edx, const char* path)
    {
        if (bBlacklistControlScheme)
            OverrideForBlacklistControls(path);

        if (GetOverloadedFilePathA(path, nullptr/*s.data()*/, 0/*s.size()*/))
            return nullptr;
        return shLookup.fastcall<void*>(pFFileManagerArc, edx, path);
    }
}

namespace FFileManagerLinear
{
    int* dword_13A334C = nullptr;
    SafetyHookInline shLookup{};
    void* __fastcall Lookup(void* pFFileManagerLinear, void* edx, const char* path, int a3)
    {
        if (bBlacklistControlScheme)
            OverrideForBlacklistControls(path);

        if (GetOverloadedFilePathA(path, nullptr/*s.data()*/, 0/*s.size()*/))
        {
            int cur = *dword_13A334C;
            *dword_13A334C = 1;
            auto ret = shLookup.fastcall<void*>(pFFileManagerLinear, edx, path, a3);
            *dword_13A334C = cur;
            return ret;
        }
        return shLookup.fastcall<void*>(pFFileManagerLinear, edx, path, a3);
    }

    static int curdword_13A3348 = 0;
    int* dword_13A3348 = nullptr;
    static injector::hook_back<void(__fastcall*)(void*, void*, const char*)> hbsub_43A943;
    void __fastcall sub_43A943(void* _this, void* edx, const char* path)
    {
        curdword_13A3348 = *dword_13A3348;

        if (bBlacklistControlScheme)
            OverrideForBlacklistControls(path);

        if (GetOverloadedFilePathA(path, nullptr/*s.data()*/, 0/*s.size()*/))
        {
            *dword_13A3348 = 0;
        }

        return hbsub_43A943.fun(_this, edx, path);
    }
}

int WhichPrecisionModeNeeded = 0;
int TogglePrecisionMode = 0;
int TogglePrecisionModeOn = 0;
int TogglePrecisionModeOff = 0;
static injector::hook_back<void(__fastcall*)(int*, void*, const char*, int)> hb458CD3;
void __fastcall fnTogglePrecisionMode(int* out, void* edx, const char* name, int a3)
{
    hb458CD3.fun(&TogglePrecisionModeOn, edx, "TogglePrecisionModeOn", a3);
    hb458CD3.fun(&TogglePrecisionModeOff, edx, "TogglePrecisionModeOff", a3);
    hb458CD3.fun(out, edx, name, a3);
    TogglePrecisionMode = *out;
}

int DetectB2W = 0;
void __fastcall fnDetectB2W(int* out, void* edx, const char* name, int a3)
{
    hb458CD3.fun(out, edx, name, a3);
    DetectB2W = *out;
}

int StopDetectB2W = 0;
void __fastcall fnStopDetectB2W(int* out, void* edx, const char* name, int a3)
{
    hb458CD3.fun(out, edx, name, a3);
    StopDetectB2W = *out;
}

BOOL __fastcall sub_56C0AD(uint32_t* _this, void* edx)
{
    auto aimcheck = (*(uint8_t*)((uint8_t*)_this + 0xA42));

    // run speed, disable run when player wants to aim
    if (WhichPrecisionModeNeeded == TogglePrecisionModeOn)
    {
        *(float*)&_this[753] = 200.0f;
    }
    else if (WhichPrecisionModeNeeded == TogglePrecisionModeOff)
    {
        *(float*)&_this[753] = 1000000.0f;
    }
    
    enum AimType
    {
        eNormal = 4,
        eFromCover = 5,
        eFirstPerson = 12,
    };

    if ((WhichPrecisionModeNeeded == TogglePrecisionModeOn) && (aimcheck == eNormal || aimcheck == eFromCover || aimcheck == eFirstPerson))
        return true;
    if ((WhichPrecisionModeNeeded == TogglePrecisionModeOff) && (aimcheck != eNormal && aimcheck != eFromCover && aimcheck != eFirstPerson))
        return true;

    return ((_this[655] & 0x100000) != 0) || ((_this[656] & 0x200000) != 0);
}

SafetyHookInline shsub_46E388{};
int __fastcall sub_46E388(void* a1, void* edx, void* a2, int a3, int a4, int fullscreen, int a6)
{
    return shsub_46E388.fastcall<int>(a1, edx, a2, a3, a4, 0, a6);
}

SafetyHookInline shWndProc{};
int __fastcall WndProc(HDC _this, void* edx, UINT Msg, int a3, unsigned int a4)
{
    auto ret = shWndProc.fastcall<int>(_this, edx, Msg, a3, a4);
    if (Msg == WM_STYLECHANGED)
        SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    return ret;
}

void Init()
{
    CIniReader iniReader("");
    //auto bWindowedMode = iniReader.ReadInteger("MAIN", "WindowedMode", 1) != 0;
    auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    auto bSkipSystemDetection = iniReader.ReadInteger("MAIN", "SkipSystemDetection", 1) != 0;
    bDisableBlackAndWhiteFilter = iniReader.ReadInteger("MAIN", "DisableBlackAndWhiteFilter", 0) != 0;
    auto bDisableCharacterLighting = iniReader.ReadInteger("MAIN", "DisableCharacterLighting", 0) != 0;
    bBlacklistControlScheme = iniReader.ReadInteger("CONTROLS", "BlacklistControlScheme", 1) != 0;
    auto bUnlockDLC = iniReader.ReadInteger("UNLOCKS", "UnlockDLC", 1) != 0;

    auto sLANHelperExePath = iniReader.ReadString("STARTUP", "LANHelperExePath", "");
    static auto sServerAddr = iniReader.ReadString("STARTUP", "ServerAddr", "127.0.0.1");

    //allow loading unpacked files
    {
        ModuleList dlls;
        dlls.Enumerate(ModuleList::SearchLocation::LocalOnly);
        for (auto& e : dlls.m_moduleList)
        {
            auto m = std::get<HMODULE>(e);
            if (IsModuleUAL(m)) {
                GetOverloadedFilePathA = (decltype(GetOverloadedFilePathA))GetProcAddress(m, "GetOverloadedFilePathA");
                break;
            }
        }

        if (GetOverloadedFilePathA)
        {
            auto pattern = hook::pattern("83 3D ? ? ? ? ? 56 8B F1 75 5C");
            FFileManagerLinear::dword_13A334C = *pattern.get_first<int*>(2);

            pattern = hook::pattern("39 1D ? ? ? ? 0F 84 ? ? ? ? 8B 86");
            FFileManagerLinear::dword_13A3348 = *pattern.get_first<int*>(2);

            pattern = hook::pattern("55 8B EC 83 EC 18 53 56 57 FF 75 08 8D 45 E8");
            FFileManagerArc::shLookup = safetyhook::create_inline(pattern.get_first(), FFileManagerArc::Lookup);
            
            pattern = hook::pattern("55 8B EC 83 EC 18 83 3D ? ? ? ? ? 56 8B F1 75 5C");
            FFileManagerLinear::shLookup = safetyhook::create_inline(pattern.get_first(), FFileManagerLinear::Lookup);

            pattern = hook::pattern("E8 ? ? ? ? 39 1D ? ? ? ? 0F 84 ? ? ? ? 8B 86");
            FFileManagerLinear::hbsub_43A943.fun = injector::MakeCALL(pattern.get_first(), FFileManagerLinear::sub_43A943, true).get();

            pattern = hook::pattern("8D 4D E4 E8 ? ? ? ? 8B C7 5F 5E 5B C9 C2 10 00");
            static auto restore = safetyhook::create_mid(pattern.get_first(8), [](SafetyHookContext& regs)
            {
                *FFileManagerLinear::dword_13A3348 = FFileManagerLinear::curdword_13A3348;
            });
        }
    }

    // causes crash on aim on coste level
    //if (bWindowedMode)
    //{
    //    auto pattern = hook::pattern("55 8B EC 83 EC 40 53 56 83 C8 FF");
    //    shsub_46E388 = safetyhook::create_inline(pattern.get_first(), sub_46E388);
    //    
    //    IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
    //        std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
    //        std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
    //        std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
    //        std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
    //        std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
    //        std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)
    //    );
    //
    //    pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 ? ? ? ? 8B 45 0C");
    //    shWndProc = safetyhook::create_inline(pattern.get_first(), WndProc);
    //}

    if (bSkipIntro)
    {
        auto pattern = hook::pattern("55 8D 6C 24 88 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 74 53 56 57 BE ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? E8 ? ? ? ? 50");
        injector::MakeRET(pattern.get_first());
    }

    if (bSkipSystemDetection)
    {
        auto pattern = hook::pattern("75 13 E8 ? ? ? ? 50 E8");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);
    }

    //if (bDisableBlackAndWhiteFilter) //light and shadow
    {
        auto pattern = hook::pattern("E8 ? ? ? ? D9 05 ? ? ? ? 83 C4 0C 33 F6 56 51");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 83 3D ? ? ? ? ? 0F 8C ? ? ? ? 7F 0D 83 3D ? ? ? ? ? 0F 86");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 83 3D ? ? ? ? ? 7C 77 7F 09 83 3D ? ? ? ? ? 76 6C");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("D9 5C 24 04 D9 40 08 D9 1C 24 E8 ? ? ? ? 83 C4 0C C3");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(10), sub_8330DB, true).get();
    }

    if (bDisableCharacterLighting)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 33 C9 38 48 1C");
        static auto CharacterLightingHook1 = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& ctx)
        {
            sub_78C666_hook.ccall<int>("l3d-char-lighting");
        });

        pattern = hook::pattern("E9 ? ? ? ? 8D 45 90 68 ? ? ? ? 50 E8 ? ? ? ? 85 C0 59 59 74 0B 81 C6 ? ? ? ? E9 ? ? ? ? 8D 45 88 50 68 ? ? ? ? FF 75 90 E8 ? ? ? ? 83 C4 0C 85 C0 74 12 F3 0F 10 45 ? F3 0F 11 86 ? ? ? ? E9 ? ? ? ? 8D 45 88");
        static auto CharacterLightingHook2 = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& ctx)
        {
            *(uint8_t*)ctx.esi ^= 1;
        });

        pattern = hook::pattern("55 8D AC 24 ? ? ? ? 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 85 ? ? ? ? 8B 85 ? ? ? ? 56 57 8B BD");
        sub_78C666_hook = safetyhook::create_inline(pattern.get_first(0), sub_78C666);
    }

    // controller scheme force X360
    {
        auto pattern = hook::pattern("66 81 FF ? ? 75 0E 66 81 FE ? ? 75 13");
        injector::MakeNOP(pattern.get_first(), 14, true);
    }

    if (bBlacklistControlScheme)
    {
        // aiming check
        auto pattern = hook::pattern("E8 ? ? ? ? 8B 4D F8 48");
        injector::MakeCALL(pattern.get_first(), sub_56C0AD, true);

        // add modes for aiming
        pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 10 8B 00 3B 05 ? ? ? ? 75 04");
        hb458CD3.fun = injector::MakeCALL(pattern.get_first(), fnTogglePrecisionMode, true).get();

        // check aiming mode
        pattern = hook::pattern("3B 05 ? ? ? ? 75 07 8B CE E8 ? ? ? ? 5F 5E 5B");
        static auto loc_56DC64 = (uintptr_t)pattern.get_first(15);
        struct GetPrecisionMode
        {
            void operator()(injector::reg_pack& regs)
            {
                if (regs.eax == TogglePrecisionMode || regs.eax == TogglePrecisionModeOn || regs.eax == TogglePrecisionModeOff)
                    WhichPrecisionModeNeeded = regs.eax;
                else
                    *(uintptr_t*)(regs.esp - 4) = loc_56DC64;
            }
        }; injector::MakeInline<GetPrecisionMode>(pattern.get_first(0), pattern.get_first(8));

        // cover modes
        pattern = hook::pattern("E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 1A 81 0D ? ? ? ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? BF");
        hb458CD3.fun = injector::MakeCALL(pattern.get_first(0), fnDetectB2W, true).get();
        pattern = hook::pattern("E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 1A 81 0D ? ? ? ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? BF");
        hb458CD3.fun = injector::MakeCALL(pattern.get_first(0), fnStopDetectB2W, true).get();

        // cover
        static bool bCoverStateStarted = false;
        static auto loc_56DBB3 = (uintptr_t)hook::get_pattern("3B 05 ? ? ? ? 75 13 8B 8E ? ? ? ? 8B 01");

        pattern = hook::pattern("3B 05 ? ? ? ? 75 0B 09 BE");
        struct CoverHook
        {
            void operator()(injector::reg_pack& regs)
            {
                if (regs.eax == DetectB2W || regs.eax == StopDetectB2W)
                {
                    static bool toggleState = false; // Toggle state for entering/exiting cover
                    static bool buttonWasPressed = false; // Tracks if the button was previously pressed
                    auto bInCover = (*(uint32_t*)(regs.esi + 0xA44) & 0x100);

                    // Check if the button is currently pressed
                    bool buttonIsPressed = (regs.eax == DetectB2W);

                    // Toggle the state only on the transition from not pressed to pressed
                    if (buttonIsPressed && !buttonWasPressed)
                    {
                        toggleState = !toggleState;

                        // Apply the toggle state to enter or exit cover
                        if (toggleState)
                        {
                            // Enter cover if not already in cover
                            if (!bInCover)
                            {
                                *(uint32_t*)(regs.esi + 0xA44) |= 0x100;
                            }
                        }
                        else
                        {
                            // Exit cover if currently in cover
                            if (bInCover)
                            {
                                *(uint32_t*)(regs.esi + 0xA44) &= ~0x100;
                            }
                        }
                    }
                    else if (!buttonIsPressed)
                    {
                        // If the button is released, ensure the player does not get stuck in a seeking cover state
                        if (!bCoverStateStarted)
                            *(uint32_t*)(regs.esi + 0xA44) &= ~0x100;
                    }

                    // Update the buttonWasPressed state for the next frame
                    buttonWasPressed = buttonIsPressed;
                    bCoverStateStarted = false;
                }
                else
                    *(uintptr_t*)(regs.esp - 4) = loc_56DBB3;
            }
        }; injector::MakeInline<CoverHook>(pattern.get_first(0), pattern.get_first(14));

        pattern = hook::pattern("81 88 ? ? ? ? ? ? ? ? 8B 4B 78");
        static auto UCoverNavStartState = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            bCoverStateStarted = true;
        });
    }

    if (bUnlockDLC)
    {
        auto pattern = hook::pattern("74 15 FF 74 24 08 8B CE E8 ? ? ? ? 8B 40 08");
        if (!pattern.empty())
            injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true); // UEPECInventory

        pattern = hook::pattern("74 15 FF 74 24 08 8B CE E8 ? ? ? ? 8B 40 04 F7 D0");
        if (!pattern.empty())
            injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true); // UEPECInventory

        pattern = hook::pattern("75 25 FF 75 FC");
        if (!pattern.empty())
            injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true); // MenuUniformSelect

        pattern = hook::pattern("8B 41 7C D1 E8 F7 D0");
        if (!pattern.empty())
        {
            injector::MakeNOP(pattern.get_first(), 10, true);
            static auto UEChallengeProfileExternal1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { regs.eax = 0; });
        }

        pattern = hook::pattern("8B 41 7C C1 E8 03");
        if (!pattern.empty())
        {
            injector::MakeNOP(pattern.get_first(), 11, true);
            static auto UEChallengeProfileExternal2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { regs.eax = 0; });
        }

        // MapConfigurationDLC.xml
        pattern = hook::pattern("0F 85 ? ? ? ? 39 7D EC");
        if (!pattern.empty())
            injector::MakeNOP(pattern.get_first(), 6, true);
        
        pattern = hook::pattern("0F 84 ? ? ? ? 8D 45 E8 50 8B CB");
        if (!pattern.empty())
            injector::MakeNOP(pattern.get_first(), 6, true);
        
        pattern = hook::pattern("7E 50 8B 45 E8");
        if (!pattern.empty())
            injector::MakeJMP(pattern.get_first(), hook::pattern("8B 0D ? ? ? ? 8B 01 57 FF 35 ? ? ? ? 57 68 ? ? ? ? FF 50 10 8B D8").get_first(), true);
    }

    if (!sLANHelperExePath.empty())
    {
        std::error_code ec;
        auto dsPath = std::filesystem::path(sLANHelperExePath);
        auto processPath = dsPath.is_absolute() ? dsPath : (GetExeModulePath() / dsPath);
        auto workingDir = std::filesystem::path(processPath).remove_filename();
        if (std::filesystem::exists(processPath, ec))
        {
            HANDLE hJob = CreateJobObject(nullptr, nullptr);
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = { };
            info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &info, sizeof(info));
            PROCESS_INFORMATION pi;
            if (CreateProcessInJobAsAdmin(hJob,
                processPath.c_str(),
                NULL,
                SW_HIDE,
                workingDir.c_str(), &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
    }

    if (!sServerAddr.empty())
    {
        auto pattern = hook::pattern("68 ? ? ? ? 8D 4D E0 E8 ? ? ? ? C7 45 ? ? ? ? ? E8");
        injector::WriteMemory(pattern.get_first(1), sServerAddr.data(), true);

        pattern = hook::pattern("68 ? ? ? ? 8B CE E8 ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? 8B C6");
        injector::WriteMemory(pattern.get_first(1), sServerAddr.data(), true);
    }
}

void InitLeadD3DRender()
{
    CIniReader iniReader("");
    auto bDisableDOF = iniReader.ReadInteger("MAIN", "DisableDOF", 1) != 0;
    auto bDisableBlackAndWhiteFilter = iniReader.ReadInteger("MAIN", "DisableBlackAndWhiteFilter", 0) != 0;
    auto bEnhancedSonarVision = iniReader.ReadInteger("MAIN", "EnhancedSonarVision", 0) != 0;
    gBlacklistIndicators = iniReader.ReadInteger("MAIN", "BlacklistIndicators", 0);

    if (bDisableDOF)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "E8 ? ? ? ? 8B 06 8B 08 83 C4 18 6A 00 68");
        injector::MakeNOP(pattern.get_first(0), 5);
    }

    if (bDisableBlackAndWhiteFilter)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "E8 ? ? ? ? 8B 06 8B 08 83 C4 34");
        hb_100177B7.fun = injector::MakeCALL(pattern.get_first(), sub_100177B7, true).get();
    }

    if (bEnhancedSonarVision)
    {
        static bool bNightVision = false;
        static auto loc_1002E95F = (uintptr_t)hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "89 B8 ? ? ? ? E8 ? ? ? ? 5F 5E 5B 83 C5 78 C9 C3").get_first(13);

        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "E8 ? ? ? ? 89 5D 78");
        hb_1002581C.fun = injector::MakeCALL(pattern.get_first(), sub_1002581C, true).get();

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "8B CE 88 86");
        struct SonarVisionHook
        {
            void operator()(injector::reg_pack& regs)
            {
                bNightVision = regs.eax & 0xff;
                //if (false)
                //    *(uint8_t*)(regs.esi + 0x642C) = regs.eax & 0xff;
                //else
                *(uint8_t*)(regs.esi + 0x642C) = 0;
            }
        }; injector::MakeInline<SonarVisionHook>(pattern.get_first(2), pattern.get_first(8));

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "80 B9 ? ? ? ? ? 0F 84 ? ? ? ? 56 57 E8 ? ? ? ? 8B 03 33 FF 47");
        struct NVCheck
        {
            void operator()(injector::reg_pack& regs)
            {
                if (!bNightVision)
                    *(uintptr_t*)(regs.esp - 4) = loc_1002E95F;
            }
        }; injector::MakeInline<NVCheck>(pattern.get_first(0), pattern.get_first(13));

        //DrawVisibleOpaque
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "68 ? ? ? ? 57 8D 43 0C");
        injector::MakeNOP(pattern.get_first(0), 17);
        
        //to do: add bNightVision check
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "75 15 8B 80");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);
        
        //to do: add bNightVision check
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "80 B8 ? ? ? ? ? 74 65");
        injector::MakeNOP(pattern.get_first(0), 7);
        
        //to do: add bNightVision check
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "8B 03 80 B8");
        injector::MakeNOP(pattern.get_first(2), 7);
    }
}

void InitLED()
{
    CIniReader iniReader("");
    auto bLightSyncRGB = iniReader.ReadInteger("LOGITECH", "LightSyncRGB", 1);

    if (bLightSyncRGB)
    {
        LEDEffects::Inject([]()
        {
            static auto fPlayerVisibility = gVisibility;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto gVisCmp = static_cast<float>(static_cast<int>(gVisibility * 10.0f)) / 10.0f;
            auto fPlVisCmp = static_cast<float>(static_cast<int>(fPlayerVisibility * 10.0f)) / 10.0f;
            
            if (fPlVisCmp > gVisCmp)
                fPlayerVisibility -= 0.05f;
            else if (fPlVisCmp < gVisCmp)
                fPlayerVisibility += 0.05f;
            
            constexpr auto minBrightness = 0.3f;
            constexpr auto maxBrightness = 1.0f;

            fPlayerVisibility = std::clamp(fPlayerVisibility, minBrightness, maxBrightness);
            
            auto [R, G, B] = LEDEffects::RGBtoPercent(255, 39, 26, gBlacklistIndicators ? fPlayerVisibility : ((maxBrightness + minBrightness) - fPlayerVisibility));
            LEDEffects::SetLighting(R, G, B, false, false, false);
        });
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        {
            std::error_code ec;
            auto injPath = std::filesystem::path("InjectorCLIx86.exe");
            auto processPath = GetExeModulePath() / injPath;
            auto workingDir = std::filesystem::path(processPath).remove_filename();
            if (std::filesystem::exists(processPath, ec))
            {
                auto cmd = processPath.wstring() + L" pid:" + std::to_wstring(GetCurrentProcessId()) + L" HookLibraryx86.dll";
                HANDLE hJob = CreateJobObject(nullptr, nullptr);
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = { };
                info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &info, sizeof(info));
                STARTUPINFO si = { sizeof(si) };
                PROCESS_INFORMATION pi;
                if (CreateProcessInJob(hJob,
                    NULL,
                    const_cast<wchar_t*>(cmd.c_str()),
                    nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                    nullptr, workingDir.c_str(), &si, &pi)) {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
        }

        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("D9 1C 24 E8 ? ? ? ? D9 5E 0C"));
        CallbackHandler::RegisterCallback(L"LeadD3DRender.dll", InitLeadD3DRender);
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitLED, hook::pattern("D9 1C 24 E8 ? ? ? ? D9 5E 0C"));
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

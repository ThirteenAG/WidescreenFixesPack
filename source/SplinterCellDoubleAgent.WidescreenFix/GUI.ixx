module;

#include <stdafx.h>

export module GUI;

import ComVars;

export enum class KnownHashes : uint32_t
{
    P_SaveLoadGame = 0xe1dd075c,
    P_Save = 0xd31fbbb2,
    P_LoadGame = 0xf9fec7eb,
    P_Difficulty = 0xbb54b1c8,
    P_MainMenu = 0x9da62dfb,
    P_Pause = 0x9318c61f,
    P_Options = 0x9ab9a8d7,
    P_Profile = 0xcbdbf85f,
    P_Training = 0xbe335729,
    Lst_Main = 0xd23e68f6,
    P_CheatMain = 0x971d7135,
    P_CheatSam = 0xd1178720,
    P_CheatPerformance = 0x663a17ab,
    P_CheatDebug = 0x70e11379,
    P_CheatLoadMap = 0x1fd22eb8,
    P_CheatPreSub = 0xcb73be55,
    P_CheatTrust = 0x1642b870,
    P_mission_1 = 0xb04b8c55,
    P_mission_2 = 0x2942ddef,
    P_CamControl = 0x6699e306,
    P_CamViewer = 0xff2b0b0f,
    P_endmission = 0x562da380,
    P_Interact = 0x4ad47d4c,
    P_ObjectiveReview = 0x081eca4c,
    P_Stealth_Statistics = 0x418edce8,
    P_Reward = 0x1d266337,
    P_OverallAchievement = 0x76252a34,
    Lst_Goals = 0xd948fae2,
    Lst_Slots = 0xd618d11d,
    P_Opsat = 0x8fc4da25,
    P_Map = 0x39fe126b,
    P_Profile_details = 0x3bc54a05,
    P_Equipment = 0xece6f283,
    P_Objectives = 0x6c8fb74b,
    P_Objectives_details = 0x246f7923,
    P_Datas = 0x8b9a58e8,
    P_DataFile = 0x4e6bbcc7,
    P_Profiles = 0xe0115896,
    P_Briefing = 0xe8f02806,
    P_HQ_Rules = 0xfb0ea0c5,
    P_opticcable = 0x2638f1c8,
    P_FullEquipment = 0xaf8183a3,
    P_QuickInventory_New = 0xce0a1185,
    P_Sniper = 0xfddf59af,
    P_MsgBox_Training = 0x0877d669,
    P_Video = 0x38458ede,
    P_Customize = 0x689aaf1c,
    P_Control = 0xa7577e1b,
    P_Control_P2 = 0xbe0d6052,
    P_Sound = 0xbc0c9776,
    P_ProfileManagement = 0xb44f8be3,
    P_CreateProfile = 0x19875f6f,
    P_Pre_LoadingScreen = 0xd2d17cce,
    P_Adjust_display = 0x0c9598fc,
    Standard = 0xe981b5d1,
    NextGen = 0x3bcb9b8e,
    Customize = 0xd5b50abf,
    P_MsgBox = 0x81792e2e,
    P_OptionPopup = 0xdbe91b11,
    LST_Options = 0xc4aaeafa,
    P_LoadingScreen = 0x0ee09cdd,
    progress = 0x2201f246,
    A_Cursor = 0x26994a5f,
    B_OK = 0xe156edf5,
    B_OKALONE = 0xea6caa6f,
    B_CANCEL = 0xcc2b20f0,
    L_Title = 0xcfcada23,
    L_MSG = 0xc9239996,
    P_Progress = 0x49202fe0,
    L_Operation = 0x82b3d761,
    A_Panel_Live = 0xc2ba1dc0,
    I_icon_Message = 0xb04407ae,
    I_icon_friendsrequest = 0x18111041,
    I_icon_gameinvite = 0xe8993399,
    L_username = 0x579bd8e1,
    B_Back = 0xa5a9e3cb,
    Back = 0xcdfc6e09,
    B_Cancel = 0x3b7fe240,
    Quit = 0xe39878ab,
    P_SavingScreen = 0xeb30567d,
    A_Button_1 = 0x3be1f955,
    A_Button_2 = 0xa2e8a8ef,
    A_Button_3 = 0xd5ef9879,
    A_Button_4 = 0x4b8b0dda,
    Text = 0x9bb908f9,
    Image = 0x04fc2b5b,
    Controller = 0x03af654a,
    P_Storage_Screen = 0x01cf3586,
    T_saving = 0x69cd27a4,
    T_infos2 = 0xf92973c2,
    A_Save_Infos = 0x852ea7bd,
    PH_InputProcess = 0x1a93be15,
};

namespace CMenusManager
{
    SafetyHookInline shDisplayMenu = {};
    void __fastcall DisplayMenu(int* menusManager, void* edx, uint32_t* entry, char show, char a4, char a5)
    {
        uint32_t hash = entry[1];
        IsMenuDisplayedCache[hash] = (show != 0);
        shDisplayMenu.unsafe_fastcall(menusManager, edx, entry, show, a4, a5);
    }

    export bool IsMenuDisplayed(uint32_t hash)
    {
        auto it = IsMenuDisplayedCache.find(hash);
        return (it != IsMenuDisplayedCache.end()) ? it->second : false;
    }

    export bool IsMenuDisplayed(KnownHashes hash)
    {
        return IsMenuDisplayed(std::to_underlying(hash));
    }

    export bool IsMenuDisplayed(const char* name)
    {
        if (!name)
            return false;

        uint32_t hash = crc32(0, name, std::strlen(name));
        return IsMenuDisplayed(hash);
    }

    // Helper to compute delta and bounds for X axis
    inline void GetXAxisBounds(int16_t& outDelta, int16_t& outVmin, int16_t& outVmax)
    {
        outDelta = static_cast<int16_t>(((600.0f * Screen.fAspectRatio) - 800.0f) / 2.0f);
        outVmin = static_cast<int16_t>(0 - outDelta);
        outVmax = static_cast<int16_t>(800 + outDelta);
    }

    // Helper to convert raw input to normalized [0, 1] range
    inline float NormalizeRawInputX(int16_t rawX)
    {
        return static_cast<float>(rawX) / static_cast<float>(Screen.Width);
    }

    inline float NormalizeRawInputY(int16_t rawY)
    {
        return static_cast<float>(rawY) / static_cast<float>(Screen.Height);
    }

    // Helper to convert normalized coordinates to game space
    inline int16_t NormalizedToGameX(float normalized, int16_t minGame, int16_t maxGame)
    {
        return static_cast<int16_t>(normalized * (maxGame - minGame) + minGame);
    }

    inline int16_t NormalizedToGameY(float normalized, int16_t minGame, int16_t maxGame)
    {
        return static_cast<int16_t>(normalized * (maxGame - minGame) + minGame);
    }

    // Helper to convert game coordinates back to raw input space
    inline int16_t GameToRawInputX(int16_t gameX, int16_t vmin, int16_t vmax)
    {
        float normalized = static_cast<float>(gameX - vmin) / static_cast<float>(vmax - vmin);
        return static_cast<int16_t>(normalized * Screen.Width);
    }

    inline int16_t GameToRawInputY(int16_t gameY, int16_t minGame, int16_t maxGame)
    {
        float normalized = static_cast<float>(gameY) / static_cast<float>(maxGame);
        return static_cast<int16_t>(normalized * Screen.Height);
    }
}

int16_t __cdecl ClampXAxisState(int16_t value, int16_t min, int16_t max)
{
    int16_t delta, vmin, vmax;
    CMenusManager::GetXAxisBounds(delta, vmin, vmax);

    if (Screen.bRawInputMouseForMenu)
    {
        float normalizedX = CMenusManager::NormalizeRawInputX(RawMouseCursorX);
        int16_t menuX = CMenusManager::NormalizedToGameX(normalizedX, vmin, vmax);
        return std::clamp(menuX, vmin, vmax);
    }
    else
        return std::clamp(value, int16_t(0 - delta), int16_t(800 + delta));
}

int16_t __cdecl ClampYAxisState(int16_t value, int16_t min, int16_t max)
{
    max += 15;

    if (Screen.bRawInputMouseForMenu)
    {
        float normalizedY = CMenusManager::NormalizeRawInputY(RawMouseCursorY);
        int16_t menuY = CMenusManager::NormalizedToGameY(normalizedY, min, max);
        return std::clamp(menuY, min, max);
    }
    else
        return std::clamp(value, min, max);
}

export void InitGUI()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 66 89 44 24 40");
    if (!pattern.empty())
        injector::MakeCALL(pattern.get_first(0), ClampXAxisState, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 66 89 44 24 42");
    if (!pattern.empty())
        injector::MakeCALL(pattern.get_first(0), ClampYAxisState, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "0F BF 42 02 85 C0");
    if (!pattern.empty())
    {
        struct CMenusManagerSetMousePositionHook
        {
            void operator()(injector::reg_pack& regs)
            {
                int16_t delta, vmin, vmax;
                CMenusManager::GetXAxisBounds(delta, vmin, vmax);

                int16_t clampedGameX = std::clamp(*(int16_t*)(regs.edx + 0), int16_t(0 - delta), int16_t(800 + delta));
                int16_t clampedGameY = std::clamp(*(int16_t*)(regs.edx + 2), int16_t(0), int16_t(585));

                if (Screen.bRawInputMouseForMenu)
                {
                    RawMouseCursorX = CMenusManager::GameToRawInputX(clampedGameX, vmin, vmax);
                    RawMouseCursorY = CMenusManager::GameToRawInputY(clampedGameY, 0, 585);
                }

                *(int16_t*)&regs.eax = clampedGameX;
                *(int16_t*)&regs.ecx = clampedGameY;
            }
        }; injector::MakeInline<CMenusManagerSetMousePositionHook>(pattern.get_first(0), pattern.get_first(35));
    }

    //Not hooked for raw input, probably no need:
    //USCMagmaInteraction::ProcessInputPressState
    //USCMagmaInteraction::ProcessInputReleaseState

    if (Screen.bRawInputMouseForCamera)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "F3 0F 11 03 F3 0F 10 45 E8");
        if (!pattern.empty())
        {
            injector::MakeNOP(pattern.get_first(0), 4);
            injector::MakeNOP(pattern.get_first(12), 4);

            static auto APlayerControllerTickHook = safetyhook::create_mid(pattern.get_first(16), [](SafetyHookContext& regs)
            {
                constexpr float CameraSensitivity = 100.0f;

                float scaledDeltaX = static_cast<float>(RawMouseDeltaX) / CameraSensitivity;
                float scaledDeltaY = static_cast<float>(RawMouseDeltaY) / CameraSensitivity;

                *(float*)regs.ebx = scaledDeltaX;
                *(float*)regs.edi = scaledDeltaY;

                RawMouseDeltaX = 0;
                RawMouseDeltaY = 0;
            });
        }
    }

    CMenusManager::shDisplayMenu = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?DisplayMenu@CMenusManager@@QAE_NPAVPage@magma@@_N1W4EMENUPRIORITY@@@Z"), CMenusManager::DisplayMenu);
}
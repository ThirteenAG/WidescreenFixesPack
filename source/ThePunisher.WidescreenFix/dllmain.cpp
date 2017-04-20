#include "stdafx.h"

#define _LOG
#ifdef _LOG
#include <intrin.h>  
#pragma intrinsic(_ReturnAddress)  
#include <fstream>
std::ofstream logfile;
uint32_t logit;
std::map<uintptr_t, uint32_t> retXmap;
std::map<uintptr_t, uint32_t> retYmap;
#endif // _LOG

enum HudCoords
{
    HEALTH_CLUSTER_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND,
    HEALTH_CLUSTER_BACKGROUND_COORDS__SET_ME_TO_MOVE_HEALTHSLAUGHTER_HUD,
    HEALTH_BAR_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND,
    HEALTH_BAR_WIDTH_HEIGHT_NEED_IT_SINCE_HEALTH_AND_ARMOR_ARE_IN_THE_SAME_METER,
    SLAUGHTER_METER_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND,
    SKULL_ICON_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND,
    USE_MESSAGE,
    MAIN_WEAPON_UPPER_LEFT_CORNER,
    WEAPON_ICON_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX,
    AMMO_COUNTER_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX,
    RESERVE_AMMO_BAR_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX,
    RETICLE_OFFSETS_FROM_CENTER_OF_SCREEN,
    HUMAN_SHIELD,
    THE_INTERROGATE_INTERACTIVE_HUD_ELEMENT,
    THE_INTERROGATE_CONTROL_EXPLAINATION,
    AMMO_PICKUP_MESSAGES,
    HUD_PERSONA_BACKGROUND,
    HUD_PERSONA_HEAD,
    FRONT_DAMAGE_INDICATOR_CENTER,
    LEFT_DAMAGE_INDICATOR_CENTER,
    BACK_DAMAGE_INDICATOR_CENTER,
    RIGHT_DAMAGE_INDICATOR_CENTER,
    HUD_MESSAGE_LOG_TEXT_AREA_WIDTH_HEIGHT_IGNORED,
    HUD_COUNTDOWN_TIMER,
    GENERIC_MESSAGE_BACKGROUND_BITMAP_COORDINATES,
    GENERIC_MESSAGE_TEXT_LOCATION,
    GENERIC_MESSAGE_TEXT_WIDTH_IN_PIXELS_LINE_1,
    POSITION_OF_OBJECTIVES_POPUP_FRAME,
    COMBO_SCORE_DISPLAY,
    LOADING_PROGRESS_BAR_NOTE_NOT_A_HUD_ITEM,
    TUTORIAL_MESSAGE,
    TUTORIAL_MESSAGE_WIDTH_Y_IS_IGNORED,
    CHALLENGE_MODE_TEXT_X_IGNORED_CAUSE_ITS_CENTERED_ON_THE_SCREEN,
    PUNISHMENT_MODE_TEXT_X_IGNORED,
    PUNISHMENT_MODE_COUNTER,
    SECONDARY_HEALTH_BAR_X_IGNORED,
    SUBTITLE_YPOS_X_IS_USED_TO_LIMIT_WIDTH_MESSAGE_WIDTH__SCREEN_WIDTH__2X
};

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    float fAspectRatioDiff;
    float fFieldOfView;
    float fHUDScaleX;
    float fMenuOffsetRealX;
    float fMenuOffsetRealY;
    int32_t PresetWidth;
    int32_t PresetHeight;
    float PresetFactorX;
    float PresetFactorY;
} Screen;

int32_t retX()
{
    #ifdef _LOG
    if (GetAsyncKeyState(VK_F1) & 0x8000)
    {
        auto retaddr = (uintptr_t)_ReturnAddress() - 5;
        logfile << "retX addr: " << std::dec << retXmap[retaddr] << std::endl;
    }
    #endif // _LOG

    return Screen.PresetWidth;
};

int32_t retY()
{
    #ifdef _LOG
    if (GetAsyncKeyState(VK_F1) & 0x8000)
    {
        auto retaddr = (uintptr_t)_ReturnAddress() - 5;
        logfile << "retY addr: " << std::dec << retYmap[retaddr] << std::endl;
    }
    #endif // _LOG

    return Screen.PresetHeight;
};

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("89 1D ? ? ? ? A3 ? ? ? ? 88 0D");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);

    //Presets
    int32_t arrY[] = { 448, 480, 600, 768, 960, Screen.Height };
    struct StartWithMinHeight
    {
        bool operator()(int32_t a, int32_t b)
        {
            return ((a > b) && (a < Screen.Height && b < Screen.Height));
        }
    }; std::sort(std::begin(arrY), std::end(arrY), StartWithMinHeight());
    
    Screen.PresetHeight = arrY[0];

    switch (Screen.PresetHeight)
    {
    case 448:
    case 480:
        Screen.PresetWidth = 640;
        break;
    case 600:
        Screen.PresetWidth = 800;
        break;
    case 768:
        Screen.PresetWidth = 1024;
        break;
    case 960:
        Screen.PresetWidth = 1280;
        break;
    default:
        Screen.PresetWidth = 1280;
        Screen.PresetHeight = 960;
        break;
    }

    Screen.PresetFactorX = static_cast<float>(Screen.Width) / static_cast<float>(Screen.PresetWidth);
    Screen.PresetFactorY = static_cast<float>(Screen.Height) / static_cast<float>(Screen.PresetHeight);

    static int32_t left = ((Screen.fWidth / 2.0f) - ((float)Screen.PresetWidth / 2.0f));
    static int32_t top = ((Screen.fHeight / 2.0f) - ((float)Screen.PresetHeight / 2.0f));
    static int32_t right = left + Screen.PresetWidth;
    static int32_t bottom = top + Screen.PresetHeight;

    //Screen.fMenuOffsetRealX = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    //Screen.fMenuOffsetRealY = ((640.0f * Screen.fAspectRatio) - 480.0f) / 2.0f;

    //Resolution
    static int32_t* dword_913250 = *pattern.get_first<int32_t*>(2);
    static int32_t* dword_913254 = *pattern.get_first<int32_t*>(7);
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_913250 = Screen.Width;
            *dword_913254 = Screen.Height;
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(11));
        
    pattern = hook::pattern("C7 04 24 80 02 00 00"); //0x4DD6CB
    injector::MakeNOP(pattern.get_first(-2), 2, true);
    injector::WriteMemory(pattern.get_first(3), Screen.Width, true);

    pattern = hook::pattern("C7 44 24 04 E0 01 00 00"); //0x4DD6ED
    injector::MakeNOP(pattern.get_first(-2), 2, true);
    injector::WriteMemory(pattern.get_first(4), Screen.Height, true);

    //Hud, menu and presets

    //fixing only menu and startup stuff, which makes the game stuck in an infinite loop
    uint32_t retXIncludes[] = { 25, 100, 101, 102, 103, 105, 106, 107, 130, 140, 142, 144, 145, 146, 149, 150, 151, 158, /*180,*/ 215, 216, 56, 99 };
    //uint32_t retXIncludes[] = { 25, 100 }; // 25 - hud, 100 - preset, 180 - mouse
    uint32_t retYIncludes[] = { 24, 108, 107, 109, 110, 111, 112, 113, 114, 144, 146, 149, 150, 151, 152, 153, 154, 161, /*187,*/ 223, 224 };
    //uint32_t retYIncludes[] = { 24, 108 }; // 24 - hud, 108 - preset, 187 - mouse

    pattern = hook::pattern("E8 ? ? ? ? 50 E8 ? ? ? ? 50 6A 01 6A 03 E8 ? ? ? ? 8B 15");
    auto sub_510A00 = injector::GetBranchDestination(pattern.get_first(6), true);
    auto sub_510A10 = injector::GetBranchDestination(pattern.get_first(0), true);

    pattern = hook::pattern("E8 ? ? ? ?");
    for (size_t i = 0, j = 0, k = 0; i < pattern.size(); ++i)
    {
        auto addr = pattern.get(i).get<uint32_t>(0);
        auto dest = injector::GetBranchDestination(addr, true);

        if (dest == sub_510A00)
        {
            #ifdef _LOG
            retXmap.insert(std::pair<uintptr_t, uint32_t>((uintptr_t)addr, j));
            #endif
            if (!(std::end(retXIncludes) == std::find(std::begin(retXIncludes), std::end(retXIncludes), j)))
                injector::MakeCALL(addr, retX, true);
            ++j;
        }
        else if (dest == sub_510A10)
        {
            #ifdef _LOG
            retYmap.insert(std::pair<uintptr_t, uint32_t>((uintptr_t)addr, k));
            #endif
            if (!(std::end(retYIncludes) == std::find(std::begin(retYIncludes), std::end(retYIncludes), k)))
                injector::MakeCALL(addr, retY, true);
            ++k;
        }
    }
        
    //HUD
    pattern = hook::pattern("68 ? ? ? ? 8B CF E8 ? ? ? ? 84 C0 ? ? 56 8B 74 24 08"); //0x4632C0
    static auto str_5F5588 = *pattern.get_first<uint32_t>(1);
    static size_t curLine;
    struct iHudHook
    {
        void operator()(injector::reg_pack& regs)
        {
            curLine = 0;
            *(uint32_t*)regs.esp = str_5F5588; //"#End"
            regs.ecx = regs.edi;
        }
    };
    injector::WriteMemory<uint8_t>(pattern.get_first(0), 0x56i8, true);
    injector::MakeInline<iHudHook>(pattern.get_first(1), pattern.get_first(7));

    pattern = hook::pattern("68 ? ? ? ? 8B CF 83 C6 08 E8"); //0x4632E8
    struct HudHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto x = *(int32_t*)(regs.esi + 0x00);
            auto y = *(int32_t*)(regs.esi + 0x04);

            switch (curLine)
            {
            case HEALTH_CLUSTER_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND:
            case HEALTH_BAR_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND:
            case HEALTH_BAR_WIDTH_HEIGHT_NEED_IT_SINCE_HEALTH_AND_ARMOR_ARE_IN_THE_SAME_METER:
            case SLAUGHTER_METER_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND:
            case SKULL_ICON_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND:
                break;
            case WEAPON_ICON_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX:
            case AMMO_COUNTER_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX:
            case RESERVE_AMMO_BAR_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX:
            case RETICLE_OFFSETS_FROM_CENTER_OF_SCREEN:
                break;
           case FRONT_DAMAGE_INDICATOR_CENTER:
               x = (Screen.Width / 2) - 176;
               y = 0;
               break;
           case BACK_DAMAGE_INDICATOR_CENTER:
               x = (Screen.Width / 2) - 176;
               y = Screen.Height - 69;
               break;
           case LEFT_DAMAGE_INDICATOR_CENTER:
               x = 0;
               y = (Screen.Height / 2) - 176;
               break;
           case RIGHT_DAMAGE_INDICATOR_CENTER:
               x = Screen.Width - 69;
               y = (Screen.Height / 2) - 176;
               break;
            default:
                x = static_cast<int32_t>(static_cast<float>(x) * Screen.PresetFactorX);
                y = static_cast<int32_t>(static_cast<float>(y) * Screen.PresetFactorY);

                if (x > (Screen.Width / 2))
                    x += 70;

                break;
            }

            *(int32_t*)(regs.esi + 0x00) = x;
            *(int32_t*)(regs.esi + 0x04) = y;


            *(uint32_t*)regs.esp = str_5F5588; //"#End"
            regs.ecx = regs.edi;
            ++curLine;
        }
    }; 
    injector::WriteMemory<uint8_t>(pattern.get_first(0), 0x56i8, true);
    injector::MakeInline<HudHook>(pattern.get_first(1), pattern.get_first(7));

    //menu
    //pattern = hook::pattern(""); //
    //static auto dword_73DFA4 = 0x73DFA4;
    //static auto dword_73DFA8 = 0x73DFA8;
    //static auto dword_73DFAC = 0x73DFAC;
    //static auto dword_73DFB0 = 0x73DFB0;
    //static uint32_t* dword_73DF80 = (uint32_t*)0x73DF80;
    //struct MenuHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        auto i = *dword_73DF80 * 44;
    //        
    //        auto x = *(int32_t*)(dword_73DFA4 + i);
    //        auto y = *(int32_t*)(dword_73DFA8 + i);
    //        auto w = *(int32_t*)(dword_73DFAC + i);
    //        auto h = *(int32_t*)&regs.eax;
    //
    //        //if (x == 0 && y == 0 && w == -1 && h == -1)
    //        {
    //    
    //        }
    //
    //        *(int32_t*)(dword_73DFA4 + i) = x;
    //        *(int32_t*)(dword_73DFA8 + i) = y;
    //        *(int32_t*)(dword_73DFAC + i) = w;
    //        *(int32_t*)&regs.eax = h;
    //
    //
    //        *dword_73DF80 = regs.ecx;
    //    }
    //}; injector::MakeInline<MenuHook>(0x477E26, 0x477E26 +6);
    
    
    //menu centering
    struct SetMenuPosHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esi + 0x60) = 0.6f;
            if (*(int32_t*)(regs.esi + 0x2C) == 0 && *(int32_t*)(regs.esi + 0x30) == 0)
            {
                *(int32_t*)(regs.esi + 0x2C) = left;
                *(int32_t*)(regs.esi + 0x30) = top;
                *(int32_t*)(regs.esi + 0x34) = Screen.PresetWidth;
                *(int32_t*)(regs.esi + 0x38) = Screen.PresetHeight;
            }
        }
    }; injector::MakeInline<SetMenuPosHook>(0x479663, 0x47967C);







    injector::MakeNOP(0x4DD5B9, 5, true); // no intro



    return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        //MessageBox(0,0,0,0);
        #ifdef _LOG
        logfile.open("Pun.WidescreenFix.log");
        #endif // _LOG
        Init(NULL);
    }
    return TRUE;
}
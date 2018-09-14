#include "stdafx.h"

#ifdef _DEBUG
#include <intrin.h>  
#pragma intrinsic(_ReturnAddress)  
std::map<uintptr_t, uint32_t> retXmap;
std::map<uintptr_t, uint32_t> retYmap;
#endif

char* memstr(char* block, char* pattern, size_t bsize)
{
    int32_t found = 0;
    char*   start = block;
    char*   where;

    while (!found) {
        where = (char*)memchr(start, (int32_t)pattern[0], (size_t)bsize - (size_t)(start - block));
        if (where == NULL) {
            found++;
        }
        else {
            if (memcmp(where, pattern, strlen(pattern)) == 0) {
                found++;
            }
        }
        start = where + 1;
    }
    return where;
}

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
    float fDynamicScreenFieldOfViewScale;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
    float fMenuOffsetRealX;
    float fMenuOffsetRealY;
    int32_t PresetWidth;
    int32_t PresetHeight;
    float PresetFactorX;
    float PresetFactorY;
} Screen;

int32_t retX()
{
    DBGONLY(KEYPRESS(VK_F1) { spd::log()->info("retX addr: 0x{0:08x}", retXmap[((uintptr_t)_ReturnAddress() - 5)]); });

    return Screen.PresetWidth;
};

int32_t retY()
{
    DBGONLY(KEYPRESS(VK_F1) { spd::log()->info("retY addr: 0x{0:08x}", retYmap[((uintptr_t)_ReturnAddress() - 5)]); });

    return Screen.PresetHeight;
};

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 0) != 0;
    static float fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 0.0f);
    fFOVFactor = fFOVFactor ? fFOVFactor : 1.0f;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

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

    Screen.PresetFactorX = Screen.fWidth / static_cast<float>(Screen.PresetWidth);
    Screen.PresetFactorY = Screen.fHeight / static_cast<float>(Screen.PresetHeight);

    //Resolution
    auto pattern = hook::pattern("89 1D ? ? ? ? A3 ? ? ? ? 88 0D");
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
    //this code doesn't work anymore because patterns don't find 0x46d91b and 0x4b7cbb calls for some reason and retXmap indices are wrong
    //uint32_t retXIncludes[] = { 25, 100, 101, 102, 103, 105, /*106,*/ 107, 130, 140, 142, 144, 145, 146, 149, 150, 151, 158, /*180,*/ 215, 216, 56, 99 };
    uint32_t retXIncludes[] = { 25, 100 }; // 25 - hud, 100 - preset, 180 - mouse, 215 - weapons overlay preset, 106 - weapons overlay scale
    //int32_t retYIncludes[] = { 24, 108, 107, 109, 110, 111, 112, 113, 114, 144, 146, 149, 150, 151, 152, 153, 154, 161, /*187,*/ 223, 224 };
    uint32_t retYIncludes[] = { 24, 108 }; // 24 - hud, 108 - preset, 187 - mouse, 223 - weapons overlay preset, 112+113 - weapons overlay scale

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

    //so instead just manual patch
    //pattern = hook::pattern("E8 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 6A 01"); //X - 25, Y - 24
    //injector::MakeCALL(pattern.get_first(0), retX, true);
    //injector::MakeCALL(pattern.get_first(10), retY, true);
    //pattern = hook::pattern("E8 ? ? ? ? 50 8D 44 24 38 68 ? ? ? ? 50"); //X - 100, Y - 108
    //injector::MakeCALL(pattern.get_first(0), retX, true);
    //injector::MakeCALL(pattern.get_first(-6), retY, true);

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
            case HEALTH_CLUSTER_BACKGROUND_COORDS__SET_ME_TO_MOVE_HEALTHSLAUGHTER_HUD:
                y = static_cast<int32_t>(static_cast<float>(y) * Screen.PresetFactorY);
                break;
            case COMBO_SCORE_DISPLAY:
            case AMMO_PICKUP_MESSAGES:
            case MAIN_WEAPON_UPPER_LEFT_CORNER:
                x = Screen.Width - (Screen.PresetWidth - x);
                y = static_cast<int32_t>(static_cast<float>(y) * Screen.PresetFactorY);
                break;
            case HEALTH_CLUSTER_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND:
            case HEALTH_BAR_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND:
            case HEALTH_BAR_WIDTH_HEIGHT_NEED_IT_SINCE_HEALTH_AND_ARMOR_ARE_IN_THE_SAME_METER:
            case SLAUGHTER_METER_COORDS_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND:
            case SKULL_ICON_RELATIVE_TO_HEALTH_CLUSTER_BACKGROUND:
                break;
                //case WEAPON_ICON_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX:
            case AMMO_COUNTER_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX:
            case RESERVE_AMMO_BAR_OFFSET_FROM_UPPER_LEFT_CORNER_OF_BOX:
                //case RETICLE_OFFSETS_FROM_CENTER_OF_SCREEN:
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


    //Menu
    pattern = hook::pattern("89 0D ? ? ? ? 68 ? ? ? ? 8D"); //0x477E26
    static uint32_t* dword_73DF80 = *pattern.get_first<uint32_t*>(2);
    static auto dword_73DFA4 = *hook::get_pattern<uint32_t>("89 81 ? ? ? ? 8D 0C 24 E8 ? ? ? ? 8B 15", 2);
    static auto dword_73DFA8 = *hook::get_pattern<uint32_t>("89 82 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 6B C9 2C", 2);
    static auto dword_73DFAC = *hook::get_pattern<uint32_t>("89 81 ? ? ? ? 8D 0C 24 E8 ? ? ? ? 8B 0D", 2);
    static auto dword_73DFB0 = *hook::get_pattern<uint32_t>("8D 4C 24 04 89 82 ? ? ? ? E8 ? ? ? ? 84 C0", 6);
    struct MenuHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto i = *dword_73DF80 * 44;
            auto x = *(int32_t*)(dword_73DFA4 + i);
            auto y = *(int32_t*)(dword_73DFA8 + i);
            auto w = *(int32_t*)(dword_73DFAC + i);
            auto h = *(int32_t*)&regs.eax;
            if (x == 0 && w == Screen.PresetWidth && h == (Screen.PresetHeight - y))
            {
                //*(int32_t*)(dword_73DFA4 + i) = x;
                //*(int32_t*)(dword_73DFA8 + i) = y;
                *(int32_t*)(dword_73DFAC + i) = Screen.Width; //makes weapons overlay not being cut off by preset
                *(int32_t*)&regs.eax = Screen.Height;
            }
            *dword_73DF80 = regs.ecx;
        }
    }; injector::MakeInline<MenuHook>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("A2 ? ? ? ? 8A 44 24 10 88 0D"); //0x510B6C
    static auto gTexColor = *pattern.get_first<uint32_t*>(1);
    static auto GetTexColor = []() -> uint32_t {
        return *gTexColor;
    };

    static auto SetTexColor = [](uint32_t argb) {
        *gTexColor = argb;
    };

    pattern = hook::pattern("57 8B 7C 24 08 57 E8 ? ? ? ? 83 C4 04 85 C0 7D 7E A1 ? ? ? ? 55 56 8B F0 6B F6 2C"); //50C750
    static auto GetTexID = (int32_t(__cdecl*)(const char* FileName)) pattern.get_first(0);

    static injector::hook_back<void(__cdecl*)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int8_t, int8_t, int32_t, int8_t, int8_t)> hb_519CA0;
    auto DrawHook = [](int32_t texID, int32_t x, int32_t y, int32_t w, int32_t h, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int8_t a10, int8_t a11, int32_t a12, int8_t a13, int8_t a14)
    {
        if (x == 0 && y == 0 && w == Screen.Width && h == Screen.Height)
        {
            x = static_cast<int32_t>(Screen.fHudOffsetReal);
            //y = top;
            w = static_cast<int32_t>(Screen.fWidth - Screen.fHudOffsetReal - Screen.fHudOffsetReal);
            //h = Screen.PresetHeight;

            auto og = GetTexColor();
            SetTexColor(0xFF000000);
            hb_519CA0.fun(texID, 0, 0, (int)Screen.fHudOffsetReal, Screen.Height, a6, a7, a8, a9, a10, a11, a12, a13, a14);
            hb_519CA0.fun(texID, Screen.Width - (int32_t)Screen.fHudOffsetReal, 0, Screen.Width, Screen.Height, a6, a7, a8, a9, a10, a11, a12, a13, a14);
            SetTexColor(og);
        }
        else
        {
            if ((x == 0 && y == 0 && w == 640 && h == 360) || (x == 0 && y == 0 && w == 960 && h == 540) || (x == 0 && y == 540 && w == 960 && h == 540))
            {
                x += static_cast<int32_t>(Screen.fHudOffsetReal);
                w -= static_cast<int32_t>(Screen.fHudOffsetReal);

                auto og = GetTexColor();
                SetTexColor(0xFF000000);
                hb_519CA0.fun(texID, 0, 0, (int)Screen.fHudOffsetReal, Screen.Height, a6, a7, a8, a9, a10, a11, a12, a13, a14);
                hb_519CA0.fun(texID, Screen.Width - (int32_t)Screen.fHudOffsetReal, 0, Screen.Width, Screen.Height, a6, a7, a8, a9, a10, a11, a12, a13, a14);
                SetTexColor(og);
            }
            else
            {
                if ((x == 960 && y == 0 && w == 960 && h == 540) || (x == 960 && y == 540 && w == 960 && h == 540))
                {
                    w -= static_cast<int32_t>(Screen.fHudOffsetReal);

                    auto og = GetTexColor();
                    SetTexColor(0xFF000000);
                    hb_519CA0.fun(texID, 0, 0, (int)Screen.fHudOffsetReal, Screen.Height, a6, a7, 1, 1, a10, a11, a12, a13, a14);
                    hb_519CA0.fun(texID, Screen.Width - (int32_t)Screen.fHudOffsetReal, 0, Screen.Width, Screen.Height, a6, a7, 1, 1, a10, a11, a12, a13, a14);
                    SetTexColor(og);
                }
            }
        }
        return hb_519CA0.fun(texID, x, y, w, h, a6, a7, a8, a9, a10, a11, a12, a13, a14);
    };
    pattern = hook::pattern("8B 54 24 34 51 52 50 E8 ? ? ? ? 83 C4 38 C3"); //0x510C68
    hb_519CA0.fun = injector::MakeCALL(pattern.get_first(7), static_cast<void(__cdecl*)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int8_t, int8_t, int32_t, int8_t, int8_t)>(DrawHook), true).get();

    //struct WeaponsOverlayHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        regs.eax = *(uint32_t*)(regs.esi + regs.edi + 0x230);
    //        bWeapOverlay = true;
    //    }
    //}; injector::MakeInline<WeaponsOverlayHook>(0x47D2DA, 0x47D2DA+7);

    pattern = hook::pattern("6A 00 6A 00 B9 ? ? ? ? C7 05"); // 0x47C872 0x47C8C2
    injector::WriteMemory<uint8_t>(pattern.count(2).get(0).get<uint8_t*>(1), 1i8, true); // makes weapons screen stretched
    injector::WriteMemory<uint8_t>(pattern.count(2).get(0).get<uint8_t*>(1), 1i8, true);

    pattern = hook::pattern("75 ? E8 ? ? ? ? 8B 4C 24 28 89 01 8B CB E8 ? ? ? ? 89 45 00"); //0x5266C4
    struct WeaponsOverlayHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            **(int32_t**)(regs.esp + 0x28) = static_cast<int32_t>(static_cast<float>(**(int32_t**)(regs.esp + 0x28)) * (Screen.fWidth / (480.0f * Screen.fAspectRatio)));
            **(int32_t**)(regs.esp + 0x2C) = static_cast<int32_t>(static_cast<float>(**(int32_t**)(regs.esp + 0x2C)) * Screen.fHeight / 448.0f);

            **(int32_t**)(regs.esp + 0x28) += static_cast<int32_t>(Screen.fHudOffsetReal);
        }
    };
    injector::MakeNOP(pattern.get_first(0), 2, true); //weapons overlay preset
    injector::MakeInline<WeaponsOverlayHook2>(pattern.get_first(23), pattern.get_first(23 + 14)); //0x5266DB, 0x5266E9

    pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 18 E8 ? ? ? ? 89"); //0x47D10E
    injector::WriteMemory(pattern.get_first(2), &Screen.fHUDScaleX, true); //weapons overlay scale
    //pattern = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B 15 ? ? ? ? 50 6A"); //0x557FC8
    //injector::WriteMemory(pattern.get_first(2), &Screen.fHUDScaleX, true); //videos scale

    pattern = hook::pattern("8B 8F 8C 00 00 00 50 53 53 51"); //0x48ADB2
    struct BackgroundHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.edi + 0x8C);
            auto str = (char*)((*(uintptr_t*)(regs.edi)) + 0x30);
            static char* menuBackgrounds[] = { "options_bg", "bf_detail_bg", "pc_controls_menu_bg", "gs_menu_bg", "av_menu_bg" };

            for (size_t i = 0; i < std::size(menuBackgrounds); ++i)
            {
                if (memstr(str, menuBackgrounds[i], 0x60) != NULL)
                {
                    regs.eax += 1;
                    break;
                }
            }
        }
    }; injector::MakeInline<BackgroundHook>(pattern.get_first(0), pattern.get_first(6));


    if (bSkipIntro)
    {
        pattern = hook::pattern("E8 ? ? ? ? 68 00 00 40 3F 68 00 00"); // 0x4DD5B9
        injector::MakeNOP(pattern.count(5).get(3).get<void*>(0), 5, true);
    }

    //FOV
    pattern = hook::pattern("A0 ? ? ? ? 83 EC 50 84 C0"); // 0x515890
    static auto byte_9132CC = *pattern.get_first<uint8_t*>(1);
    struct FOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *byte_9132CC;
            *(float*)(regs.esp + 0x0C) = AdjustFOV(*(float*)(regs.esp + 0x0C), Screen.fAspectRatio) * fFOVFactor;
        }
    }; injector::MakeInline<FOVHook>(pattern.get_first(0));
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("89 1D ? ? ? ? A3 ? ? ? ? 88 0D"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}
module;

#include <stdafx.h>

export module WidescreenHUD2;

import ComVars;

export enum class hud_PC : uint32_t
{
    alarm = 1956595421,
    hud_icon_notes = 2840175901,
    hud_icon_objective = 4004306325,
    hud_radar_border = 2941877717,
    hud_reticule_foregrip = 4175357751,
    icon_coopinteraction = 2914525342,
    icon_coopinteraction_border = 422250158,
    icon_revivesyringe = 451393891,
    icon_talkall = 2422271528,
    LW_icon_designation = 3143010318,
    LW_icon_electrostatic = 3617456639,
    LW_icon_explosion = 4166503279,
    LW_icon_hacking = 1452143097,
    LW_icon_remoteaccess = 3940553799,
    LW_icon_soundconductor = 3703913722,
    LW_mire = 462746100,
    LW_point = 1899848169,
    QI_share_icon = 2789032129,
    QI_share_icon_red = 1547439285,
    radar_dead = 327558092,
    radar_player = 2748937572,
    reticule_mire_gr = 2278950989,
    reticule_motoenage = 2439817659,
    SC_greenstroke = 2731859674,
    SC_noise01 = 3585549550,
    SC_noise04 = 2782777441,
    SC_whiteborder = 2788404590,
    arrows = 3669484878,
    black = 2307242536,
    black_alpha = 3744670750,
    checkbox_cancel = 533042745,
    checkbox_empty = 3537058594,
    checkbox_full = 609171284,
    compass_ruler = 4204949627,
    computer_back = 1879166341,
    goggles_border = 1750577301,
    goggles_border_white2 = 3783554923,
    green = 3499814433,
    green_alpha = 1589136939,
    hud_crochet_big = 31428708,
    hud_select_interact = 933652789,
    hud_stealthbar = 138747756,
    hud_stealthbar_scroll = 4279611401,
    icon_coopaction_border = 4079215610,
    icon_file_hacking = 789242941,
    icon_flashgrenade = 706182912,
    icon_fn2000 = 2526098702,
    icon_fn2000_foregrip = 2775680965,
    icon_fn2000_launcher = 3526589992,
    icon_fn2000_scope = 624864586,
    icon_fn2000_shotgun = 4249566477,
    icon_fn7 = 1401884002,
    icon_fraggrenade = 387368590,
    icon_gazgrenade = 2730121599,
    icon_ringairfoilround = 875564491,
    icon_smokegrenade = 3992387367,
    icon_stickycamera = 2678268821,
    icon_stickyshocker = 2215814964,
    icon_wallmine = 2558228076,
    reticule_57 = 1410065888,
    reticule_57_stroke = 2263824691,
    reticule_57_stroke2 = 2776838178,
    reticule_mire = 2274167227,
    reticule_mire_border = 37960903,
    sniper_firstborder = 258125297,
    sniper_mire = 340120050,
    sniper_secondborder = 1643361012,
    white = 1120914852,
    P_hud = 856462664,
    P_hudCoop = 4011034197,
    P_endmission = 1445831552,
    P_Sniper = 4259273135,
    P_Camera = 1760296801,
    P_opticcable = 641266120,
    P_FadeBlack = 1128923019,
    P_EEV = 336078728,
    P_KeyPad = 2640742754,
    P_ChatIngame = 914490011,
    P_QuickInventory_pc = 3426341998,
    P_QuickInventory_New = 3456766341,
    A_Goggles = 3326147903,
    A_HUD = 4163594736,
    A_Communications = 2253148608,
    A_Interact = 2193135514,
    A_reticules = 1474598195,
    A_timer = 775565974,
    A_Goals = 1108336039,
    A_Notes = 2217764100,
    A_missionMessage2lines = 3003692632,
    A_missionMessage4lines = 1699829061,
    A_missionMessage6lines = 681178190,
    AInst_FadeToBlack = 3572068294,
    A_Communications1 = 406741334,
    A_coopinteraction_stop = 2630401369,
};

export bool HudNeedsToMoveLeft(uint32_t id)
{
    static const hud_PC elements[] = {
        hud_PC::A_Communications,
    };

    return std::any_of(std::begin(elements), std::end(elements), [id](hud_PC hash)
    {
        return id == std::to_underlying(hash);
    });
}

export bool HudNeedsToMoveRight(uint32_t id)
{
    static const hud_PC elements[] = {
        hud_PC::A_missionMessage2lines,
        hud_PC::A_missionMessage4lines,
        hud_PC::A_missionMessage6lines,
        hud_PC::A_Notes,
        hud_PC::A_HUD,
        hud_PC::A_Interact,
        hud_PC::A_timer,
        hud_PC::A_Goals,
        hud_PC::A_Communications1,
        hud_PC::A_coopinteraction_stop,
    };

    return std::any_of(std::begin(elements), std::end(elements), [id](hud_PC hash)
    {
        return id == std::to_underlying(hash);
    });
}

export void InitWidescreenHUD2()
{
    auto pattern = hook::pattern("FF 52 ? 8B CF E8 ? ? ? ? 8D 4C 24 1C");
    static auto MAGMAParserHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto id = *(uint32_t*)(regs.esp + 0x18);
        auto& x = *(int16_t*)(regs.eax + 0x8);
        auto& y = *(int16_t*)(regs.eax + 0xA);

        if (HudNeedsToMoveLeft(id))
        {
            x -= 100;
        }
        else if (HudNeedsToMoveRight(id))
        {
            x += 100;
        }
        else
        {
            //const char* enumName = nullptr;
            //switch (static_cast<hud_PC>(id))
            //{
            //case hud_PC::alarm: enumName = "alarm"; break;
            //case hud_PC::hud_icon_notes: enumName = "hud_icon_notes"; break;
            //case hud_PC::hud_icon_objective: enumName = "hud_icon_objective"; break;
            //case hud_PC::hud_radar_border: enumName = "hud_radar_border"; break;
            //case hud_PC::hud_reticule_foregrip: enumName = "hud_reticule_foregrip"; break;
            //case hud_PC::icon_coopinteraction: enumName = "icon_coopinteraction"; break;
            //case hud_PC::icon_coopinteraction_border: enumName = "icon_coopinteraction_border"; break;
            //case hud_PC::icon_revivesyringe: enumName = "icon_revivesyringe"; break;
            //case hud_PC::icon_talkall: enumName = "icon_talkall"; break;
            //case hud_PC::LW_icon_designation: enumName = "LW_icon_designation"; break;
            //case hud_PC::LW_icon_electrostatic: enumName = "LW_icon_electrostatic"; break;
            //case hud_PC::LW_icon_explosion: enumName = "LW_icon_explosion"; break;
            //case hud_PC::LW_icon_hacking: enumName = "LW_icon_hacking"; break;
            //case hud_PC::LW_icon_remoteaccess: enumName = "LW_icon_remoteaccess"; break;
            //case hud_PC::LW_icon_soundconductor: enumName = "LW_icon_soundconductor"; break;
            //case hud_PC::LW_mire: enumName = "LW_mire"; break;
            //case hud_PC::LW_point: enumName = "LW_point"; break;
            //case hud_PC::QI_share_icon: enumName = "QI_share_icon"; break;
            //case hud_PC::QI_share_icon_red: enumName = "QI_share_icon_red"; break;
            //case hud_PC::radar_dead: enumName = "radar_dead"; break;
            //case hud_PC::radar_player: enumName = "radar_player"; break;
            //case hud_PC::reticule_mire_gr: enumName = "reticule_mire_gr"; break;
            //case hud_PC::reticule_motoenage: enumName = "reticule_motoenage"; break;
            //case hud_PC::SC_greenstroke: enumName = "SC_greenstroke"; break;
            //case hud_PC::SC_noise01: enumName = "SC_noise01"; break;
            //case hud_PC::SC_noise04: enumName = "SC_noise04"; break;
            //case hud_PC::SC_whiteborder: enumName = "SC_whiteborder"; break;
            //case hud_PC::arrows: enumName = "arrows"; break;
            //case hud_PC::black: enumName = "black"; break;
            //case hud_PC::black_alpha: enumName = "black_alpha"; break;
            //case hud_PC::checkbox_cancel: enumName = "checkbox_cancel"; break;
            //case hud_PC::checkbox_empty: enumName = "checkbox_empty"; break;
            //case hud_PC::checkbox_full: enumName = "checkbox_full"; break;
            //case hud_PC::compass_ruler: enumName = "compass_ruler"; break;
            //case hud_PC::computer_back: enumName = "computer_back"; break;
            //case hud_PC::goggles_border: enumName = "goggles_border"; break;
            //case hud_PC::goggles_border_white2: enumName = "goggles_border_white2"; break;
            //case hud_PC::green: enumName = "green"; break;
            //case hud_PC::green_alpha: enumName = "green_alpha"; break;
            //case hud_PC::hud_crochet_big: enumName = "hud_crochet_big"; break;
            //case hud_PC::hud_select_interact: enumName = "hud_select_interact"; break;
            //case hud_PC::hud_stealthbar: enumName = "hud_stealthbar"; break;
            //case hud_PC::hud_stealthbar_scroll: enumName = "hud_stealthbar_scroll"; break;
            //case hud_PC::icon_coopaction_border: enumName = "icon_coopaction_border"; break;
            //case hud_PC::icon_file_hacking: enumName = "icon_file_hacking"; break;
            //case hud_PC::icon_flashgrenade: enumName = "icon_flashgrenade"; break;
            //case hud_PC::icon_fn2000: enumName = "icon_fn2000"; break;
            //case hud_PC::icon_fn2000_foregrip: enumName = "icon_fn2000_foregrip"; break;
            //case hud_PC::icon_fn2000_launcher: enumName = "icon_fn2000_launcher"; break;
            //case hud_PC::icon_fn2000_scope: enumName = "icon_fn2000_scope"; break;
            //case hud_PC::icon_fn2000_shotgun: enumName = "icon_fn2000_shotgun"; break;
            //case hud_PC::icon_fn7: enumName = "icon_fn7"; break;
            //case hud_PC::icon_fraggrenade: enumName = "icon_fraggrenade"; break;
            //case hud_PC::icon_gazgrenade: enumName = "icon_gazgrenade"; break;
            //case hud_PC::icon_ringairfoilround: enumName = "icon_ringairfoilround"; break;
            //case hud_PC::icon_smokegrenade: enumName = "icon_smokegrenade"; break;
            //case hud_PC::icon_stickycamera: enumName = "icon_stickycamera"; break;
            //case hud_PC::icon_stickyshocker: enumName = "icon_stickyshocker"; break;
            //case hud_PC::icon_wallmine: enumName = "icon_wallmine"; break;
            //case hud_PC::reticule_57: enumName = "reticule_57"; break;
            //case hud_PC::reticule_57_stroke: enumName = "reticule_57_stroke"; break;
            //case hud_PC::reticule_57_stroke2: enumName = "reticule_57_stroke2"; break;
            //case hud_PC::reticule_mire: enumName = "reticule_mire"; break;
            //case hud_PC::reticule_mire_border: enumName = "reticule_mire_border"; break;
            //case hud_PC::sniper_firstborder: enumName = "sniper_firstborder"; break;
            //case hud_PC::sniper_mire: enumName = "sniper_mire"; break;
            //case hud_PC::sniper_secondborder: enumName = "sniper_secondborder"; break;
            //case hud_PC::white: enumName = "white"; break;
            //case hud_PC::P_hud: enumName = "P_hud"; break;
            //case hud_PC::P_hudCoop: enumName = "P_hudCoop"; break;
            //case hud_PC::P_endmission: enumName = "P_endmission"; break;
            //case hud_PC::P_Sniper: enumName = "P_Sniper"; break;
            //case hud_PC::P_Camera: enumName = "P_Camera"; break;
            //case hud_PC::P_opticcable: enumName = "P_opticcable"; break;
            //case hud_PC::P_FadeBlack: enumName = "P_FadeBlack"; break;
            //case hud_PC::P_EEV: enumName = "P_EEV"; break;
            //case hud_PC::P_KeyPad: enumName = "P_KeyPad"; break;
            //case hud_PC::P_ChatIngame: enumName = "P_ChatIngame"; break;
            //case hud_PC::P_QuickInventory_pc: enumName = "P_QuickInventory_pc"; break;
            //case hud_PC::P_QuickInventory_New: enumName = "P_QuickInventory_New"; break;
            //case hud_PC::A_Goggles: enumName = "A_Goggles"; break;
            //case hud_PC::A_HUD: enumName = "A_HUD"; break;
            //case hud_PC::A_Communications: enumName = "A_Communications"; break;
            //case hud_PC::A_Interact: enumName = "A_Interact"; break;
            //case hud_PC::A_reticules: enumName = "A_reticules"; break;
            //case hud_PC::A_timer: enumName = "A_timer"; break;
            //case hud_PC::A_Goals: enumName = "A_Goals"; break;
            //case hud_PC::A_Notes: enumName = "A_Notes"; break;
            //case hud_PC::A_missionMessage2lines: enumName = "A_missionMessage2lines"; break;
            //case hud_PC::A_missionMessage4lines: enumName = "A_missionMessage4lines"; break;
            //case hud_PC::A_missionMessage6lines: enumName = "A_missionMessage6lines"; break;
            //case hud_PC::AInst_FadeToBlack: enumName = "AInst_FadeToBlack"; break;
            //case hud_PC::A_Communications1: enumName = "A_Communications1"; break;
            //case hud_PC::A_coopinteraction_stop: enumName = "A_coopinteraction_stop"; break;
            //default: enumName = "UNKNOWN"; break;
            //}
            //
            //if (std::string_view(enumName) == "UNKNOWN")
            //{
            //    spd::log()->info("{}", id);
            //}
        }
    });
}
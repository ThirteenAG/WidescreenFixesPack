module;

#include <stdafx.h>

export module ComVars;

export struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t nAspectRatioDiff;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

export const char* movies[] =
{
    "B10_Trns_B11_27",
    "B11_Trns_B12_028",
    "B12_Trns_B125_29",
    "B12_Trns_B13_30",
    "b13_end_033",
    "B13_Smithzilla_Con",
    "B13_Smithzilla_Des",
    "B1_Trns_B2_002",
    "B2_RM_003S",
    "B2_Trns_B3_005",
    "B2_Trns_RM_004",
    "B3_DO_006S",
    "B3_Trns_B4_012",
    "B4_LH_013S",
    "B4_Trns_B5_014",
    "B5_LO_016",
    "B5_Trns_B6_018",
    "B65_Trns_B7_021",
    "B6_Trns_B65_020s",
    "B6_TS_019S",
    "B7_022s",
    "B7_Trns_B8_023",
    "B8_Trns_B9_024",
    "B9_Trns_B10_025",
    "c_hand",
    "C_Staff",
    "C_Sword",
    "Extra_B3Bamboo_Finish",
    "Extra_B3_Gun_Intro",
    "Extra_B5_Rooftop_Showdown",
    "Extra_B6_SS_WallSmack",
    "Extra_B8AiTutSuperJump",
    "Extra_BLUR_makingof",
    "Extra_OUTTAKES",
    "pon_trailer",
    "PowerBubble",
    "serpahfight_256",
    "start_screen_512",
    "WB_Green"
};


export ProtectedGameRef<int32_t> dword_6CACDC, dword_6CACE0;
export GameRef<void*> CinemaRenderer;
export ProtectedGameRef<float> BarHeight;
export GameRef<float> WideRatio, CropRatio, WideCamera;
export GameRef<uintptr_t> ActiveCamera;

#include "stdafx.h"
#include <dxsdk/d3d8.h>

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHUDScaleX;
    double dHUDScaleX;
    double dHUDScaleXInv;
    float fHudOffset;
    float fHudOffsetRight;
    float fFMVoffsetStartX;
    float fFMVoffsetEndX;
    float fFMVoffsetStartY;
    float fFMVoffsetEndY;
    float fIniHudOffset;
    float fWidescreenHudOffset;
    bool bHudWidescreenMode;
    bool bOpsatWidescreenMode;
    uint32_t nPostProcessFixedScale;
    uint32_t nFMVWidescreenMode;
} Screen;

union FColor
{
    uint32_t RGBA;
    struct
    {
        uint8_t B, G, R, A;
    };
} gColor;

std::vector<std::pair<const std::wstring, std::wstring>> ResList =
{
    { L"640x480",   L"" },
    { L"800x600",   L"" },
    { L"1024x768",  L"" },
    { L"1280x1024", L"" },
    { L"1600x1200", L"" },
};

std::vector<uintptr_t> EchelonGameInfoPtrs;
uintptr_t pDrawTile;
bool bSkipIntro = false;
int EPlayerControllerState = -1;
int EchelonMainHUDState = -1;
int EGameInteractionState = -1;
bool bPlayingVideo = false;
bool bPressStartToContinue = false;
bool bSkipPressStartToContinue = false;
bool bKeyPad = false;
bool bElevatorPanel = false;

namespace HudMatchers
{
    inline bool IsStealthMeterLine(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX2 - offsetX1 == 17.0f) &&
            (offsetY1 == 337.0f || offsetY1 == 360.0f) &&
            (offsetY2 == 368.0f || offsetY2 == 345.0f);
    }

    inline bool IsStealthMeter(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX2 - offsetX1 == 109.0f) &&
            offsetY1 == 338.0f && offsetY2 == 368.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 4271286934 || color.RGBA == 4283459416 || color.RGBA == 4288064150);
    }

    inline bool IsWeaponIconBackground(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 == 491.0f && offsetX2 == 592.0f &&
            offsetY1 == 373.0f && offsetY2 == 441.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 4283459416);
    }

    inline bool IsWeaponIconBorder(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 487.0f || offsetX1 == 503.0f || offsetX1 == 580.0f) &&
            (offsetX2 == 503.0f || offsetX2 == 580.0f || offsetX2 == 596.0f) &&
            (offsetY1 == 369.0f || offsetY1 == 385.0f || offsetY1 == 429.0f) &&
            color.RGBA == 2152752984;
    }

    inline bool IsAlarmCounterText(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 >= 562.0f && offsetX2 <= 602.0f &&
            (offsetY1 == 316.0f || offsetY1 == 315.0f) &&
            offsetY2 == 330.0f &&
            (color.RGBA == 4265759816 || color.RGBA == 4283459416 || color.RGBA == 4282537289);
    }

    inline bool IsHealthBar(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 570.0f || offsetX1 == 586.0f) &&
            (offsetX2 == 586.0f || offsetX2 == 602.0f) &&
            color.RGBA == 3360712536;
    }

    inline bool IsEnvelopeIcon(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 == 492.0f && offsetX2 == 519.0f &&
            offsetY1 == 314.0f && offsetY2 == 333.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 4283459416);
    }

    inline bool IsAlarmIcon(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 == 521.0f && offsetX2 == 562.0f &&
            offsetY1 == 306.0f && offsetY2 == 340.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 2152752984 || color.RGBA == 4283459416);
    }

    inline bool IsWeaponIcon(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 494.0f || offsetX1 == 515.0f) &&
            offsetY1 == 377.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 4282537289 || color.RGBA == 4283459416);
    }

    inline bool IsWeaponNameText(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 >= 491.0f || offsetX1 >= 490.0f) &&
            offsetY1 == 423.0f &&
            (offsetY2 == 437.0f || offsetY2 == 438.0f);
    }

    inline bool IsAmmoText(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 >= 571.0f &&
            offsetY1 == 373.0f &&
            (offsetY2 == 385.0f || offsetY2 == 388.0f) &&
            (color.RGBA == 4265759816 || color.RGBA == 4283459416 || color.RGBA == 4282537289);
    }

    inline bool IsBulletsImage(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 581.0f && (offsetX2 == 588.0f || offsetX2 == 592.0f)) ||
            (offsetX1 == 568.0f && offsetY1 == 406.0f);
    }

    inline bool IsRateOfFireImage(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 >= 536.0f && offsetX2 <= 576.0f &&
            offsetY1 == 416.0f && offsetY2 == 421.0f;
    }

    inline bool IsWeaponAddonBackground(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return ((offsetX1 == 441.0f && offsetX2 == 481.0f && offsetY1 == 373.0f && offsetY2 == 413.0f) ||
            (offsetX1 == 439.0f && offsetX2 == 483.0f && offsetY1 == 380.0f && offsetY2 == 412.0f)) &&
            (color.RGBA == 4266682200 || color.RGBA == 4282537289 || color.RGBA == 4283459416 || color.RGBA == 4286094213);
    }

    inline bool IsWeaponAddonBorder(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 437.0f || offsetX1 == 453.0f || offsetX1 == 469.0f) &&
            (offsetX2 == 453.0f || offsetX2 == 469.0f || offsetX2 == 485.0f) &&
            (offsetY1 == 369.0f || offsetY1 == 385.0f || offsetY1 == 401.0f) &&
            color.RGBA == 2152752984;
    }

    inline bool IsWeaponAddonAmmoText(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 452.0f || offsetX1 == 456.0f || offsetX1 == 457.0f || offsetX1 == 460.0f) &&
            (offsetX2 == 460.0f || offsetX2 == 465.0f || offsetX2 == 464.0f || offsetX2 == 467.0f || offsetX2 == 469.0f) &&
            offsetY1 == 371.0f &&
            (offsetY2 == 383.0f || offsetY2 == 386.0f) &&
            (color.RGBA == 4265759816 || color.RGBA == 4282537289 || color.RGBA == 4283459416 || color.RGBA == 4286094213);
    }

    inline bool IsInteractionMenuBackground(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return ((offsetX1 == 362.0f) || (offsetX1 == 422.0f || offsetX1 == 443.0f)) &&
            offsetX2 == 562.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 4269316740 || color.RGBA == 4283459416 || color.RGBA == 4282537289 || color.RGBA == 4286094213);
    }

    inline bool IsInteractionMenuBorder(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return ((offsetX1 == 359.0f || offsetX1 == 375.0f || offsetX1 == 549.0f || offsetX1 == 554.0f ||
            offsetX1 == 440.0f || offsetX1 == 456.0f || offsetX1 == 537.0f ||
            offsetX1 == 419.0f || offsetX1 == 435.0f || offsetX1 == 439.0f) &&
            (offsetX2 == 375.0f || offsetX2 == 370.0f || offsetX2 == 549.0f || offsetX2 == 565.0f ||
            offsetX2 == 456.0f || offsetX2 == 451.0f || offsetX2 == 435.0f || offsetX2 == 430.0f) &&
            (color.RGBA == 2152752984 || color.RGBA == 4266682200 || color.RGBA == 4283459416));
    }

    inline bool IsInteractionMenuText(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 >= 365.0f &&
            offsetY1 >= 42.0f && offsetY2 <= 192.0f &&
            (color.RGBA == 4265759816 || color.RGBA == 4269316740 || color.RGBA == 4282537289 || color.RGBA == 4283459416 || color.RGBA == 4286094213);
    }

    inline bool IsTurretInterface(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 371.0f || offsetX1 == 379.0f || offsetX1 == 387.0f ||
            offsetX1 == 558.0f || offsetX1 == 566.0f) &&
            offsetY1 >= 39.0f && offsetY2 <= 162.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 3036610302 ||
            color.RGBA == 4282537289 || color.RGBA == 4283459416 || color.RGBA == 4286094213 || color.RGBA == 3036676095);
    }

    inline bool IsTopDialogueMenuBackground(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 == 48.0f && offsetX2 == 348.0f &&
            offsetY1 >= 42.0f &&
            (offsetY2 <= 87.0f || offsetY2 <= 90.0f) &&
            (color.RGBA == 4266682200 || color.RGBA == 3036610302 || color.RGBA == 4282537289 || color.RGBA == 4283459416 || color.RGBA == 4286094213);
    }

    inline bool IsTopDialogueMenuBorder(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 45.0f || offsetX1 == 61.0f || offsetX1 == 335.0f) &&
            (offsetX2 == 61.0f || offsetX2 == 335.0f || offsetX2 == 351.0f) &&
            (color.RGBA == 4266682200 || color.RGBA == 3036610302 || color.RGBA == 4282537289 || color.RGBA == 4283459416 || color.RGBA == 4286094213);
    }

    inline bool IsTopDialogueMenuText(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 >= 52.0f &&
            offsetY1 >= 43.0f && offsetY2 <= 185.0f &&
            (color.RGBA == 4265759816 || color.RGBA == 4283459416 || color.RGBA == 4282537289);
    }

    inline bool IsTimer(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        bool timerText = (offsetX1 >= 67.0f && offsetX2 <= 171.0f &&
                        offsetY1 >= 385.0f && offsetY2 <= 409.0f &&
                        (color.RGBA == 4286094213 || color.RGBA == 4282384384));

        bool timerTopBorder = (offsetX1 == 64.0f && (offsetX2 == 144.0f || offsetX2 == 174.0f) &&
                            offsetY1 == 382.0f && offsetY2 == 383.0f &&
                            color.RGBA == 4283459416);

        bool timerBottomBorder = (offsetX1 == 64.0f && (offsetX2 == 144.0f || offsetX2 == 174.0f) &&
                                offsetY1 == 411.0f && offsetY2 == 412.0f &&
                                color.RGBA == 4283459416);

        bool timerLeftBorder = (offsetX1 == 64.0f && offsetX2 == 65.0f &&
                            offsetY1 == 382.0f && offsetY2 == 412.0f &&
                            color.RGBA == 4283459416);

        bool timerRightBorder = ((offsetX1 == 143.0f || offsetX1 == 173.0f) && 
                                (offsetX2 == 144.0f || offsetX2 == 174.0f) &&
                                offsetY1 == 382.0f && offsetY2 == 412.0f &&
                                color.RGBA == 4283459416);

        bool timerBackground = (offsetX1 >= 65.0f && (offsetX2 == 143.0f || offsetX2 <= 173.0f) &&
                            (offsetX2 - offsetX1 >= 70.0f) &&
                            offsetY1 >= 383.0f && offsetY2 <= 411.0f &&
                            (color.RGBA == 4266682200 || color.RGBA == 4283459416));

        return timerText || timerTopBorder || timerBottomBorder || timerLeftBorder || timerRightBorder || timerBackground;
    }

    inline bool ShouldOffsetRight(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return IsStealthMeterLine(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsStealthMeter(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponIconBackground(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponIconBorder(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsAlarmCounterText(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsHealthBar(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsEnvelopeIcon(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsAlarmIcon(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponIcon(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponNameText(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsAmmoText(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsBulletsImage(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsRateOfFireImage(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponAddonBackground(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponAddonBorder(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponAddonAmmoText(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsInteractionMenuBackground(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsInteractionMenuBorder(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsInteractionMenuText(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsTurretInterface(offsetX1, offsetX2, offsetY1, offsetY2, color);
    }

    inline bool ShouldOffsetLeft(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return IsTopDialogueMenuBackground(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsTopDialogueMenuBorder(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsTopDialogueMenuText(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsTimer(offsetX1, offsetX2, offsetY1, offsetY2, color);
    }
}

void WidescreenHud(float& offsetX1, float& offsetX2, float& offsetY1, float& offsetY2, FColor& Color)
{
    DBGONLY(KEYPRESS(VK_F2)
    {
        spd::log()->info("{0:f} {1:f} {2:f} {3:f} {4:08x}", offsetX1, offsetX2, offsetY1, offsetY2, Color.RGBA);
    });

    if (HudMatchers::ShouldOffsetRight(offsetX1, offsetX2, offsetY1, offsetY2, Color))
    {
        offsetX1 += Screen.fWidescreenHudOffset;
        offsetX2 += Screen.fWidescreenHudOffset;
    }
    else if (HudMatchers::ShouldOffsetLeft(offsetX1, offsetX2, offsetY1, offsetY2, Color))
    {
        offsetX1 -= Screen.fWidescreenHudOffset;
        offsetX2 -= Screen.fWidescreenHudOffset;
    }
}

void __fastcall FCanvasUtilDrawTileHook(void* _this, uint32_t EDX, float X, float Y, float SizeX, float SizeY, float U, float V, float SizeU, float SizeV, float unk1, void* Texture, FColor Color, uint32_t unk3, uint32_t unk4)
{
    static auto DrawTile = (void(__fastcall *)(void* _this, uint32_t EDX, float, float, float, float, float, float, float, float, float, void*, FColor, uint32_t, uint32_t)) pDrawTile; //0x103D2DA0;
    FColor ColBlack; ColBlack.RGBA = 0xFF000000;

    if (X == 0.0f && SizeX == 188.0f) //zoom scope 1
    {
        DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
        return;
    }

    if (X == (640.0f - 188.0f) && SizeX == 640.0f) //zoom scope 2
    {
        DrawTile(_this, EDX, SizeX + Screen.fHudOffset + Screen.fHudOffset - 188.0f, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
        return;
    }

    if ((X == 0.0f || X == 256.0f || X == 384.0f) && Y == 448.0f && (SizeX == 256.0f || SizeX == 384.0f || SizeX == 640.0f) && SizeY == 479.0f) //hiding menu background
    {
        //DrawTile(_this, EDX, 0.0f, 0.0f, 640.0f + Screen.fHudOffset + Screen.fHudOffset, Y, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4); hides all menu overlay graphics
        DrawTile(_this, EDX, 0.0f, 0.0f, 640.0f + Screen.fHudOffset + Screen.fHudOffset, 30.0f, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        DrawTile(_this, EDX, 0.0f, 447.0f, 640.0f + Screen.fHudOffset + Screen.fHudOffset, 480.0f, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        return;
    }

    if (X == 0.0f && SizeX == 64.0f) //sony ericsson overlay
    {
        if (Screen.bOpsatWidescreenMode)
        {
            DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
        else
        {
            DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset + 64.0f, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        }
    }
    if (X == 64.0f && SizeX == 320.0f) //sony ericsson overlay
    {
        if (Screen.bOpsatWidescreenMode)
        {
            DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
    }
    if (X == 320.0f && SizeX == 576.0f) //sony ericsson overlay
    {
        if (Screen.bOpsatWidescreenMode)
        {
            DrawTile(_this, EDX, X + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
    }

    if (X == 576.0f && SizeX == 640.0f) //sony ericsson overlay
    {
        if (Screen.bOpsatWidescreenMode)
        {
            DrawTile(_this, EDX, X + Screen.fHudOffset + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
        else
        {
            DrawTile(_this, EDX, 576.0f + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        }
    }

    if (X == 0.0f && (SizeX == 640.0f || SizeX == Screen.fWidth))
    {
        if ((Color.R == 0xFE && Color.G == 0xFE && Color.B == 0xFE) || (Color.R == 0xFF && Color.G == 0xFF && Color.B == 0xFF)) //flashbang grenade flash
        {
            DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
        else
        {
            DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
            DrawTile(_this, EDX, SizeX + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        }
    }

    if (Screen.bHudWidescreenMode)
        WidescreenHud(X, SizeX, Y, SizeY, Color);

    X += Screen.fHudOffset;
    SizeX += Screen.fHudOffset;

    return DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    Screen.nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 0);
    Screen.bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) != 0;
    Screen.bOpsatWidescreenMode = iniReader.ReadInteger("MAIN", "OpsatWidescreenMode", 1) != 0;
    Screen.fIniHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 140.0f);
    Screen.nPostProcessFixedScale = iniReader.ReadInteger("MAIN", "PostProcessFixedScale", 1);
    gColor.RGBA = iniReader.ReadInteger("BONUS", "GogglesLightColor", 0);
    bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    bSkipPressStartToContinue = iniReader.ReadInteger("MAIN", "SkipPressStartToContinue", 0) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    auto exePath = GetExeModulePath();

    mINI::INIStructure ini;
    mINI::INIFile mIni(iniReader.GetIniPath());
    mIni.read(ini);

    auto userIniPath = exePath / "SplinterCell2User.ini";

    // Read the existing user INI file into a structure
    mINI::INIStructure userIni;
    mINI::INIFile userIniFile(userIniPath);
    userIniFile.read(userIni);

    if (ini.has("Engine.Input"))
    {
        for (auto const& kv : ini["Engine.Input"])
        {
            std::string key = std::get<0>(kv);
            std::string value = std::get<1>(kv);
            userIni["Engine.Input"].setAll(key, value);
        }
    }
    userIni["Engine.EPCGameOptions"]["Resolution"] = std::to_string(Screen.Width) + "x" + std::to_string(Screen.Height);
    userIniFile.generate(userIni);

    auto gameIniPath = exePath / "SplinterCell2.ini";
    CIniReader iniWriter(gameIniPath);
    iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportX", Screen.Width);
    iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportY", Screen.Height);
    iniWriter.WriteString("WinDrv.WindowsClient", "UseJoystick", "True");

    auto ESettingIniPath = exePath / "ESetting.ini";
    mINI::INIStructure ESettingIni;
    mINI::INIFile ESettingIniFile(ESettingIniPath);
    ESettingIniFile.read(ESettingIni);

    for (auto const& section : ESettingIni)
    {
        std::string sectionName = std::get<0>(section);
        if (ESettingIni[sectionName].has("useAimTuning"))
        {
            ESettingIni[sectionName]["useAimTuning"] = "v=0";
        }
        //if (ESettingIni[sectionName].has("AimSpeedAdjust"))
        //{
        //    ESettingIni[sectionName]["AimSpeedAdjust"] = "v=1.0";
        //}
    }
    ESettingIniFile.write(ESettingIni);

    std::vector<std::string> list;
    GetResolutionsList(list);

    // Add current resolution to list
    auto str = format("%dx%d", Screen.Width, Screen.Height);
    if (std::find(list.begin(), list.end(), str) == list.end())
    {
        list.push_back(str);
    }

    // Sort list by resolution size (descending)
    std::sort(list.begin(), list.end(), [](const std::string& lhs, const std::string& rhs) {
        int32_t x1, y1, x2, y2;
        sscanf_s(lhs.c_str(), "%dx%d", &x1, &y1);
        sscanf_s(rhs.c_str(), "%dx%d", &x2, &y2);
        return (x1 * y1) > (x2 * y2);
    });

    // Group resolutions by string length
    std::map<size_t, std::vector<std::string>> lengthToRes;
    for (const auto& res : list)
    {
        lengthToRes[res.length()].push_back(res);
    }

    // Assign to ResList slots in reverse order (largest slots first)
    for (auto it = ResList.rbegin(); it != ResList.rend(); ++it)
    {
        size_t targetLength = it->first.length();
        auto& candidates = lengthToRes[targetLength];
        if (!candidates.empty())
        {
            // Assign the largest available resolution for this length
            it->second = std::wstring(candidates.front().begin(), candidates.front().end());
            candidates.erase(candidates.begin());  // Remove to prevent reuse
        }
        else
        {
            // No matching length available, use the template resolution
            it->second = it->first;
        }
    }

    // Ensure current resolution is in the list
    auto strWide = std::wstring(str.begin(), str.end());
    bool currentResInList = std::any_of(ResList.begin(), ResList.end(), [&strWide](const auto& pair) { return pair.second == strWide; });

    if (!currentResInList && !ResList.empty())
    {
        auto it = std::find_if(ResList.begin(), ResList.end(), [&strWide](const auto& pair) { return pair.first == strWide; });
        if (it != ResList.end())
        {
            it->second = strWide;
        }
        else
        {
            // Find a slot with matching string length and replace it
            for (auto& pair : ResList)
            {
                if (pair.second.length() == strWide.length() && pair.second != strWide)
                {
                    pair.second = strWide;
                    break;
                }
            }
        }
    }

#ifdef _DEBUG
    // Log the assignments
    for (size_t i = 0; i < ResList.size(); ++i)
    {
        spd::log()->info("{0} : {1}", std::string(ResList[i].first.begin(), ResList[i].first.end()), std::string(ResList[i].second.begin(), ResList[i].second.end()));
    }
#endif
}

SafetyHookInline shappFromAnsi = {};
wchar_t* __cdecl appFromAnsi(const char* a1)
{
    bool bReqPtr = false;
    std::vector<int> offsets = { 0, 24, 48, 74, 101 };
    for (size_t i = 0; i < ResList.size(); i++)
    {
        const auto& r = ResList[i].first;
        std::string str(r.begin(), r.end());
        if (std::string_view(a1 + offsets[i]) == str)
            bReqPtr = true;
        else
        {
            bReqPtr = false;
            break;
        }
    }

    if (bReqPtr)
    {
        for (size_t i = 0; i < ResList.size(); i++)
        {
            const auto& r = ResList[i].second;
            std::string str(r.begin(), r.end());
            injector::scoped_unprotect p{ const_cast<char*>(a1 + offsets[i]), str.size() + 1 };
            strcpy_s(const_cast<char*>(a1 + offsets[i]), str.size() + 1, str.c_str());
        }
    }

    return shappFromAnsi.unsafe_ccall<wchar_t*>(a1);
}

namespace UObject
{
    wchar_t* (__fastcall* GetFullName)(void*, void*, wchar_t*) = nullptr;

    void* pPlayerObj = nullptr;
    SafetyHookInline shGotoState = {};
    int __fastcall GotoState(void* uObject, void* edx, int StateID)
    {
        wchar_t buffer[256];
        auto objName = std::wstring_view(GetFullName(uObject, edx, buffer));

        if (objName.starts_with(L"EPlayerController "))
        {
            EPlayerControllerState = StateID;
        } else if (objName.starts_with(L"EchelonMainHUD "))
        {
            EchelonMainHUDState = StateID;
        }
        else if (objName.starts_with(L"EGameInteraction "))
        {
            EGameInteractionState = StateID;
        }
        else if (objName.starts_with(L"EElevatorPanel "))
        {
            bKeyPad = false;
            bElevatorPanel = true;
        } else if (objName.starts_with(L"EKeyPad "))
        {
            bKeyPad = true;
            bElevatorPanel = false;
        }

        return shGotoState.unsafe_fastcall<int>(uObject, edx, StateID);
    }
}

void InitCore()
{
    auto pattern = find_module_pattern(GetModuleHandle(L"Core"), "C7 85 D4 F1 FF FF 00 00 00 00", "C7 85 ? ? ? ? ? ? ? ? EB ? 8B 8D ? ? ? ? 83 C1 ? 89 8D ? ? ? ? 81 BD ? ? ? ? ? ? ? ? 0F 83"); //0x1000CE5E
    uint32_t pfappInit = (uint32_t)pattern.get_first();

    auto rpattern = hook::range_pattern(pfappInit, pfappInit + 0x900, "80 02 00 00");
    injector::WriteMemory<uint16_t>(rpattern.count(2).get(0).get<uint32_t>(-6), 0x7EEB, true); //pfappInit + 0x5FC
    injector::WriteMemory(rpattern.count(2).get(1).get<uint32_t>(0), Screen.Width, true);  //pfappInit + 0x67E + 0x1
    rpattern = hook::range_pattern(pfappInit, pfappInit + 0x900, "E0 01 00 00");
    injector::WriteMemory(rpattern.count(2).get(1).get<uint32_t>(0), Screen.Height, true); //pfappInit + 0x69F + 0x1

    // set player speed to max on game start
    pattern = hook::module_pattern(GetModuleHandle(L"Core"), "8B D9 C1 E9 02 83 E3 03 F3 A5 8B CB F3 A4 5B 5F 5E 59 8B F0");
    if (!pattern.empty())
    {
        static auto UObjectInitPropertiesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) {
            EchelonGameInfoPtrs.push_back(regs.edi - 0x2C);
        });
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Core"), "8D 47 ? 50 8D 42");
        static auto UObjectInitPropertiesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) {
            EchelonGameInfoPtrs.push_back(regs.esi);
        });
    }

    pattern = find_module_pattern(GetModuleHandle(L"Core"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 83 EC ? 53 56 57 89 65 ? C7 45 ? ? ? ? ? 83 7D ? ? 75 ? 33 C0 E9 ? ? ? ? E8 ? ? ? ? 89 45 ? 8B 45 ? 89 45 ? 33 C9 74 ? EB ? C7 45 ? ? ? ? ? EB ? 8B 55 ? 83 C2 ? 89 55 ? 81 7D ? ? ? ? ? 7D ? 8B 45 ? 03 45",
        "55 8B EC 83 EC ? 83 7D ? ? 75 ? 33 C0 EB ? E8 ? ? ? ? 89 45 ? C7 45 ? ? ? ? ? EB ? 8B 45 ? 83 C0 ? 89 45 ? 81 7D ? ? ? ? ? 7D ? 8B 4D ? 03 4D");
    shappFromAnsi = safetyhook::create_inline(pattern.get_first(), appFromAnsi);

    pattern = find_module_pattern(GetModuleHandle(L"Core"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC ? 53 56 8B 75 ? 85 F6 57 89 65 ? 8B F9 C7 45 ? ? ? ? ? 75 ? E8 ? ? ? ? 8B F0 85 FF", "55 8B EC 53 56 8B 75 ? 8B D9 85 F6 75");
    UObject::GetFullName = (decltype(UObject::GetFullName))pattern.get_first();

    pattern = find_module_pattern(GetModuleHandle(L"Core"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC ? 53 56 57 8B F1 8B 46 ? 33 FF 3B C7 89 65 ? 89 7D ? 75 ? 33 C0", "55 8B EC 83 EC ? 53 56 57 8B F9 8B 47");
    UObject::shGotoState = safetyhook::create_inline(pattern.get_first(), UObject::GotoState);

    pattern = hook::module_pattern(GetModuleHandle(L"Core"), "33 C0 0F B7 0B");
    if (!pattern.empty())
    {
        static auto OverwriteResolutions = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto str = std::wstring_view((const wchar_t*)regs.ebx);
    
            for (const auto& it : ResList)
            {
                if (str == it.first)
                {
                    wcscpy_s((wchar_t*)regs.ebx, it.second.size() + 1, it.second.c_str());
                    break;
                }
            }
        });
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Core"), "33 C0 66 85 C9");
        static auto OverwriteResolutions = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto str = std::wstring_view((const wchar_t*)regs.esi);

            for (const auto& it : ResList)
            {
                if (str == it.first)
                {
                    wcscpy_s((wchar_t*)regs.esi, it.second.size() + 1, it.second.c_str());
                    break;
                }
            }
        });
    }
}

D3DPRESENT_PARAMETERS* pPresentParams = nullptr;
SafetyHookInline shUD3DRenderDeviceSetRes = {};
int __fastcall UD3DRenderDeviceSetRes(void* UD3DRenderDevice, void* edx, void* UViewport, int width, int height, int a5)
{
    bool bInvalidRes = true;
    std::wstring resStr = std::to_wstring(width) + L"x" + std::to_wstring(height);
    for (const auto& it : ResList)
    {
        if (resStr == it.first)
        {
            swscanf(it.second.c_str(), L"%dx%d", &width, &height);
            bInvalidRes = false;
            break;
        }
    }

    if (bInvalidRes)
    {
        width = Screen.Width;
        height = Screen.Height;
        bInvalidRes = false;
    }

    auto ret = shUD3DRenderDeviceSetRes.unsafe_fastcall<int>(UD3DRenderDevice, edx, UViewport, width, height, a5);

    Screen.Width = pPresentParams->BackBufferWidth;
    Screen.Height = pPresentParams->BackBufferHeight;
    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.dHUDScaleX = static_cast<double>(Screen.fHUDScaleX);
    Screen.dHUDScaleXInv = 1.0 / Screen.dHUDScaleX;
    Screen.fFMVoffsetStartX = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.fFMVoffsetEndX = Screen.fWidth - Screen.fFMVoffsetStartX;
    Screen.fFMVoffsetStartY = 0.0f - ((Screen.fHeight - ((Screen.fHeight / 1.5f) * ((16.0f / 9.0f) / Screen.fAspectRatio))) / 2.0f);
    Screen.fFMVoffsetEndY = Screen.fHeight - Screen.fFMVoffsetStartY;
    Screen.fWidescreenHudOffset = Screen.fIniHudOffset;
    if (Screen.fAspectRatio < (16.0f / 9.0f))
        Screen.fWidescreenHudOffset = Screen.fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
    if (Screen.fAspectRatio <= (4.0f / 3.0f))
        Screen.fWidescreenHudOffset = 0.0f;

    if (Screen.Width < 640 || Screen.Height < 480)
        return ret;

    CIniReader iniReader("");
    auto [DesktopResW, DesktopResH] = GetDesktopRes();
    if (Screen.Width != DesktopResW || Screen.Height != DesktopResH)
    {
        iniReader.WriteInteger("MAIN", "ResX", Screen.Width);
        iniReader.WriteInteger("MAIN", "ResY", Screen.Height);
    }
    else
    {
        iniReader.WriteInteger("MAIN", "ResX", 0);
        iniReader.WriteInteger("MAIN", "ResY", 0);
    }

    if (pPresentParams->Windowed)
    {
        tagRECT rect;
        rect.left = (LONG)(((float)DesktopResW / 2.0f) - (Screen.fWidth / 2.0f));
        rect.top = (LONG)(((float)DesktopResH / 2.0f) - (Screen.fHeight / 2.0f));
        rect.right = (LONG)Screen.Width;
        rect.bottom = (LONG)Screen.Height;
        SetWindowLong(pPresentParams->hDeviceWindow, GWL_STYLE, GetWindowLong(pPresentParams->hDeviceWindow, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(pPresentParams->hDeviceWindow, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
        SetWindowPos(pPresentParams->hDeviceWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(pPresentParams->hDeviceWindow);
        SetCursor(NULL);
    }

    return ret;
}

void InitD3DDrv()
{
    pPresentParams = *find_module_pattern(GetModuleHandle(L"D3DDrv"), "BF ? ? ? ? 33 C0 8B D9 C1 E9 02 83 E3 03", "68 ? ? ? ? FF 15 ? ? ? ? 8B 8D").get_first<D3DPRESENT_PARAMETERS*>(1);
    auto pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "B8 01 00 00 00 8B 4D F4 64 89 0D 00 00 00 00 5F 5E 5B 8B E5 5D C2 10 00", "B8 ? ? ? ? EB ? 33 C0 89 86");
    
    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 81 EC ? ? ? ? 53 56 57 8B 7D ? 8B F1", "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 ? 56 57 50 8D 45 ? 64 A3 ? ? ? ? 8B F1 89 75");
    shUD3DRenderDeviceSetRes = safetyhook::create_inline(pattern.get_first(), UD3DRenderDeviceSetRes);

    //FMV
    //pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C1 E9 02 33 C0 F3 AB 8B CA 83 E1 03 F3 AA 8B 45 0C");
    //if (!pattern.empty())
    //{
    //    struct OpenVideo_Hook
    //    {
    //        void operator()(injector::reg_pack& regs)
    //        {
    //            regs.ecx = 0x4B000;
    //            regs.ecx >>= 2;
    //            regs.eax = 0;
    //        }
    //    }; injector::MakeInline<OpenVideo_Hook>(pattern.count(2).get(0).get<void*>(0)); //pfOpenVideo + 0x2D4
    //}
    //else
    //{
    //
    //}

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "D9 1C 24 56 56 FF 15");
    if (!pattern.empty())
    {
        injector::WriteMemory<uint8_t>(pattern.get_first(-4), 0x56, true); //push esi
        struct DisplayVideo_Hook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x04) = static_cast<float>(*(int32_t*)(regs.esp + 0x68 + 0x4));

                if (Screen.nFMVWidescreenMode)
                {
                    *(float*)(regs.esp + 0x00) = Screen.fFMVoffsetStartY;
                    *(float*)(regs.esp + 0x08) = Screen.fFMVoffsetEndY;
                }
                else
                {
                    *(float*)&regs.esi = Screen.fFMVoffsetStartX;
                    *(float*)(regs.esp + 0x04) = Screen.fFMVoffsetEndX;
                }
            }
        }; injector::MakeInline<DisplayVideo_Hook>(pattern.get_first(-3), pattern.get_first(4)); //pfDisplayVideo + 0x37E
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 04 24 ? ? ? ? FF 15 ? ? ? ? 8D 8D ? ? ? ? FF 15 ? ? ? ? 8B 8E");
        struct DisplayVideo_Hook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x00) = 0.0f;

                if (Screen.nFMVWidescreenMode)
                {
                    *(float*)(regs.esp + 0x04) = Screen.fFMVoffsetStartY;
                    *(float*)(regs.esp + 0x0C) = Screen.fFMVoffsetEndY;
                }
                else
                {
                    *(float*)(regs.esp + 0x00) = Screen.fFMVoffsetStartX;
                    *(float*)(regs.esp + 0x08) = Screen.fFMVoffsetEndX;
                }
            }
        }; injector::MakeInline<DisplayVideo_Hook>(pattern.get_first(0), pattern.get_first(7));
    }

    if (Screen.nPostProcessFixedScale)
    {
        if (Screen.nPostProcessFixedScale == 1)
            Screen.nPostProcessFixedScale = Screen.Width;

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "68 00 02 00 00 68 00 02 00 00");
        for (size_t i = 0; i < pattern.size(); i++)
        {
            if (pattern.size() == 8 && i == 2)
                continue;

            injector::WriteMemory(pattern.get(i).get<uint32_t>(1), Screen.nPostProcessFixedScale, true); //affects glass reflections
            injector::WriteMemory(pattern.get(i).get<uint32_t>(6), Screen.nPostProcessFixedScale, true);
        }

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 00 02 00 00");
        if (!pattern.empty())
            injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), Screen.nPostProcessFixedScale, true);
        else
        {
            pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 80 ? ? ? ? ? ? ? ? 56");
            injector::WriteMemory(pattern.get_first(6), Screen.nPostProcessFixedScale, true);

            pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 80 ? ? ? ? ? ? ? ? 8B 45 ? 6A");
            injector::WriteMemory(pattern.get_first(6), Screen.nPostProcessFixedScale, true);
        }
    }
}

#if _DEBUG
SafetyHookInline shFindAxisName = {};
float* __fastcall FindAxisName(void* UInput, void* edx, void* AActor, const wchar_t* a3)
{
    auto ret = shFindAxisName.unsafe_fastcall<float*>(UInput, edx, AActor, a3);

    if (std::wstring_view(a3) == L"aMouseX")
    {
        return ret;
    }
    else if (std::wstring_view(a3) == L"aMouseY")
    {
        return ret;
    }

    return ret;
}
#endif

namespace UGameEngine
{
    SafetyHookInline shPressStartToContinue = {};
    void __fastcall PressStartToContinue(void* UGameEngine, void* edx, int a2, float a3)
    {
        bPressStartToContinue = true;
        shPressStartToContinue.unsafe_fastcall(UGameEngine, edx, a2, a3);
        bPressStartToContinue = false;
    }
}

void InitEngine()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B ? 94 00 00 00 E8");
    pDrawTile = injector::GetBranchDestination(pattern.count(3).get(0).get<uintptr_t>(6), true).as_int();

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "DC 0D ? ? ? ? DB 43 18 DC 0D ? ? ? ? D9 5D E4 75 12 D9 45 10");
    if (!pattern.empty())
        injector::WriteMemory(pattern.get_first(2), &Screen.dHUDScaleX, true);
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F2 0F 5E 05 ? ? ? ? 66 0F 5A D0");
        injector::WriteMemory(pattern.get_first(4), &Screen.dHUDScaleXInv, true);
    }

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 4D F4 5F 5E 64 89 0D 00 00 00 00 5B 8B E5 5D C2 4C 00");
    if (!pattern.empty())
        injector::MakeCALL(pattern.count(1).get(0).get<uint32_t>(-5), FCanvasUtilDrawTileHook, true); //pfFUCanvasDrawTile + 0x219
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 11 0C 24 E8 ? ? ? ? 5F 5E 5B");
        injector::MakeCALL(pattern.get_first(5), FCanvasUtilDrawTileHook, true);
    }

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 4C 24 04 8B 51 44 83");
    if (!pattern.empty())
    {
        uint32_t pfsub_103762F0 = (uint32_t)pattern.get_first(0);
        auto rpattern = hook::range_pattern(pfsub_103762F0, pfsub_103762F0 + 0x800, "E8 ? ? ? ? 8B ?");
        injector::MakeCALL(rpattern.get(3).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x36E
        injector::MakeCALL(rpattern.get(5).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x43D
        injector::MakeCALL(rpattern.get(7).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x4DA
        injector::MakeCALL(rpattern.get(9).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x564
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 83 7D ? ? 8B 4D");
        injector::MakeCALL(pattern.get_first(0), FCanvasUtilDrawTileHook, true);
    }

    //FOV
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "81 EC 84 06 00 00 A1 ? ? ? ? 53 56 57");
    if (!pattern.empty())
    {
        uint32_t pfDraw = (uint32_t)pattern.get_first();
        auto rpattern = hook::range_pattern(pfDraw, pfDraw + 0x1036, "8B ? ? 03 00 00");
        struct UGameEngine_Draw_Hook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)&regs.ecx = AdjustFOV(*(float*)(regs.eax + 0x374), Screen.fAspectRatio);
            }
        }; injector::MakeInline<UGameEngine_Draw_Hook>(rpattern.get(0).get<uint32_t>(0), rpattern.get(0).get<uint32_t>(0 + 6));

        struct UGameEngine_Draw_Hook2 //1038AA8F
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)&regs.eax = AdjustFOV(*(float*)(regs.edx + 0x374), Screen.fAspectRatio);
            }
        };
        injector::MakeInline<UGameEngine_Draw_Hook2>(rpattern.get(2).get<uint32_t>(0), rpattern.get(2).get<uint32_t>(0 + 6));
        injector::MakeInline<UGameEngine_Draw_Hook2>(rpattern.get(3).get<uint32_t>(0), rpattern.get(3).get<uint32_t>(0 + 6));
    }
    else
    {
        struct UGameEngine_Draw_Hook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.xmm0.f32[0] = AdjustFOV(*(float*)(regs.eax + 0x374), Screen.fAspectRatio);
            }
        };
        
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 8D ? ? ? ? F3 0F 10 80");
        injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(6), pattern.get_first(6 + 8));
        
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 41 ? F3 0F 10 80 ? ? ? ? 8B C4");
        injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(3), pattern.get_first(3 + 8));

        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "C7 86 ? ? ? ? ? ? ? ? F3 0F 10 80");
        injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(10), pattern.get_first(10 + 8));
    }

    if (Screen.nPostProcessFixedScale)
    {
        if (Screen.nPostProcessFixedScale == 1)
            Screen.nPostProcessFixedScale = Screen.Width;

        auto pattern2 = hook::module_pattern(GetModuleHandle(L"Engine"), "68 00 02 00 00 68 00 02 00 00");
        for (size_t i = 0; i < pattern2.size(); i++)
        {
            injector::WriteMemory(pattern2.get(i).get<uint32_t>(1), Screen.nPostProcessFixedScale, true);
            injector::WriteMemory(pattern2.get(i).get<uint32_t>(6), Screen.nPostProcessFixedScale, true);
        }
    }

    if (gColor.RGBA)
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "B0 7F 88 45 24 88 45 25 88 45 26 C6 45 27 FF 8B 45 24 50 8B CB FF 52 40"); //104070CF
        if (!pattern.empty())
        {
            struct USkeletalMeshInstanceRenderHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint8_t*)(regs.ebp + 0x24) = gColor.B;
                    *(uint8_t*)(regs.ebp + 0x25) = gColor.G;
                    *(uint8_t*)(regs.ebp + 0x26) = gColor.R;
                    *(uint8_t*)(regs.ebp + 0x27) = 0xFF;
                }
            }; injector::MakeInline<USkeletalMeshInstanceRenderHook>(pattern.get_first(0), pattern.get_first(15));
        }
        else
        {
            pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "C7 85 ? ? ? ? ? ? ? ? FF B5 ? ? ? ? FF 50");
            struct USkeletalMeshInstanceRenderHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint8_t*)(regs.ebp - 0xA8) = gColor.B;
                    *(uint8_t*)(regs.ebp - 0xA7) = gColor.G;
                    *(uint8_t*)(regs.ebp - 0xA6) = gColor.R;
                    *(uint8_t*)(regs.ebp - 0xA5) = 0xFF;
                }
            }; injector::MakeInline<USkeletalMeshInstanceRenderHook>(pattern.get_first(0), pattern.get_first(10));
        }
    }

#if _DEBUG
    pattern = find_module_pattern<1>(GetModuleHandle(L"Engine"), "55 8B EC 83 EC ? 53 56 57 6A ? FF 75 ? 8D 4D ? FF 15 ? ? ? ? 83 7D ? ? 0F 84 ? ? ? ? 8B 75");
    if (!pattern.empty())
        shFindAxisName = safetyhook::create_inline(pattern.get_first(), FindAxisName);
#endif

    // LOD
    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "0F 84 ? ? ? ? FF 15 ? ? ? ? 8B 8C 24", "0F 84 ? ? ? ? 8B 41 ? F3 0F 10 81");
    injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp

    if (bSkipIntro)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "6A ? 6A ? 68 ? ? ? ? 53 E8 ? ? ? ? 83 C4 ? 68", "6A ? FF D6 85 C0");
        auto dest = find_module_pattern(GetModuleHandle(L"Engine"), "8B 83 ? ? ? ? 8B 48 ? 8B 01 83 A0", "8B 87 ? ? ? ? 8D 8D ? ? ? ? 8B 40");
        injector::MakeJMP(pattern.get_first(0), dest.get_first(0), true);
    }

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 83 C4 ? 85 C0 75 ? 8B 4F", "83 7B ? ? BE");
    static auto VideoPlaybackStartHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPlayingVideo = true;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 0D ? ? ? ? 8B 54 24 ? 89 11", "89 02 8B 13");
    static auto VideoPlaybackEndHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPlayingVideo = false;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC ? A1 ? ? ? ? 53 56 8B F1 33 C9", "55 8B EC 83 EC ? 83 3D ? ? ? ? ? 56");
    if (!pattern.empty())
        UGameEngine::shPressStartToContinue = safetyhook::create_inline(pattern.get_first(), UGameEngine::PressStartToContinue);

    if (bSkipPressStartToContinue)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "0F 84 ? ? ? ? F3 0F 10 45 ? F3 0F 59 05");
        if (!pattern.empty())
            injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp
        else
        {
            pattern = find_module_pattern(GetModuleHandle(L"Engine"), "74 ? D9 45 ? D8 0D ? ? ? ? E8");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jz -> jmp
        }
    }
}

void InitEchelon()
{
    // set player speed to max on game start
    auto pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "8B 4D ? 8B 79 ? 33 DB 47 8B C7 89 79 ? 80 38 42 89 65 ? 89 5D ? 75 ? 8B 41 ? 53 50 FF 15 ? ? ? ? D9 86",
        "8B 4D ? FF 41 ? 8B 41 ? 80 38 ? 75 ? 6A ? FF 71 ? FF 15 ? ? ? ? F3 0F 10 86 ? ? ? ? 83 EC ? F3 0F 10 8E ? ? ? ? F3 0F 59 C0 F3 0F 59 C9 F3 0F 58 C1 0F 5A C0 F2 0F 11 04 24 FF 15 ? ? ? ? F3 0F 10 25");
    static auto defautSpeedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (std::find(EchelonGameInfoPtrs.begin(), EchelonGameInfoPtrs.end(), regs.esi) != EchelonGameInfoPtrs.end())
        {
            *(uintptr_t*)(regs.esi + 0x778) = 5;
        }
        EchelonGameInfoPtrs.clear();
    });

    // Camera acceleration
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "0F 8A ? ? ? ? F3 0F 10 81 ? ? ? ? 0F 2E C1 9F F6 C4 ? 0F 8A ? ? ? ? F3 0F 10 89");
    if (!pattern.empty())
        injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jp -> jmp
    else
    {
        pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "7A ? D9 05 ? ? ? ? D9 81 ? ? ? ? DA E9 DF E0 F6 C4 ? 7A");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jp -> jmp
    }

    // Aiming camera smoothing
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "0F 84 ? ? ? ? 8B 86 ? ? ? ? DD D8", "0F 84 ? ? ? ? 8B 87 ? ? ? ? 83 EC ? F3 0F 10 80");
    injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp
}

void InitXidi()
{
    typedef bool (*RegisterProfileCallbackFunc)(const wchar_t* (*callback)());
    auto xidiModule = GetModuleHandleW(L"Xidi.dll");

    if (xidiModule)
    {
        auto RegisterProfileCallback = (RegisterProfileCallbackFunc)GetProcAddress(xidiModule, "RegisterProfileCallback");
        if (RegisterProfileCallback)
        {
            RegisterProfileCallback([]() -> const wchar_t*
            {
                constexpr auto s_Turret = 8330;
                constexpr auto s_KeyPadInteract = 8338;
                constexpr auto s_Zooming = 6942;
                constexpr auto s_PlayerSniping = 7059;
                constexpr auto s_UsingPalm = 8274;
                constexpr auto s_LaserMicTargeting = 10015;
                constexpr auto s_GameInteractionMenu = 8122;

                if (bPlayingVideo || bPressStartToContinue)
                    return L"Video";

                if (EchelonMainHUDState == 8707 || EchelonMainHUDState == 8708)
                {
                    switch (EPlayerControllerState)
                    {
                    case s_KeyPadInteract:
                    {
                        if (bElevatorPanel)
                            return L"Elevator";
                        else if (bKeyPad)
                            return L"Keypad";
                        break;
                    }
                    case s_Zooming:
                    case s_UsingPalm:
                    case s_LaserMicTargeting:
                        return L"Zooming";
                    case s_Turret:
                        return L"Turret";
                    default:
                    {
                        if (EGameInteractionState == s_GameInteractionMenu)
                            return L"GameInteractionMenu";
                        return L"Main";
                    }
                    }
                }

                return L"Main";
            });
        }
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init);
        CallbackHandler::RegisterCallback(L"Core.dll", InitCore);
        CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
        CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
        CallbackHandler::RegisterCallback(L"Echelon.dll", InitEchelon);
        CallbackHandler::RegisterCallback(L"Xidi.dll", InitXidi);
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
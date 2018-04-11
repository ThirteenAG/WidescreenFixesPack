#include "stdafx.h"

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float HUDScaleX;
    float fHudOffset;
    float fHudOffsetRight;
    float fFMVoffsetStartX;
    float fFMVoffsetEndX;
    float fFMVoffsetStartY;
    float fFMVoffsetEndY;
} Screen;

union FColor
{
    uint32_t RGBA;
    struct
    {
        uint8_t B, G, R, A;
    };
};

uint32_t nForceShadowBufferMode, nFMVWidescreenMode, nPostProcessFixedScale;
bool bHudWidescreenMode, bOpsatWidescreenMode;
float fWidescreenHudOffset;

uintptr_t pDrawTile;

void WidescreenHud(float& offsetX1, float& offsetX2, float& offsetY1, float& offsetY2, FColor& Color)
{
    DBGONLY(KEYPRESS(VK_F1) { spd::log->info("{0:f} {1:f} {2:f} {3:f} {4:08x}", offsetX1, offsetX2, offsetY1, offsetY2, Color.RGBA); });

    if (
        ((offsetX2 - offsetX1 == 17) && (offsetY1 == 337.0f || offsetY1 == 360.0f) && (offsetY2 == 368.0f || offsetY2 == 345.0f) /*&& Color.RGBA == 4266682200*/) || //stealth meter line 
        ((offsetX2 - offsetX1 == 109) && offsetY1 == 338.0f && offsetY2 == 368.0f && (Color.RGBA == 4266682200 || Color.RGBA == 4271286934)) || //stealth meter
        (offsetX1 == 491.0f && offsetX2 == 592.0f && offsetY1 == 373.0f && offsetY2 == 441.0f && Color.RGBA == 4266682200) || //weapon icon background
        ((offsetX1 == 487.0f || offsetX1 == 503.0f || offsetX1 == 580.0f) && (offsetX2 == 503.0f || offsetX2 == 580.0f || offsetX2 == 596.0f) && (offsetY1 == 369.0f || offsetY1 == 385.0f || offsetY1 == 429.0f) && Color.RGBA == 2152752984) || //weapon icon background border
        (offsetX1 >= 562.0f && offsetX2 <= 602.0f && offsetY1 == 316.0f && offsetY2 == 330.0f && Color.RGBA == 4265759816) || //alarm counter digits text
        (offsetX1 >= 562.0f && offsetX2 <= 602.0f && offsetY1 == 315.0f && offsetY2 == 330.0f && Color.RGBA == 4265759816) || //alarm counter digits text (russian)
        ((offsetX1 == 570.0f || offsetX1 == 586.0f) && (offsetX2 == 586.0f || offsetX2 == 602.0f) /*&& (offsetY1 == 125.0f || offsetY1 == 141.0f || offsetY1 == 269.0f)*/ /*&& (offsetY2 == 141.0f || offsetY2 == 269.0f || offsetY2 == 285.0f)*/ && Color.RGBA == 3360712536) || //health bar
        (offsetX1 == 492.0f && offsetX2 == 519.0f && offsetY1 == 314.0f && offsetY2 == 333.0f && Color.RGBA == 4266682200) || //envelope icon
        (offsetX1 == 521.0f && offsetX2 == 562.0f && offsetY1 == 306.0f && offsetY2 == 340.0f && (Color.RGBA == 4266682200 || Color.RGBA == 2152752984)) || //alarm icon
        ((offsetX1 == 494.0f || offsetX1 == 515.0f) && offsetY1 == 377.0f /*&& offsetY2 == 423.0f*/ && Color.RGBA == 4266682200) || //weapon icon
        (offsetX1 >= 491.0f && /*offsetX2 == 519.0f &&*/ offsetY1 == 423.0f && offsetY2 == 437.0f /*&& Color.RGBA == 4265759816*/) || //weapon name text
        (offsetX1 >= 490.0f && /*offsetX2 == 519.0f &&*/ offsetY1 == 423.0f && offsetY2 == 438.0f /*&& Color.RGBA == 4265759816*/) || //weapon name text (russian)
        (offsetX1 >= 571.0f && /*offsetX2 == 519.0f &&*/ offsetY1 == 373.0f && offsetY2 == 385.0f && Color.RGBA == 4265759816) || //ammo text
        (offsetX1 >= 571.0f && /*offsetX2 == 519.0f &&*/ offsetY1 == 373.0f && offsetY2 == 388.0f && Color.RGBA == 4265759816) || //ammo text (russian)
        (offsetX1 == 581.0f && (offsetX2 == 588.0f || offsetX2 == 592.0f) /*&& (Color.RGBA == 4266682200 || Color.RGBA == 4265759816)*/) ||  //bullets image
        (offsetX1 == 568.0f /*&& (offsetX2 == 588.0f || offsetX2 == 592.0f)*/&& offsetY1 == 406.0f /*&& (Color.RGBA == 4266682200 /*|| Color.RGBA == 4265759816)*/) ||  //another bullet image
        (offsetX1 >= 536.0f && offsetX2 <= 576.0f && offsetY1 == 416.0f && offsetY2 == 421.0f /*&& Color.RGBA == 0xFE000000*/) || //rate of fire image
        (offsetX1 == 441.0f && offsetX2 == 481.0f && offsetY1 == 373.0f && offsetY2 == 413.0f && Color.RGBA == 4266682200) || //weapon addon icon background
        (offsetX1 == 439.0f && offsetX2 == 483.0f && offsetY1 == 380.0f && offsetY2 == 412.0f && Color.RGBA == 4266682200) || //weapon addon icon
        ((offsetX1 == 437.0f || offsetX1 == 453.0f || offsetX1 == 469.0f) && (offsetX2 == 453.0f || offsetX2 == 469.0f || offsetX2 == 485.0f) && (offsetY1 == 369.0f || offsetY1 == 385.0f || offsetY1 == 401.0f) && Color.RGBA == 2152752984) || //weapon addon icon background border
        ((offsetX1 == 452.0f || offsetX1 == 456.0f || offsetX1 == 457.0f || offsetX1 == 460.0f) && (offsetX2 == 460.0f || offsetX2 == 465.0f || offsetX2 == 467.0f || offsetX2 == 469.0f) && offsetY1 == 371.0f && offsetY2 == 383.0f && Color.RGBA == 4265759816) || //weapon addon ammo text
        ((offsetX1 == 452.0f || offsetX1 == 456.0f || offsetX1 == 457.0f || offsetX1 == 460.0f) && (offsetX2 == 460.0f || offsetX2 == 464.0f || offsetX2 == 467.0f || offsetX2 == 469.0f) && offsetY1 == 371.0f && offsetY2 == 386.0f && Color.RGBA == 4265759816) || //weapon addon ammo text (russian)
        (offsetX1 == 362.0f && offsetX2 == 562.0f /*&& offsetY1 == 42.0f && offsetY2 == 66.0f*/ && (Color.RGBA == 4266682200 || Color.RGBA == 4269316740)) || //interaction menu background
        ((offsetX1 == 422.0f || offsetX1 == 443.0f) && offsetX2 == 562.0f /*&& offsetY1 == 42.0f && offsetY2 == 66.0f*/ && (Color.RGBA == 4266682200 || Color.RGBA == 4269316740)) || //interaction menu background (russian)
        ((offsetX1 == 359.0f || offsetX1 == 375.0f || offsetX1 == 549.0f || offsetX1 == 554.0f) && (offsetX2 == 375.0f || offsetX2 == 370.0f || offsetX2 == 549.0f || offsetX2 == 565.0f) && (Color.RGBA == 2152752984 || Color.RGBA == 4266682200)) || //interaction menu background border
        ((offsetX1 == 440.0f || offsetX1 == 456.0f || offsetX1 == 537.0f || offsetX1 == 554.0f) && (offsetX2 == 456.0f || offsetX2 == 451.0f || offsetX2 == 549.0f || offsetX2 == 565.0f) && (Color.RGBA == 2152752984 || Color.RGBA == 4266682200)) || //interaction menu background border (russian)
        ((offsetX1 == 419.0f || offsetX1 == 435.0f || offsetX1 == 439.0f || offsetX1 == 549.0f) && (offsetX2 == 435.0f || offsetX2 == 430.0f || offsetX2 == 549.0f || offsetX2 == 565.0f) && (Color.RGBA == 2152752984 || Color.RGBA == 4266682200)) || //interaction menu background border (russian2)
        (offsetX1 >= 365.0f && offsetY1 >= 42.0f && offsetY2 <= 192.0f && (Color.RGBA == 4265759816 || Color.RGBA == 4269316740)) || //interaction menu text
        ((offsetX1 == 371.0f || offsetX1 == 379.0f || offsetX1 == 387.0f || offsetX1 == 558.0f || offsetX1 == 566.0f) && offsetY1 >= 39.0f && offsetY2 <= 162.0f && (Color.RGBA == 4266682200 || Color.RGBA == 3036610302)) // turret interface, Color.RGBA == 4271155864 -> turret mouse area, maybe something else
        )
    {
        offsetX1 += fWidescreenHudOffset;
        offsetX2 += fWidescreenHudOffset;
    }
    else
    {
        if (
            (offsetX1 == 48.0f && offsetX2 == 348.0f && offsetY1 >= 42.0f && offsetY2 <= 87.0f && Color.RGBA == 4266682200) || //top dialogue menu background
            (offsetX1 == 48.0f && offsetX2 == 348.0f && offsetY1 >= 42.0f && offsetY2 <= 90.0f && Color.RGBA == 4266682200) || //top dialogue menu background (russian)
            ((offsetX1 == 45.0f || offsetX1 == 61.0f || offsetX1 == 335.0f) && (offsetX2 == 61.0f || offsetX2 == 335.0f || offsetX2 == 351.0f) && Color.RGBA == 4266682200) || //top dialogue menu background border
            (offsetX1 >= 52.0f && offsetY1 >= 43.0f && offsetY2 <= 185.0f && Color.RGBA == 4265759816) //top dialogue menu text
            )
        {
            offsetX1 -= fWidescreenHudOffset;
            offsetX2 -= fWidescreenHudOffset;
        }
    }
}

uint32_t DisplayVideo_HookJmp;
void __declspec(naked) DisplayVideo_Hook()
{
    static uint32_t __ECX;
    _asm
    {
        fstp    dword ptr[esp]
        push    esi
        push    esi
        mov		__ECX, ecx
        mov ecx, nFMVWidescreenMode
        cmp ecx, 0
        jz label1
        mov ecx, Screen.fFMVoffsetStartY
        mov[esp + 4h], ecx;
        mov ecx, Screen.fFMVoffsetEndY
            mov[esp + 0xC], ecx;
        jmp label2
            label1 :
        mov ecx, Screen.fFMVoffsetStartX
            mov[esp + 0h], ecx;
        mov ecx, Screen.fFMVoffsetEndX
            mov[esp + 8h], ecx;
    label2:
        mov	 ecx, __ECX
            jmp	 DisplayVideo_HookJmp
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
        if (bOpsatWidescreenMode)
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
        if (bOpsatWidescreenMode)
        {
            DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
    }
    if (X == 320.0f && SizeX == 576.0f) //sony ericsson overlay
    {
        if (bOpsatWidescreenMode)
        {
            DrawTile(_this, EDX, X + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
    }

    if (X == 576.0f && SizeX == 640.0f) //sony ericsson overlay
    {
        if (bOpsatWidescreenMode)
        {
            DrawTile(_this, EDX, X + Screen.fHudOffset + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
        else
        {
            DrawTile(_this, EDX, 576.0f + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        }
    }

    if (X == 0.0f && SizeX == 640.0f)
    {
        if (Color.R == 0xFE && Color.G == 0xFE && Color.B == 0xFE) //flashbang grenade flash
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

    if (bHudWidescreenMode)
        WidescreenHud(X, SizeX, Y, SizeY, Color);

    X += Screen.fHudOffset;
    SizeX += Screen.fHudOffset;

    return DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
}

DWORD WINAPI InitCore(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"Core"), "C7 85 D4 F1 FF FF 00 00 00 00"); //0x1000CE5E

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitCore, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle(L"Core")).count_hint(1).empty()) { Sleep(0); };

    uint32_t pfappInit = (uint32_t)pattern.get_first();

    auto rpattern = hook::range_pattern(pfappInit, pfappInit + 0x900, "80 02 00 00");
    injector::WriteMemory<uint16_t>(rpattern.count(2).get(0).get<uint32_t>(-6), 0x7EEB, true); //pfappInit + 0x5FC
    injector::WriteMemory(rpattern.count(2).get(1).get<uint32_t>(0), Screen.Width, true);  //pfappInit + 0x67E + 0x1
    rpattern = hook::range_pattern(pfappInit, pfappInit + 0x900, "E0 01 00 00");
    injector::WriteMemory(rpattern.count(2).get(1).get<uint32_t>(0), Screen.Height, true); //pfappInit + 0x69F + 0x1

    return 0;
}

DWORD WINAPI InitD3DDrv(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C1 E9 02 33 C0 F3 AB 8B CA 83 E1 03 F3 AA 8B 45 0C");

    if (pattern.count_hint(2).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitD3DDrv, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle(L"D3DDrv")).count_hint(2).empty()) { Sleep(0); };

    //FMV
    Screen.fFMVoffsetStartX = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.fFMVoffsetEndX = Screen.fWidth - Screen.fFMVoffsetStartX;
    Screen.fFMVoffsetStartY = 0.0f - ((Screen.fHeight - ((Screen.fHeight / 1.5f) * ((16.0f / 9.0f) / Screen.fAspectRatio))) / 2.0f);
    Screen.fFMVoffsetEndY = Screen.fHeight - Screen.fFMVoffsetStartY;

    struct OpenVideo_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = 0x4B000;
            regs.ecx >>= 2;
            regs.eax = 0;
        }
    }; injector::MakeInline<OpenVideo_Hook>(pattern.count(2).get(0).get<void*>(0)); //pfOpenVideo + 0x2D4

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "D9 1C 24 56 56 FF 15");
    injector::MakeJMP(pattern.get_first(0), DisplayVideo_Hook, true);//pfDisplayVideo + 0x37E
    DisplayVideo_HookJmp = (uint32_t)pattern.get_first(5);

    if (nPostProcessFixedScale)
    {
        if (nPostProcessFixedScale == 1)
            nPostProcessFixedScale = Screen.Width;

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "68 00 02 00 00 68 00 02 00 00");
        for (size_t i = 0; i < pattern.size(); i++)
        {
            injector::WriteMemory(pattern.get(i).get<uint32_t>(1), nPostProcessFixedScale, true); //affects glass reflections
            injector::WriteMemory(pattern.get(i).get<uint32_t>(6), nPostProcessFixedScale, true);
        }

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 00 02 00 00");
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), nPostProcessFixedScale, true);
    }
    return 0;
}

DWORD WINAPI InitEngine(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B ? 94 00 00 00 E8");

    if (pattern.count_hint(3).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitEngine, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle(L"Engine")).count_hint(3).empty()) { Sleep(0); };


    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.HUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);

    pDrawTile = injector::GetBranchDestination(pattern.count(3).get(0).get<uintptr_t>(6), true).as_int();

    auto rpattern = hook::range_pattern(pDrawTile, pDrawTile + 0x100, "DC 0D");
    injector::WriteMemory<double>(*rpattern.get(0).get<uint32_t*>(2), Screen.HUDScaleX, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 4D F4 5F 5E 64 89 0D 00 00 00 00 5B 8B E5 5D C2 4C 00");
    injector::MakeCALL(pattern.count(1).get(0).get<uint32_t>(-5), FCanvasUtilDrawTileHook, true); //pfFUCanvasDrawTile + 0x219


    uint32_t pfsub_103762F0 = (uint32_t)hook::module_pattern(GetModuleHandle(L"Engine"), "8B 4C 24 04 8B 51 44 83").get_first(0);
    rpattern = hook::range_pattern(pfsub_103762F0, pfsub_103762F0 + 0x800, "E8 ? ? ? ? 8B ?");
    injector::MakeCALL(rpattern.get(3).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x36E
    injector::MakeCALL(rpattern.get(5).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x43D
    injector::MakeCALL(rpattern.get(7).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x4DA
    injector::MakeCALL(rpattern.get(9).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x564


    //FOV
    uint32_t pfDraw = (uint32_t)hook::module_pattern(GetModuleHandle(L"Engine"), "81 EC 84 06 00 00 A1 ? ? ? ? 53 56 57").get_first();
    rpattern = hook::range_pattern(pfDraw, pfDraw + 0x1036, "8B ? ? 03 00 00");
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

    if (Screen.fAspectRatio < (16.0f / 9.0f))
    {
        fWidescreenHudOffset = fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
    }

    if (nPostProcessFixedScale)
    {
        if (nPostProcessFixedScale == 1)
            nPostProcessFixedScale = Screen.Width;

        auto pattern2 = hook::module_pattern(GetModuleHandle(L"Engine"), "68 00 02 00 00 68 00 02 00 00 ?");
        for (size_t i = 0; i < pattern2.size(); i++)
        {
            injector::WriteMemory(pattern2.get(i).get<uint32_t>(1), nPostProcessFixedScale, true);
            injector::WriteMemory(pattern2.get(i).get<uint32_t>(6), nPostProcessFixedScale, true);
        }
    }

    return 0;
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("C7 45 E8 00 00 00 00 8B 15");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebp - 0x18) = 0;

            CIniReader iniReader("");
            Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
            Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
            nForceShadowBufferMode = iniReader.ReadInteger("MAIN", "ForceShadowBufferMode", 1);
            nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 0);
            bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) == 1;
            bOpsatWidescreenMode = iniReader.ReadInteger("MAIN", "OpsatWidescreenMode", 1) == 1;
            fWidescreenHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 140.0f);
            nPostProcessFixedScale = iniReader.ReadInteger("MAIN", "PostProcessFixedScale", 1);

            if (!Screen.Width || !Screen.Height)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

            char UserIni[MAX_PATH];
            GetModuleFileNameA(GetModuleHandle(NULL), UserIni, (sizeof(UserIni)));
            *strrchr(UserIni, '\\') = '\0';
            strcat(UserIni, "\\SplinterCell2User.ini");

            CIniReader iniWriter(UserIni);
            char szRes[50];
            sprintf(szRes, "%dx%d", Screen.Width, Screen.Height);
            iniWriter.WriteString("Engine.EPCGameOptions", "Resolution", szRes);

            *strrchr(UserIni, '\\') = '\0';
            strcat(UserIni, "\\SplinterCell2.ini");
            iniWriter.SetIniPath(UserIni);
            iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportX", Screen.Width);
            iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportY", Screen.Height);

            *strrchr(UserIni, '\\') = '\0';
            strcat(UserIni, "\\SplinterCell.ini");
            iniWriter.SetIniPath(UserIni);
            iniWriter.WriteInteger("D3DDrv.D3DRenderDevice", "ForceShadowMode", nForceShadowBufferMode);

            InitCore(NULL);
            InitEngine(NULL);
            InitD3DDrv(NULL);
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(7));

    return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
    }
    return TRUE;
}
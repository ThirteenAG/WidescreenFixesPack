module;

#include <stdafx.h>

export module Engine;

import ComVars;
import WidescreenHUD;

namespace FCanvasUtil
{
    void(__fastcall* DrawTile)(void* _this, uint32_t EDX, float, float, float, float, float, float, float, float, float, void*, FColor);  //0x10305F01;
}

void __fastcall FCanvasUtilDrawTileHook(void* _this, uint32_t EDX, float X, float Y, float SizeX, float SizeY, float U, float V, float SizeU, float SizeV, float unk1, void* Texture, FColor Color)
{
    static FColor ColBlack; ColBlack.RGBA = 0xFF000000;

    /*if (Color.RGBA == 0xfe000000 && X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>(Screen.fWidth - Screen.fHudOffset - Screen.fHudOffset) && //cutscene borders
    ((Y == 0.0f && static_cast<uint32_t>(SizeY) <= static_cast<uint32_t>((Screen.fWidth * (3.0f / 4.0f)) / (480.0f / 60.0f))) ||
    (static_cast<uint32_t>(Y) <= static_cast<uint32_t>((Screen.fWidth * (3.0f / 4.0f)) / (480.0f / (421.0f))) && static_cast<uint32_t>(SizeY) == static_cast<uint32_t>(Screen.fHeight))))
    {
    return;
    }*/

    if (
        (X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 188.0f))) || //zoom scope borders
        (X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 43.0f))) ||  //sticky camera borders
        (X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 30.0f))) ||  //optic cable
        (X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 163.0f)))    //optic mic
        )
    {
        FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, nullptr, ColBlack);
        FCanvasUtil::DrawTile(_this, EDX, Screen.fWidth - Screen.fHudOffset, Y, Screen.fWidth, SizeY, U, V, SizeU, SizeV, unk1, nullptr, ColBlack);
    }

    if (X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f)))
    {
        if ((Color.R == 0x40 && Color.G == 0x00 && Color.B == 0x00) || (Color.R == 0x80 && Color.G == 0x80 && Color.B == 0x80)) //mission failed red screen
        {
            FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
            return;
        }
        else if ((Color.R == 0x00 && Color.G == 0x40 && Color.B == 0x00)) //mission passed green screen
        {
            FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
            return;
        }
        else
        {
            FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack);
            FCanvasUtil::DrawTile(_this, EDX, SizeX + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack);
        }
    }

    if (Screen.bHudWidescreenMode)
        WidescreenHud(X, SizeX, Y, SizeY, Color);

    X += Screen.fHudOffset;
    SizeX += Screen.fHudOffset;

    return FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
}

export void InitEngine()
{
    //HUD
    auto flt_104E9F78 = *hook::pattern(GetModuleHandle(L"Engine"), "D8 C9 D8 0D").count(9).get(0).get<float*>(4);
    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), pattern_str(0xD8, 0xC9, 0xD8, 0x0D, to_bytes(flt_104E9F78)));
    for (size_t i = 0; i < pattern.count_hint(4).size(); ++i)
    {
        injector::WriteMemory(pattern.get(i).get<void>(4), &Screen.HUDScaleX, true); //(DWORD)Engine + 0x1E9F78
    }

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B ? 94 00 00 00 E8");
    FCanvasUtil::DrawTile = (decltype(FCanvasUtil::DrawTile))injector::GetBranchDestination(pattern.count(1).get(0).get<uintptr_t>(6), true).as_int();

    injector::MakeCALL(pattern.count(1).get(0).get<uintptr_t>(6), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC8ECE

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B ? 94 00 00 00 52 E8");
    injector::MakeCALL(pattern.count(2).get(0).get<uint32_t>(7), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC9B7C
    injector::MakeCALL(pattern.count(2).get(1).get<uint32_t>(7), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC9DE1

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 46 34 8B 88 B0 02 00 00");
    struct UGameEngine_Draw_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.ecx = AdjustFOV(*(float*)(regs.eax + 0x2B0), Screen.fAspectRatio);
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.count(1).get(0).get<uint32_t>(3), pattern.count(1).get(0).get<uint32_t>(3 + 6)); //pfDraw + 0x104
}
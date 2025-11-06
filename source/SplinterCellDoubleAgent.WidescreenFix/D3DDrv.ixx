module;

#include "stdafx.h"
#include <d3d9.h>
#include <D3dx9.h>
#pragma comment(lib, "D3dx9.lib")

export module D3DDrv;

import ComVars;

float* __cdecl FGetHSV(float* dest, uint8_t H, uint8_t S, uint8_t V)
{
    auto bChangeColor = ((H == 0x41 && S == 0x33) || (H == 0x5B && S == 0 && V == 0xFF) || (H == 0x2B && S == 0x40 && V == 0xFF)); //goggles || green ind || yellow ind
    if (bChangeColor)
    {
        if (gColor)
        {
            switch (gColor)
            {
            case 2:
                dest[0] = 1.0f;
                dest[1] = 0.0f;
                dest[2] = 0.0f;
                dest[3] = 1.0f;
                break;
            case 3:
                dest[0] = 1.0f;
                dest[1] = 0.5f;
                dest[2] = 0.0f;
                dest[3] = 1.0f;
                break;
            default:
                dest[0] = 0.0f;
                dest[1] = 1.0f;
                dest[2] = 0.0f;
                dest[3] = 1.0f;
                break;
            }
            if (gBlacklistIndicators)
            {
                dest[0] *= gVisibility;
                dest[1] *= gVisibility;
                dest[2] *= gVisibility;
                dest[3] *= gVisibility;
            }
            return dest;
        }
    }

    float r, g, b, a = 1.0f;
    float v14 = (float)H * 6.0f * 0.00390625f;
    float v4 = floor(v14);
    float v5 = (float)(255 - S) * 0.0039215689f;
    float v6 = (float)V * 0.0039215689f;
    float v16 = (1.0f - v5) * v6;
    float v10 = (1.0f - (v5 * (v14 - v4))) * v6;
    float v7 = (1.0f - (v14 - v4)) * v5;
    float v15 = (float)V * 0.0039215689f;
    float v17 = (1.0f - v7) * v6;

    switch ((uint32_t)v4)
    {
    case 0:
        r = v15;
        g = v17;
        b = v16;
        break;
    case 1:
        r = v10;
        g = v15;
        b = v16;
        break;
    case 2:
        r = v16;
        g = v15;
        b = v17;
        break;
    case 3:
        r = v16;
        g = v10;
        b = v15;
        break;
    case 4:
        r = v17;
        g = v16;
        b = v15;
        break;
    case 5:
        r = v15;
        g = v16;
        b = v10;
        break;
    default:
        break;
    }

    dest[0] = r;
    dest[1] = g;
    dest[2] = b;
    dest[3] = a;

    if (bChangeColor && gBlacklistIndicators)
    {
        dest[0] *= gVisibility;
        dest[1] *= gVisibility;
        dest[2] *= gVisibility;
        dest[3] *= gVisibility;
    }

    return dest;
}

export void InitD3DDrv()
{
    CIniReader iniReader("");
    static auto nShadowMapResolution = iniReader.ReadInteger("GRAPHICS", "ShadowMapResolution", 0);
    static auto nGlowResolution = iniReader.ReadInteger("GRAPHICS", "GlowResolution", 0);

    //https://github.com/ThirteenAG/WidescreenFixesPack/issues/725#issuecomment-612613927
    if (nShadowMapResolution)
    {
        //causes bugs #1251

        if (nShadowMapResolution > 8192)
            nShadowMapResolution = 8192;

        auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 00 04 00 00 C3 ? ? ? ? ? ? ? ? ? ? B8 00 04 00 00 C3 ? ? ? ? ? ? ? ? ? ? B8 00 02 00 00 C3 ? ? ? ? ? ? ? ? ? ? B8 00 02 00 00 C3");
        if (!pattern.empty())
        {
            auto range = hook::range_pattern((uintptr_t)pattern.get_first(0), (uintptr_t)pattern.get_first(54), "B8 ? ? ? ? C3");
            range.for_each_result([&](hook::pattern_match match)
            {
                injector::WriteMemory(match.get<void>(1), nShadowMapResolution, true);
            });
        }

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "0F B6 8E ? ? ? ? B8");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(3), nShadowMapResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "6A 00 B8 ? ? ? ? D3 F8 8B 8E ? ? ? ? 8B 11");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(3), nShadowMapResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "6A 02 B8");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(3), nShadowMapResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 ? ? ? ? D3 F8 8D 8E ? ? ? ? 51 8B 8E");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(1), nShadowMapResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 ? ? ? ? D3 F8 8B 8E ? ? ? ? 8B 11 6A 01 50 50 51 FF 52 5C 0F B6 8E");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(1), nShadowMapResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 ? ? ? ? D3 F8 8B 8E ? ? ? ? 8B 09 6A 50");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(1), nShadowMapResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 ? ? ? ? D3 F8 8D 8E ? ? ? ? 51 6A 00");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(1), nShadowMapResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 ? ? ? ? EB 11");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(1), nShadowMapResolution, true);
    }

    if (nGlowResolution)
    {
        if (nGlowResolution > 2048)
            nGlowResolution = 2048;

        static injector::hook_back<void(__fastcall*)(void* _this, int edx, int a2, int a3, int a4, int a5, int a6)> sub_100B9D10;
        auto sub_100B9D10_hook = [](void* _this, int edx, int a2, int a3, int a4, int a5, int a6)
        {
            a3 = nGlowResolution;
            a4 = nGlowResolution;
            return sub_100B9D10.fun(_this, edx, a2, a3, a4, a5, a6);
        };

        const char* xrefs_arr[] = {
            "E8 ? ? ? ? EB 02 33 C0 89 86 ? ? ? ? 8B 0D",
            "E8 ? ? ? ? EB 02 33 C0 89 86 ? ? ? ? A1 ? ? ? ? 8B 08 8B 11 68 ? ? ? ? 6A 10 89 5C 24 3C FF 12 89 44 24 40 85 C0 C7 44 24 ? ? ? ? ? 74 12 6A 00 6A 15 6A 40",
            "E8 ? ? ? ? EB 02 33 C0 89 86 ? ? ? ? A1 ? ? ? ? 8B 08 8B 11 68 ? ? ? ? 6A 10 89 5C 24 3C FF 12 89 44 24 40 85 C0 C7 44 24 ? ? ? ? ? 74 12 6A 00 6A 15 6A 20",
            "E8 ? ? ? ? EB 02 33 C0 89 86 ? ? ? ? A1 ? ? ? ? 8B 08 8B 11 68 ? ? ? ? 6A 10 89 5C 24 3C FF 12 89 44 24 40 85 C0 C7 44 24 ? ? ? ? ? 74 12 6A 00 6A 15 6A 10",
            "E8 ? ? ? ? EB 02 33 C0 89 86 ? ? ? ? A1 ? ? ? ? 8B 08 8B 11 68 ? ? ? ? 6A 10 89 5C 24 3C FF 12 89 44 24 40 85 C0 C7 44 24 ? ? ? ? ? 74 12 6A 00 6A 15 6A 08",
        };

        for (auto& xref : xrefs_arr)
        {
            auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), xref);
            if (!pattern.empty())
                sub_100B9D10.fun = injector::MakeCALL(pattern.get_first(), static_cast<void(__fastcall*)(void* _this, int edx, int a2, int a3, int a4, int a5, int a6)>(sub_100B9D10_hook), true).get();
        }

        auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8B 08");
        if (!pattern.empty())
        {
            injector::WriteMemory<float>(pattern.get_first(1), 1.003910000f, true);
            injector::WriteMemory<float>(pattern.get_first(6), 0.003906250f, true);
            injector::WriteMemory<float>(pattern.get_first(23), 1.001950000f, true);
            injector::WriteMemory<float>(pattern.get_first(28), 0.001953120f, true);
            injector::WriteMemory<float>(pattern.get_first(43), 1.000980000f, true);
            injector::WriteMemory<float>(pattern.get_first(48), 0.000976562f, true);
            injector::WriteMemory<float>(pattern.get_first(63), 1.000490000f, true);
            injector::WriteMemory<float>(pattern.get_first(68), 0.000488281f, true);
        }

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 50 55");
        if (!pattern.empty())
        {
            injector::WriteMemory(pattern.get_first(1), nGlowResolution, true);
            injector::WriteMemory(pattern.get_first(6), nGlowResolution, true);
        }

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 51 50");
        if (!pattern.empty())
        {
            injector::WriteMemory(pattern.get_first(1), nGlowResolution, true);
            injector::WriteMemory(pattern.get_first(6), nGlowResolution, true);
        }

        uint8_t asm_code[] = {
            0x68, 0xCC, 0xCC, 0xCC, 0xCC,                             //push    40h
            0x68, 0xCC, 0xCC, 0xCC, 0xCC,                             //push    40h
            0x51,                                                     //push    ecx
            0x50,                                                     //push    eax
            0x8B, 0xCE,                                               //mov     ecx, esi
            0xFF, 0x92, 0xD0, 0x04, 0x00, 0x00,                       //call    dword ptr[edx + 4D0h]
        };

        const char* xrefs_arr2[] = {
            "6A 40 6A 40 51 50 8B CE FF 92",
            "6A 20 6A 20 51 50 8B CE FF 92",
            "6A 10 6A 10 51 50 8B CE FF 92",
        };

        for (auto& xref : xrefs_arr2)
        {
            auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), xref);
            if (!pattern.empty())
            {
                injector::MakeNOP(pattern.get_first(), 31);
                injector::WriteMemoryRaw(pattern.get_first(), asm_code, sizeof(asm_code), true);
                injector::WriteMemory(pattern.get_first(1), nGlowResolution, true);
                injector::WriteMemory(pattern.get_first(6), nGlowResolution, true);
            }
        }

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 44 24 ? ? ? ? ? D9 5C 24 0C");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(4), nGlowResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "89 54 24 3C C7 44 24");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(8), nGlowResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 44 24 ? ? ? ? ? E8 ? ? ? ? 8B 10 F3 0F 10 05 ? ? ? ? 8B 86 ? ? ? ? 8B 88");
        if (!pattern.empty())
        {
            injector::WriteMemory(pattern.get(0).get<void>(4), nGlowResolution, true);
            injector::WriteMemory(pattern.get(2).get<void>(4), nGlowResolution, true);
        }

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 84 24 ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B 10 F3 0F 10 05 ? ? ? ? 8B 86 ? ? ? ? 8B 88");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get(0).get<void>(7), nGlowResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 84 24 ? ? ? ? ? ? ? ? E8 ? ? ? ? F3 0F 10 05 ? ? ? ? 8B 10 6A 01");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(7), nGlowResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 88 ? ? ? ? C7 44 24 ? ? ? ? ? E8");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(10), nGlowResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "89 54 24 40 C7 44 24");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(8), nGlowResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 84 24 ? ? ? ? ? ? ? ? E8 ? ? ? ? F3 0F 10 05 ? ? ? ? 8B 10 F3 0F 11 84 24");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(7), nGlowResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 84 24 ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B 10 8B 84 24 ? ? ? ? 8B 8C 24");
        if (!pattern.empty())
            injector::WriteMemory(pattern.get_first(7), nGlowResolution, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B9 ? ? ? ? FF 25");
        if (!pattern.empty())
        {
            uint8_t asm_code[] = {
                0x6A, 0x15, 0x68, 0x00, 0x02, 0x00, 0x00, 0x68, 0x00, 0x02, 0x00, 0x00,
                0xE9, 0xD4, 0xC6, 0xF6, 0xFF, 0x6A, 0x15, 0x68, 0x00, 0x01, 0x00, 0x00,
                0x68, 0x00, 0x01, 0x00, 0x00, 0xE9, 0x03, 0xC7, 0xF6, 0xFF, 0x6A, 0x15,
                0x68, 0x80, 0x00, 0x00, 0x00, 0x68, 0x80, 0x00, 0x00, 0x00, 0xE9, 0x32,
                0xC7, 0xF6, 0xFF, 0xC7, 0x44, 0xE4, 0x6C, 0x00, 0x00, 0x00, 0x45, 0xC7,
                0x44, 0xE4, 0x70, 0x00, 0x00, 0x00, 0x45, 0xC7, 0x84, 0xE4, 0x90, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x80, 0x44, 0xC7, 0x84, 0xE4, 0x94, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x80, 0x44, 0xC7, 0x84, 0xE4, 0xB4, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x44, 0xC7, 0x84, 0xE4, 0xB8, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x44, 0xC7, 0x84, 0xE4, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x80, 0x43, 0xC7, 0x84, 0xE4, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
                0x43, 0xE8, 0x3B, 0xB6, 0xF8, 0xFF, 0xE9, 0x85, 0xE0, 0xF8, 0xFF, 0xC7,
                0x44, 0xE4, 0x70, 0x00, 0x00, 0x00, 0x45, 0xC7, 0x44, 0xE4, 0x74, 0x00,
                0x00, 0x00, 0x45, 0xC7, 0x84, 0xE4, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x80, 0x44, 0xC7, 0x84, 0xE4, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
                0x44, 0xC7, 0x84, 0xE4, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44,
                0xC7, 0x84, 0xE4, 0xBC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0xC7,
                0x84, 0xE4, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x43, 0xC7, 0x84,
                0xE4, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x43, 0xE8, 0xDF, 0xB5,
                0xF8, 0xFF, 0xE9, 0x57, 0xE2, 0xF9, 0xFF, 0x68, 0x00, 0x02, 0x00, 0x00,
                0x68, 0x00, 0x02, 0x00, 0x00, 0x51, 0xE9, 0xF9, 0xCE, 0xF8, 0xFF, 0x68,
                0x00, 0x01, 0x00, 0x00, 0x68, 0x00, 0x01, 0x00, 0x00, 0x51, 0xE9, 0x9F,
                0xCF, 0xF8, 0xFF, 0x68, 0x80, 0x00, 0x00, 0x00, 0x68, 0x80, 0x00, 0x00,
                0x00, 0x51, 0xE9, 0x3A, 0xD0, 0xF8, 0xFF
            };

            uint8_t asm_jmp[] = {
                0xE9, 0x1F, 0x1F, 0x07, 0x00
            };

            uint8_t asm_jmp2[] = {
                0xE9, 0x4D, 0x1D, 0x06, 0x00
            };
            injector::WriteMemoryRaw(pattern.get_first(11), asm_code, sizeof(asm_code), true);

            pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "E8 ? ? ? ? E8 ? ? ? ? 8B 16");
            injector::WriteMemoryRaw(pattern.get_first(0), asm_jmp, sizeof(asm_jmp), true);

            pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "F3 0F 11 84 24 ? ? ? ? E8 ? ? ? ? 5F");
            injector::WriteMemoryRaw(pattern.get_first(9), asm_jmp2, sizeof(asm_jmp2), true);

            uint8_t flts[] = {
                0x51, 0x68, 0x00, 0x40, 0x80, 0x3F, 0x68, 0x00, 0x00, 0x00, 0x3B, 0x68, 0x00, 0x00, 0x80, 0xBF, 0x68, 0x00, 0x00, 0x80,
                0x3F, 0x68, 0x00, 0x20, 0x80, 0x3F, 0x68, 0x00, 0x00, 0x80, 0x3A, 0x68, 0x00, 0x00, 0x80, 0xBF, 0x68, 0x00, 0x00, 0x80,
                0x3F, 0x68, 0x00, 0x10, 0x80, 0x3F, 0x68, 0x00, 0x00, 0x00, 0x3A
            };

            pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "51 68 00 00 82 3F 68 00 00 80 3C 68 00 00 80 BF 68 00 00 80 3F 68 00 00 81 3F 68 00 00 00 3C 68 00 00 80 BF 68 00 00 80 3F 68 00 80 80 3F 68 00 00 80 3B");
            injector::WriteMemoryRaw(pattern.get_first(0), flts, sizeof(flts), true);
        }
    }

    //Minimap aka FilmStrip
    auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 4B 04 03 C9 85 C9 89 4C 24 10"); //?RenderFilmstrip@UD3DRenderDevice@@UAE_NXZ + 0x38C
    struct RenderFilmstrip_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = Screen.FilmstripScaleX + Screen.FilmstripScaleX;
        }
    }; injector::MakeInline<RenderFilmstrip_Hook>(pattern.get_first(0));

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "03 D2 85 D2 89 54 24 10 DB 44 24 10"); //?RenderFilmstrip@UD3DRenderDevice@@UAE_NXZ + 0x350
    struct RenderFilmstrip_Hook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = Screen.FilmstripOffset;
            *(int32_t*)(regs.esp + 0x10) = regs.edx;
        }
    }; injector::MakeInline<RenderFilmstrip_Hook2>(pattern.get_first(0), pattern.get_first(6));
    injector::WriteMemory<uint16_t>(pattern.get_first(6), 0xD285, true);     //test    edx, edx

    //SetRes
    static auto pPresentParams = *hook::module_pattern(GetModuleHandle(L"D3DDrv"), "BF ? ? ? ? F3 AB 8B 0D ? ? ? ? 39 01 8B 4D").get_first<D3DPRESENT_PARAMETERS*>(1);
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 01 00 00 00 64 89 0D 00 00 00 00 5B 8B E5 5D C2 10 00");
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = 1;

            Screen.Width = pPresentParams->BackBufferWidth;
            Screen.Height = pPresentParams->BackBufferHeight;
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.fHudScaleX = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fHudScaleX2 = 2.0f / (600.0f * Screen.fAspectRatio);
            Screen.fTextScaleX = 600.0f * Screen.fAspectRatio;
            Screen.fHudOffset = ((4.0f / 3.0f) / Screen.fAspectRatio);
            Screen.FilmstripScaleX = static_cast<int32_t>(Screen.fWidth / (1280.0f / (368.0 * ((4.0 / 3.0) / (Screen.fAspectRatio)))));
            Screen.FilmstripOffset = static_cast<int32_t>((((Screen.fWidth / 2.0f) - ((Screen.fHeight * (4.0f / 3.0f)) / 2.0f)) * 2.0f) + ((float)Screen.FilmstripScaleX / 5.25f));

            RawInputHandler<>::OnResChange();

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
                SetForegroundWindow(pPresentParams->hDeviceWindow);
                SetCursor(NULL);
            }
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0));

    //ScopeLens
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 80 ? ? ? ? 8B 08 6A 14 8D 54 24 70 52 6A 02 6A 05");
    struct ScopeLensHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.eax + 0x5F30);
    
            if (Screen.fAspectRatio > (4.0f / 3.0f))
            {
                auto pVertexStreamZeroData = regs.esp + 0x6C;
                *(float*)(pVertexStreamZeroData + 0x00) /= (Screen.fAspectRatio / (16.0f / 9.0f)) / ((Screen.fDefaultARforFOV) / (Screen.fAspectRatio));
                *(float*)(pVertexStreamZeroData + 0x14) /= (Screen.fAspectRatio / (16.0f / 9.0f)) / ((Screen.fDefaultARforFOV) / (Screen.fAspectRatio));
                *(float*)(pVertexStreamZeroData + 0x28) /= (Screen.fAspectRatio / (16.0f / 9.0f)) / ((Screen.fDefaultARforFOV) / (Screen.fAspectRatio));
                *(float*)(pVertexStreamZeroData + 0x3C) /= (Screen.fAspectRatio / (16.0f / 9.0f)) / ((Screen.fDefaultARforFOV) / (Screen.fAspectRatio));
    
                *(float*)(pVertexStreamZeroData + 0x04) /= (16.0f / 9.0f) / (Screen.fDefaultARforFOV);
                *(float*)(pVertexStreamZeroData + 0x18) /= (16.0f / 9.0f) / (Screen.fDefaultARforFOV);
                *(float*)(pVertexStreamZeroData + 0x2C) /= (16.0f / 9.0f) / (Screen.fDefaultARforFOV);
                *(float*)(pVertexStreamZeroData + 0x40) /= (16.0f / 9.0f) / (Screen.fDefaultARforFOV);
            }
        }
    }; injector::MakeInline<ScopeLensHook1>(pattern.get_first(0), pattern.get_first(6));

    //loadscreen
    static LPDIRECT3DTEXTURE9 pTexLoadscreenCustom = nullptr;
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "89 BE ? ? ? ? E8 ? ? ? ? 8B 86 ? ? ? ? 8B 08 57 6A 00 50 FF 91 ? ? ? ? 6A 04 8B CB E8 ? ? ? ? 8B 6C 24 30 F6 45 54 01 74 48 83 BE"); //
    struct LoadscHook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (regs.edi)
            {
                auto pTex = (IDirect3DTexture9*)regs.edi;
                if (pTex->GetType() == D3DRTYPE_TEXTURE)
                {
                    D3DSURFACE_DESC pDesc;
                    pTex->GetLevelDesc(0, &pDesc);
                    if (pDesc.Width == 2048 && pDesc.Height == 1024 && pDesc.Format == D3DFMT_DXT5 && pDesc.Usage == 0)
                    {
                        if (!pTexLoadscreenCustom)
                        {
                            IDirect3DDevice9* ppDevice = nullptr;
                            pTex->GetDevice(&ppDevice);
                            if (D3DXCreateTextureFromFileW(ppDevice, (LPCWSTR)(Screen.szLoadscPath.u16string().c_str()), &pTexLoadscreenCustom) == D3D_OK)
                                regs.edi = (uint32_t)pTexLoadscreenCustom;
                        }
                        else
                            regs.edi = (uint32_t)pTexLoadscreenCustom;
                    }
                }
            }
            *(uint32_t*)(regs.esi + 0x6EB0) = regs.edi;
        }
    }; injector::MakeInline<LoadscHook>(pattern.get_first(0), pattern.get_first(6));

    static auto DeviceResetHook = safetyhook::create_mid(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?resetDevice@UD3DRenderDevice@@QAEXAAU_D3DPRESENT_PARAMETERS_@@@Z"), [](SafetyHookContext& regs)
    {
        auto SafeRelease = [](auto ppT)
        {
            if (*ppT)
            {
                (*ppT)->Release();
                *ppT = NULL;
            }
        };

        SafeRelease(&pTexLoadscreenCustom);
    });

    //Enhanced night vision NaN workaround
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "F3 A5 8B 90 30 02 00 00");
    injector::MakeNOP(pattern.get_first(0), 2, true);

    //Goggles Light Color
    if (gColor || gBlacklistIndicators)
    {
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "FF 15 ? ? ? ? 83 C4 10 8D AE ? ? ? ? 6A 01 8B CD 8B D8 E8");
        injector::MakeNOP(pattern.get(0).get<void>(0), 6, true);
        injector::MakeCALL(pattern.get(0).get<void>(0), FGetHSV, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "FF 15 ? ? ? ? 8B 6D 5C 0F 57 C0 83 C4 10 85 ED F3 0F 11 44 24 ? 0F 84 ? ? ? ? 8B 44 24 10 F3 0F 10 40");
        injector::MakeNOP(pattern.get(0).get<void>(0), 6, true);
        injector::MakeCALL(pattern.get(0).get<void>(0), FGetHSV, true);
    }

    {
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B D1 F7 DA");
        struct FullScreen_RefreshRateInHzHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.edx = regs.ecx != 0 ? 1000 : 0;
            }
        }; injector::MakeInline<FullScreen_RefreshRateInHzHook>(pattern.get_first(0), pattern.get_first(9));
    }
}

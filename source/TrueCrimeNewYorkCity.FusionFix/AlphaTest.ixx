module;

#include <stdafx.h>
#include "minidx8/d3d8.h"

export module AlphaTest;

import ComVars;

// Cut-out textures (bushes, flags, signs, fences): the console draws the texels with at least 15/16 alpha with depth
// and blends the rest over the picture without depth, so leaf edges stay soft. The PC port draws these strips
// unblended with an alpha reference of 8: the black texels that the filtering pulls in at every edge come out opaque
// and the leaves get dark outlines.
namespace AlphaTest
{
    constexpr DWORD ConsoleAlphaRef = 0x78 * 255 / 0x80;

    GameRef<uint32_t> nBlendState;     // the game's cached source << 16 | destination blend
    bool bConsoleAlpha = false;
    DWORD alphaRef = 0;
    DWORD alphaFunc = 0;
    DWORD zWrite = 0;

    IDirect3DDevice8* Device()
    {
        return reinterpret_cast<IDirect3DDevice8*>(pD3DDevice.get());
    }

    // An unblended strip whose texture keeps its alpha test
    bool IsCutout(const uint8_t* strip)
    {
        return nBlendState == ((D3DBLEND_ONE << 16) | D3DBLEND_ZERO) && (strip[0x0D] & 0x10);
    }

    void BeforeDraw(SafetyHookContext& regs)
    {
        bConsoleAlpha = IsCutout(reinterpret_cast<const uint8_t*>(regs.esi));
        if (!bConsoleAlpha)
            return;

        IDirect3DDevice8* device = Device();
        device->GetRenderState(D3DRS_ALPHAREF, &alphaRef);
        device->GetRenderState(D3DRS_ALPHAFUNC, &alphaFunc);
        device->GetRenderState(D3DRS_ZWRITEENABLE, &zWrite);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        device->SetRenderState(D3DRS_ALPHAREF, ConsoleAlphaRef);
        device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    }

    void AfterDraw(SafetyHookContext& regs)
    {
        if (!bConsoleAlpha)
            return;
        bConsoleAlpha = false;

        const uint8_t* strip = reinterpret_cast<const uint8_t*>(regs.esi);
        const UINT start = *(const uint16_t*)(strip + 4);
        const UINT count = (*(const uint16_t*)(strip + 2) & 0x7FFF) - 2;

        IDirect3DDevice8* device = Device();
        device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->DrawPrimitive(D3DPT_TRIANGLESTRIP, start, count);

        device->SetRenderState(D3DRS_ZWRITEENABLE, zWrite);
        device->SetRenderState(D3DRS_ALPHAFUNC, alphaFunc);
        device->SetRenderState(D3DRS_ALPHAREF, alphaRef);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
    }
}

class AlphaTestInit
{
public:
    AlphaTestInit()
    {
        WFP::onInitEvent() += []()
        {
            using namespace AlphaTest;

            auto pattern = hook::pattern("81 3D ? ? ? ? 01 00 02 00 74 ? A1");
            nBlendState.SetAddress(*pattern.get_first<uint32_t*>(2));

            // The model strip draw: DrawPrimitive(D3DPT_TRIANGLESTRIP, strip+4, (strip+2 & 0x7FFF) - 2), esi = strip
            pattern = hook::pattern("6A 05 50 FF 92 18 01 00 00 66 8B 56 02");
            static auto beforeDraw = safetyhook::create_mid(pattern.get_first(3), BeforeDraw);
            static auto afterDraw = safetyhook::create_mid(pattern.get_first(9), AfterDraw);
        };
    }
} AlphaTestInit;

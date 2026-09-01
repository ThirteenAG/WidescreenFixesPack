module;

#include <stdafx.h>
#include "common.h"

export module Radardisc;

enum eSprites
{
    HUD_FIST,
    HUD_BAT,
    HUD_PISTOL,
    HUD_UZI,
    HUD_SHOTGUN,
    HUD_AK47,
    HUD_M16,
    HUD_SNIPER,
    HUD_ROCKET,
    HUD_FLAME,
    HUD_MOLOTOV,
    HUD_GRENADE,
    HUD_DETONATOR,
    HUD_RADARDISC = 15,
    HUD_PAGER = 16,
    HUD_SITESNIPER = 20,
    HUD_SITEM16,
    HUD_SITEROCKET,
    NUM_HUD_SPRITES,
};

class CSprite2d
{
public:
    RwTexture* m_pTexture;
};

namespace CHud
{
    CSprite2d* Sprites = nullptr;
}

constexpr int rwRASTERTYPETEXTURE = 0x04;
constexpr int rwRASTERFORMAT8888 = 0x0500;
constexpr int rwRASTERLOCKWRITE = 0x01;
constexpr int rwFILTERLINEAR = 2;
constexpr int rwTEXTUREADDRESSCLAMP = 3;

RwRaster* (*RwRasterCreate)(int, int, int, int) = nullptr;
uint8_t* (*RwRasterLock)(RwRaster*, int, int) = nullptr;
RwRaster* (*RwRasterUnlock)(RwRaster*) = nullptr;
RwTexture* (*RwTextureCreate)(RwRaster*) = nullptr;
bool (*RwTextureDestroy)(RwTexture*) = nullptr;

auto RwTextureSetFilterMode = [](RwTexture* tex, int filter)
{
    auto& fa = *(uint32_t*)((uint8_t*)tex + 80);
    fa = (fa & 0xFFFFFF00u) | (filter & 0xFF);
};

auto RwTextureSetAddressing = [](RwTexture* tex, int addr)
{
    auto& fa = *(uint32_t*)((uint8_t*)tex + 80);
    fa = (fa & 0xFFFF00FFu) | ((addr & 0xF) << 8) | ((addr & 0xF) << 12);
};

class Radardisc
{
public:
    Radardisc()
    {
        WFP::onInitEvent() += []()
        {
            // If radardisc loaded from TXD is the original low-quality 64x64 texture,
            // replace its raster with a programmatically drawn high-quality ring.
            auto pattern = hook::pattern("BD ? ? ? ? C6 05 ? ? ? ? ? C6 05 ? ? ? ? ? 8B 04 DD");
            CHud::Sprites = *pattern.get_first<decltype(CHud::Sprites)>(1);

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 85 C0 A3 ? ? ? ? 75 ? 68");
            RwRasterCreate = (decltype(RwRasterCreate))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 8B 46 ? 56");
            RwRasterLock = (decltype(RwRasterLock))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 8D 4C 24 ? 8D 54 24 ? 51 8D 44 24");
            RwRasterUnlock = (decltype(RwRasterUnlock))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 89 C3 59 55");
            RwTextureCreate = (decltype(RwTextureCreate))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? C7 83 ? ? ? ? ? ? ? ? 59 8D 83");
            RwTextureDestroy = (decltype(RwTextureDestroy))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("31 ED 31 D2 89 C0 31 C0 C7 04 AD ? ? ? ? ? ? ? ? 89 D1");
            static auto CHudInitialiseHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                RwTexture* pDiscTex = CHud::Sprites[HUD_RADARDISC].m_pTexture;
                RwRaster* pDiscRaster = pDiscTex ? RwTextureGetRaster(pDiscTex) : nil;
                if (pDiscRaster && RwRasterGetWidth(pDiscRaster) == 64 && RwRasterGetHeight(pDiscRaster) == 64)
                {
                    const int SIZE = 256;
                    RwRaster* pNewRaster = RwRasterCreate(SIZE, SIZE, 0, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
                    if (pNewRaster)
                    {
                        uint32_t* pixels = (uint32_t*)RwRasterLock(pNewRaster, 0, rwRASTERLOCKWRITE);
                        if (pixels)
                        {
                            const float cx = SIZE * 0.5f;
                            const float cy = SIZE * 0.5f;
                            const float outerR = SIZE * 0.5f - 1.0f;  // leave 1px border
                            const float innerR = outerR * 0.91f;      // ring thickness ~9% of radius, matching original
                            const float feather = 1.5f;               // anti-alias width in pixels

                            for (int y = 0; y < SIZE; y++)
                            {
                                for (int x = 0; x < SIZE; x++)
                                {
                                    float dx = x - cx + 0.5f;
                                    float dy = y - cy + 0.5f;
                                    float dist = sqrtf(dx * dx + dy * dy);

                                    // Smooth alpha at outer and inner edges
                                    float outerAlpha = (outerR + feather - dist) / (feather * 2.0f);
                                    float innerAlpha = (dist - innerR + feather) / (feather * 2.0f);
                                    float alpha = outerAlpha < innerAlpha ? outerAlpha : innerAlpha;
                                    if (alpha < 0.0f) alpha = 0.0f;
                                    if (alpha > 1.0f) alpha = 1.0f;

                                    uint32_t a = (uint32_t)(alpha * 255.0f + 0.5f);
                                    // Black ring: R=0, G=0, B=0. RGBA8888 layout is identical
                                    // for both GL3 (RGBA) and D3D (BGRA) since all channels are 0.
                                    pixels[y * SIZE + x] = a << 24;
                                }
                            }
                            RwRasterUnlock(pNewRaster);
                        }

                        // Swap in the new raster, destroying the old low-res one
                        
                        if (pDiscTex)
                        {
                            RwTextureDestroy(pDiscTex);
                            CHud::Sprites[HUD_RADARDISC].m_pTexture = nil;
                        }

                        pDiscTex = RwTextureCreate(pNewRaster);
                        RwTextureSetFilterMode(pDiscTex, rwFILTERLINEAR);
                        RwTextureSetAddressing(pDiscTex, rwTEXTUREADDRESSCLAMP);
                        CHud::Sprites[HUD_RADARDISC].m_pTexture = pDiscTex;
                    }
                }
            });
        };
    }
} Radardisc;
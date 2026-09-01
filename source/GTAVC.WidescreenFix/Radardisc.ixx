module;

#include <stdafx.h>
#include "common.h"

export module Radardisc;

enum eSprites
{
    HUD_FIST,
    HUD_SITEROCKET = 41,
    HUD_RADARDISC = 50,
    HUD_SITESNIPER = 63,
    HUD_SITEM16,
    HUD_SITELASER,
    HUD_LASERDOT,
    HUD_VIEWFINDER,
    HUD_BLEEDER,
    NUM_HUD_SPRITES = 69,
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
            CIniReader iniReader("");
            auto bHQRadar = iniReader.ReadInteger("MISC", "HQRadar", 0) != 0;

            if (!bHQRadar)
                return;

            // If radardisc loaded from TXD is the original low-quality 64x64 texture,
            // replace its raster with a programmatically drawn high-quality ring.
            auto pattern = hook::pattern("BD ? ? ? ? C6 05 ? ? ? ? ? C6 05");
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

            pattern = hook::pattern("69 DB ? ? ? ? C7 05");
            static auto CHudInitialiseHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                RwTexture* pDiscTex = CHud::Sprites[HUD_RADARDISC].m_pTexture;
                RwRaster* pDiscRaster = pDiscTex ? RwTextureGetRaster(pDiscTex) : nil;
                if (pDiscRaster && RwRasterGetWidth(pDiscRaster) == 128 && RwRasterGetHeight(pDiscRaster) == 128)
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

                            const float outerR = SIZE * 0.5f - 1.0f;
                            const float innerR = outerR * 0.88f;
                            const float feather = 0.55f;
                            const float ringW = outerR - innerR;

                            const float rampPos[7] = { 0.00f, 0.16f, 0.32f, 0.50f, 0.68f, 0.84f, 1.00f };
                            const float rampR[7] = { 228.0f, 236.0f, 246.0f, 255.0f, 246.0f, 236.0f, 228.0f };
                            const float rampG[7] = { 104.0f, 138.0f, 196.0f, 245.0f, 196.0f, 138.0f, 104.0f };
                            const float rampB[7] = { 248.0f, 255.0f, 255.0f, 255.0f, 255.0f, 255.0f, 248.0f };

                            for (int y = 0; y < SIZE; y++)
                            {
                                for (int x = 0; x < SIZE; x++)
                                {
                                    float dx = x - cx + 0.5f;
                                    float dy = y - cy + 0.5f;
                                    float dist = sqrtf(dx * dx + dy * dy);

                                    float aOut = (outerR + feather - dist) / (feather * 2.0f);
                                    float aIn = (dist - innerR + feather) / (feather * 2.0f);
                                    float alpha = aOut < aIn ? aOut : aIn;
                                    if (alpha < 0.0f) alpha = 0.0f;
                                    if (alpha > 1.0f) alpha = 1.0f;

                                    float t = (dist - innerR) / ringW;
                                    if (t < 0.0f) t = 0.0f;
                                    if (t > 1.0f) t = 1.0f;

                                    int seg = 0;
                                    while (seg < 6 && t > rampPos[seg + 1])
                                        seg++;

                                    float span = rampPos[seg + 1] - rampPos[seg];
                                    float u = span > 0.0f ? (t - rampPos[seg]) / span : 0.0f;

                                    float rf = rampR[seg] + (rampR[seg + 1] - rampR[seg]) * u;
                                    float gf = rampG[seg] + (rampG[seg + 1] - rampG[seg]) * u;
                                    float bf = rampB[seg] + (rampB[seg + 1] - rampB[seg]) * u;

                                    uint32_t ri = (uint32_t)(rf + 0.5f);
                                    uint32_t gi = (uint32_t)(gf + 0.5f);
                                    uint32_t bi = (uint32_t)(bf + 0.5f);
                                    uint32_t ai = (uint32_t)(alpha * 255.0f + 0.5f);
                                    pixels[y * SIZE + x] = bi | (gi << 8) | (ri << 16) | (ai << 24);
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
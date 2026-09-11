module;

#include <stdafx.h>
#include "common.h"

export module Radardisc;

enum eSprites
{
    HUDSPRITE_FIST,
    HUDSPRITE_SITEM16,
    HUDSPRITE_SITEROCKET,
    HUDSPRITE_RADARDISC,
    HUDSPRITE_RADARRINGPLANE,
    HUDSPRITE_SKIPICON
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
            auto bHQRadarDisc = iniReader.ReadInteger("MISC", "HQRadarDisc", 0) != 0;

            if (!bHQRadarDisc)
                return;

            auto pattern = hook::pattern("BF ? ? ? ? BE ? ? ? ? BB");
            CHud::Sprites = *pattern.get_first<decltype(CHud::Sprites)>(1);

            pattern = hook::pattern("E8 ? ? ? ? 6A ? 6A ? 53 8B E8");
            RwRasterCreate = (decltype(RwRasterCreate))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 6A ? 6A ? 55 8B F0");
            RwRasterLock = (decltype(RwRasterLock))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 55 E8 ? ? ? ? 55 E8");
            RwRasterUnlock = (decltype(RwRasterUnlock))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 5F 5E 5D C6 40 ? ? 5B C3");
            RwTextureCreate = (decltype(RwTextureCreate))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 0F B7 4E");
            RwTextureDestroy = (decltype(RwTextureDestroy))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 83 C6 ? 83 C7 ? 4B");
            static auto CHudInitialiseHook = safetyhook::create_mid(pattern.get_first(14), [](SafetyHookContext& regs)
            {
                RwTexture* pDiscTex = CHud::Sprites[HUDSPRITE_RADARDISC].m_pTexture;
                RwRaster* pDiscRaster = pDiscTex ? RwTextureGetRaster(pDiscTex) : nil;
                if (pDiscRaster && RwRasterGetWidth(pDiscRaster) == 32 && RwRasterGetHeight(pDiscRaster) == 32)
                {
                    const int SIZE = 256;
                    RwRaster* pNewRaster = RwRasterCreate(SIZE, SIZE, 0, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
                    if (pNewRaster)
                    {
                        uint32_t* pixels = (uint32_t*)RwRasterLock(pNewRaster, 0, rwRASTERLOCKWRITE);
                        if (pixels)
                        {
                            const float cx = (float)SIZE;
                            const float cy = (float)SIZE;
                            const float innerR = SIZE * 0.8800f;   // 28.16/32
                            const float outerR = SIZE * 0.9900f;   // 31.68/32
                            const float feather = SIZE * 0.0109f;  // crisp anti-alias (~0.35px at 32 scale)

                            for (int y = 0; y < SIZE; y++)
                            {
                                for (int x = 0; x < SIZE; x++)
                                {
                                    float dx = x - cx + 0.5f;
                                    float dy = y - cy + 0.5f;
                                    float dist = sqrtf(dx * dx + dy * dy);

                                    // Smooth alpha at the inner edge; the outer edge exits
                                    // the texture tangentially, so it clips at the borders
                                    float alpha = (dist - innerR) / feather;
                                    float outerAlpha = (outerR - dist) / feather;
                                    if (outerAlpha < alpha) alpha = outerAlpha;
                                    if (alpha < 0.0f) alpha = 0.0f;
                                    if (alpha > 1.0f) alpha = 1.0f;

                                    uint32_t a = (uint32_t)(alpha * 255.0f + 0.5f);
                                    pixels[y * SIZE + x] = (a << 24) | 0x00FFFFFFu;
                                }
                            }
                            RwRasterUnlock(pNewRaster);
                        }

                        if (pDiscTex)
                        {
                            RwTextureDestroy(pDiscTex);
                            CHud::Sprites[HUDSPRITE_RADARDISC].m_pTexture = nil;
                        }

                        pDiscTex = RwTextureCreate(pNewRaster);
                        RwTextureSetFilterMode(pDiscTex, rwFILTERLINEAR);
                        RwTextureSetAddressing(pDiscTex, rwTEXTUREADDRESSCLAMP);
                        CHud::Sprites[HUDSPRITE_RADARDISC].m_pTexture = pDiscTex;
                    }
                }
            });
        };
    }
} Radardisc;
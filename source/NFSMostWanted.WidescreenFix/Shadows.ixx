module;

#include <stdafx.h>

export module Shadows;

import ComVars;
import Resolution;

export namespace ShadowRes
{
    constexpr uint32_t ShadowDepthCheckRes = 3072;

    // 0 = D24S8 (Shadow Map)
    // 1 = INTZ (Depth Buffer as Texture)
    // 2 = DF16 (Depth Buffer as Texture, Radeon Only)
    // 3 = DF24 (Depth Buffer as Texture, Radeon Only)
    uint32_t ShadowMapTexFormats[] = { 75, 0x5A544E49, 0x36314644, 0x34324644 };
    int CurrentTexFormat = 0;
    bool bForceSharpShadows = false;

    uint32_t Resolution = 2048;
    bool bAutoScaleShadowsRes = true;

    // X resolution ptrs
    uintptr_t dword_6C86B1;
    uint32_t dword_6C878B;
    uint32_t dword_6C87BD;
    uintptr_t dword_6C87F4;

    // Y resolution ptrs
    uintptr_t dword_6C86C1;
    uint32_t* dword_6C8786;
    uint32_t* dword_6C87B8;
    uintptr_t dword_6C87EF;

    uintptr_t DepthBiasAddr_901AC0 = 0x901AC0;
    uintptr_t DepthBiasSlopeAddr_901ABC = 0x901ABC;
    float DepthBias = 4.0f;
    float DepthBiasSlope = 4.0f;

    void update(uint32_t res, float fAspectRatio)
    {
        uint32_t resval = res;
        if (resval > 16384)
            resval = 16384;

        uint32_t resX = resval;
        if (bAutoScaleShadowsRes)
        {
            float newShadowX = resval * (fAspectRatio / (4.0f / 3.0f));
            resX = static_cast<uint32_t>(newShadowX);
        }
        uint32_t resY = resval;

        if (resX > 16384)
            resX = 16384;

        *(uint32_t*)dword_6C86B1 = resX;
        *(uint32_t*)dword_6C878B = resX;
        *(uint32_t*)dword_6C87BD = resX;
        *(uint32_t*)dword_6C87F4 = resX;

        *(uint32_t*)dword_6C86C1 = resY;
        *(uint32_t*)dword_6C8786 = resY;
        *(uint32_t*)dword_6C87B8 = resY;
        *(uint32_t*)dword_6C87EF = resY;

        if (resX > resval)
            resval = resX;

        if ((CurrentTexFormat > 0) && !bForceSharpShadows)
        {
            DepthBias = *(int32_t*)DepthBiasAddr_901AC0 * (static_cast<float>(resval) / 1024.0f);
            DepthBiasSlope = *(float*)DepthBiasSlopeAddr_901ABC * (static_cast<float>(resval) / 1024.0f);
        }
        else if (resval > ShadowDepthCheckRes)
        {
            DepthBias = *(int32_t*)DepthBiasAddr_901AC0 * (static_cast<float>(resval) / static_cast<float>(ShadowDepthCheckRes));
            DepthBiasSlope = *(float*)DepthBiasSlopeAddr_901ABC * (static_cast<float>(resval) / static_cast<float>(ShadowDepthCheckRes));
        }
    }
}

class Shadows
{
public:
    Shadows()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            ShadowRes::Resolution = iniReader.ReadInteger("GRAPHICS", "ShadowsRes", 2048);
            ShadowRes::bAutoScaleShadowsRes = iniReader.ReadInteger("GRAPHICS", "AutoScaleShadowsRes", 1) != 0;
            ShadowRes::CurrentTexFormat = iniReader.ReadInteger("GRAPHICS", "ShadowMapTextureFormat", 0);
            ShadowRes::bForceSharpShadows = iniReader.ReadInteger("GRAPHICS", "ForceSharpShadows", 0) != 0;

            bool bShadowsFix = iniReader.ReadInteger("GRAPHICS", "ShadowsFix", 1) != 0;
            bool bImproveShadowLOD = iniReader.ReadInteger("GRAPHICS", "ImproveShadowLOD", 1) != 0;

            if (bShadowsFix)
            {
                //dynamic shadow fix that stops them from disappearing when going into tunnels, under bridges, etc.
                uint32_t* dword_6DE377 = hook::pattern("75 3B C7 05 ? ? ? ? 00 00 80 3F").count(1).get(0).get<uint32_t>(0);
                injector::MakeNOP(dword_6DE377, 2, true);
                injector::WriteMemory((uint32_t)dword_6DE377 + 8, 0, true);

                //car shadow opacity
                uint32_t* dword_8A0E50 = *hook::pattern("D9 05 ? ? ? ? 8B 54 24 70 D9 1A E9 D1").count(1).get(0).get<uint32_t*>(2);
                injector::WriteMemory(dword_8A0E50, 60.0f, true);
            }

            if (ShadowRes::Resolution)
            {
                uintptr_t loc_6E5507 = reinterpret_cast<uintptr_t>(hook::pattern("52 68 C3 00 00 00 50 FF 91 E4 00 00 00").get_first(0)) + 0xD;
                uintptr_t loc_6E54E1 = loc_6E5507 - 0x26;

                ShadowRes::DepthBiasAddr_901AC0 = *(uintptr_t*)(loc_6E54E1 + 2);
                ShadowRes::DepthBiasSlopeAddr_901ABC = *(uintptr_t*)(loc_6E5507 + 2);

                struct ShadowDepthBiasHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        _asm {fld ShadowRes::DepthBias}
                    }
                }; injector::MakeInline<ShadowDepthBiasHook>(loc_6E54E1, loc_6E54E1 + 6);

                struct ShadowDepthBiasSlopeHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.edx = *(uint32_t*)(&ShadowRes::DepthBiasSlope);
                    }
                }; injector::MakeInline<ShadowDepthBiasSlopeHook>(loc_6E5507, loc_6E5507 + 6);

                uintptr_t loc_6BD32B = reinterpret_cast<uintptr_t>(hook::pattern("89 56 08 89 46 0C C7 46 04 0F 00 00 00 89 7E 10 E8").get_first(0)) + 0x10;
                uintptr_t loc_6BD333 = loc_6BD32B + 8;

                ShadowRes::dword_6C86B1 = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6BD32B)) + 1;
                ShadowRes::dword_6C86C1 = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6BD333)) + 1;

                ShadowRes::dword_6C8786 = hook::pattern("68 00 04 00 00 68 00 04 00 00 50 FF 51 5C 85 C0 7C 32").count(1).get(0).get<uint32_t>(1);
                ShadowRes::dword_6C878B = (uint32_t)ShadowRes::dword_6C8786 + 5;
                ShadowRes::dword_6C87B8 = hook::pattern("68 00 04 00 00 68 00 04 00 00 50 FF 52 5C 85 C0 7D 36").count(1).get(0).get<uint32_t>(1);
                ShadowRes::dword_6C87BD = (uint32_t)ShadowRes::dword_6C87B8 + 5;

                uintptr_t loc_6C87E5 = reinterpret_cast<uintptr_t>(hook::pattern("68 44 46 31 36 6A 02 6A 01 68").get_first(0));
                ShadowRes::dword_6C87EF = loc_6C87E5 + 0xA;
                ShadowRes::dword_6C87F4 = ShadowRes::dword_6C87EF + 5;

                DWORD oldprotect;
                injector::UnprotectMemory(ShadowRes::dword_6C86B1, sizeof(uint32_t), oldprotect);
                injector::UnprotectMemory(ShadowRes::dword_6C86C1, sizeof(uint32_t), oldprotect);
                injector::UnprotectMemory(ShadowRes::dword_6C8786, sizeof(uint32_t), oldprotect);
                injector::UnprotectMemory(ShadowRes::dword_6C878B, sizeof(uint32_t), oldprotect);
                injector::UnprotectMemory(ShadowRes::dword_6C87B8, sizeof(uint32_t), oldprotect);
                injector::UnprotectMemory(ShadowRes::dword_6C87BD, sizeof(uint32_t), oldprotect);
                injector::UnprotectMemory(ShadowRes::dword_6C87F4, sizeof(uint32_t), oldprotect);
                injector::UnprotectMemory(ShadowRes::dword_6C87EF, sizeof(uint32_t), oldprotect);

                onResChange() += [](int Width, int Height)
                {
                    float fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
                    ShadowRes::update(ShadowRes::Resolution, fAspectRatio);
                };
            }

            if (ShadowRes::CurrentTexFormat >= 0)
            {
                uintptr_t loc_6C87E5 = reinterpret_cast<uintptr_t>(hook::pattern("68 44 46 31 36 6A 02 6A 01 68").get_first(0));
                uintptr_t loc_6C8798 = loc_6C87E5 - 0x4D;
                uintptr_t loc_6C87D5 = loc_6C87E5 - 0x10;
                uintptr_t loc_6C87A2 = loc_6C87E5 - 0x43;
                uintptr_t loc_6C87B2 = loc_6C87E5 - 0x33;

                uintptr_t loc_6C174E = reinterpret_cast<uintptr_t>(hook::pattern("68 44 46 31 36 6A 03 6A 02 6A 16 6A 01 52 50 FF").get_first(0));
                uintptr_t loc_6C1703 = loc_6C174E - 0x4B;
                uintptr_t loc_6C170A = loc_6C174E - 0x44;
                uintptr_t loc_6C1719 = loc_6C174E - 0x35;
                uintptr_t loc_6C172C = loc_6C174E - 0x22;
                uintptr_t loc_6C1741 = loc_6C174E - 0xD;
                uintptr_t loc_6C1764 = loc_6C174E + 0x16;

                uintptr_t dword_982C08 = *reinterpret_cast<uintptr_t*>(loc_6C172C + 2);

                if (ShadowRes::CurrentTexFormat > (_countof(ShadowRes::ShadowMapTexFormats) - 1))
                    ShadowRes::CurrentTexFormat = (_countof(ShadowRes::ShadowMapTexFormats) - 1);

                // disable writes to the shadow map texture type variable
                injector::MakeNOP(loc_6C1703, 6);
                injector::MakeNOP(loc_6C172C, 10);
                injector::MakeNOP(loc_6C1764, 10);

                if (ShadowRes::ShadowMapTexFormats[ShadowRes::CurrentTexFormat] < 0x7F)
                {
                    injector::MakeJMP(loc_6C8798, loc_6C87A2);
                    injector::WriteMemory<uint8_t>(loc_6C87B2 + 1, ShadowRes::ShadowMapTexFormats[ShadowRes::CurrentTexFormat] & 0xFF, true);

                    injector::MakeNOP(loc_6C170A, 2);
                    injector::WriteMemory<uint8_t>(loc_6C1719 + 1, ShadowRes::ShadowMapTexFormats[ShadowRes::CurrentTexFormat] & 0xFF, true);
                }
                else
                {
                    injector::MakeJMP(loc_6C8798, loc_6C87D5);
                    injector::WriteMemory<uint32_t>(loc_6C87E5 + 1, ShadowRes::ShadowMapTexFormats[ShadowRes::CurrentTexFormat], true);

                    injector::MakeJMP(loc_6C170A, loc_6C1741);
                    injector::WriteMemory<uint32_t>(loc_6C174E + 1, ShadowRes::ShadowMapTexFormats[ShadowRes::CurrentTexFormat], true);
                }

                // Var at 0x00982C08
                // 1 = Sample Shadow Map Directly, 2 = Depth Buffer as Texture
                if (ShadowRes::CurrentTexFormat == 0)
                    *(uint32_t*)dword_982C08 = 1;
                else
                    *(uint32_t*)dword_982C08 = 2;
            }

            if (ShadowRes::bForceSharpShadows)
            {
                uintptr_t loc_6D5F3A = reinterpret_cast<uintptr_t>(hook::pattern("83 E8 03 89 44 24 14 EB 04 3B C3 7F 44").get_first(0));
                injector::WriteMemory<uint8_t>(loc_6D5F3A + 2, 2, true);
            }

            if (bImproveShadowLOD)
            {
                uint32_t* dword_6E5174 = hook::pattern("68 ? ? ? ? EB ? A1 ? ? ? ? 0D").count(1).get(0).get<uint32_t>(1);
                injector::WriteMemory(dword_6E5174, 0x00000000, true);
                uint32_t* dword_6BFFA2 = hook::pattern("68 ? ? ? ? 50 41 68").count(2).get(1).get<uint32_t>(1);
                injector::WriteMemory(dword_6BFFA2, 0x00006102, true);
            }
        };
    }
} Shadows;
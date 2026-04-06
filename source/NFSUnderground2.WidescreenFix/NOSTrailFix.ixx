module;

#include <stdafx.h>

export module NOSTrailFix;

import ComVars;

namespace NOSTrailFix
{
    static uint32_t NOSTrailFrameDelay = 1;
    uint32_t* eFrameCounter_870818 = (uint32_t*)0x00870818;
    uint32_t* fc_12989_8A44EC = (uint32_t*)0x008A44EC;
    uint32_t* NOSTrailCaveExitTrue = (uint32_t*)0x0061AD91;
    uint32_t* NOSTrailCaveExitFalse = (uint32_t*)0x61AE8E;
    void __declspec(naked) NOSTrailCave()
    {
        if ((*fc_12989_8A44EC + NOSTrailFrameDelay) <= *eFrameCounter_870818)
            _asm
        {
            mov eax, ds:eFrameCounter_870818
            mov eax, [eax]
            jmp NOSTrailCaveExitTrue
        }
        else
            _asm jmp NOSTrailCaveExitFalse
    }
}

export void InitNOSTrailFix()
{
    CIniReader iniReader("");
    bool bFixNOSTrailLength = iniReader.ReadInteger("NOSTrail", "FixNOSTrailLength", 1) == 1;
    static float fCustomNOSTrailLength = iniReader.ReadFloat("NOSTrail", "CustomNOSTrailLength", 1.0f);

    if (bFixNOSTrailLength)
    {
        static int TargetRate = 60;

        if (nFPSLimit)
            TargetRate = nFPSLimit;

        static uint32_t NOSTargetFPS = 60; // original FPS we're targeting from. Consoles target 60 but run at 30, hence have longer trails than PC. Targeting 60 is smarter due to less issues with shorter trails. Use SimRate -2 to get the same effect as console versions.
        static float fNOSTrailFrameDelay = ((float)TargetRate / (float)NOSTargetFPS) * fCustomNOSTrailLength;
        NOSTrailFix::NOSTrailFrameDelay = (uint32_t)round(fNOSTrailFrameDelay);

        auto pattern = hook::pattern("81 C1 15 02 00 00 C1 E1 04"); // 0x0061AE69
        NOSTrailFix::eFrameCounter_870818 = *pattern.count(1).get(0).get<uint32_t*>(-0xE8);
        NOSTrailFix::fc_12989_8A44EC = *pattern.count(1).get(0).get<uint32_t*>(-0xE2);
        NOSTrailFix::NOSTrailCaveExitTrue = (uint32_t*)pattern.get_first(-0xD8);
        NOSTrailFix::NOSTrailCaveExitFalse = (uint32_t*)pattern.get_first(0x25);
        injector::MakeJMP(pattern.get_first(-0xE9), NOSTrailFix::NOSTrailCave, true);
    }
}
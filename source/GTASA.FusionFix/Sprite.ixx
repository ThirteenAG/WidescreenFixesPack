module;

#include <stdafx.h>
#include "common.h"

export module Sprite;

import Draw;
import Camera;
import Skeleton;

bool CalcScreenCoors(const RwV3d* in, RwV3d* out, float* outw, float* outh, bool checkMaxVisible, bool checkMinVisible, float minVisibleZ)
{
    CVector viewvec = TheCamera->m_mViewMatrix * *in;
    *out = viewvec;

    if (checkMinVisible && out->z <= minVisibleZ)
        return false;
    if (checkMaxVisible && out->z >= CDraw::GetFarClipZ())
        return false;

    const float recip = 1.0f / out->z;
    out->x *= SCREEN_WIDTH * recip;
    out->y *= SCREEN_HEIGHT * recip;

    const float fovScale = 70.0f / CDraw::GetFOV();
    *outw = CDraw::ms_bFixSprites ? (fovScale * recip * SCREEN_HEIGHT) : (fovScale * SCREEN_SCALE_AR(recip) * SCREEN_WIDTH);
    *outh = fovScale * recip * SCREEN_HEIGHT;

    return true;
}

SafetyHookInline shCalcScreenCoors6 = {};
bool __cdecl CalcScreenCoors6(const RwV3d* in, RwV3d* out, float* w, float* h, char checkMaxVisible, char checkMinVisible)
{
    return CalcScreenCoors(in, out, w, h, checkMaxVisible != 0, checkMinVisible != 0, CDraw::GetNearClipZ() + 1.0f);
}

SafetyHookInline shCalcScreenCoors4 = {};
bool __cdecl CalcScreenCoors4(const CVector* point, CVector* outPoint, float* w, float* h)
{
    auto in = reinterpret_cast<const RwV3d*>(point);
    auto out = reinterpret_cast<RwV3d*>(outPoint);
    return CalcScreenCoors(in, out, w, h, false, true, 1.0f);
}

class Sprite
{
public:
    Sprite()
    {
        WFP::onGameInitEvent() += []()
        {
            auto p6 = hook::pattern("E8 ? ? ? ? 83 C4 ? 84 C0 0F 84 ? ? ? ? 66 8B 0D");
            shCalcScreenCoors6 = safetyhook::create_inline(injector::GetBranchDestination(p6.get_first()).as_int(), CalcScreenCoors6);

            auto p4 = hook::pattern("E8 ? ? ? ? 83 C4 ? 84 C0 5B 0F 84 ? ? ? ? 8A 85");
            shCalcScreenCoors4 = safetyhook::create_inline(injector::GetBranchDestination(p4.get_first()).as_int(), CalcScreenCoors4);
        };
    }
} Sprite;
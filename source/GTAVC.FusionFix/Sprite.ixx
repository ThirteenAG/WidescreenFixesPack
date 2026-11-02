module;

#include <stdafx.h>
#include "common.h"

export module Sprite;

import Draw;
import Camera;
import Skeleton;

SafetyHookInline shCalcScreenCoors = {};
bool __cdecl CalcScreenCoors(const RwV3d* in, RwV3d* out, float* outw, float* outh, bool farclip)
{
    CVector viewvec = TheCamera->m_viewMatrix * *in;
    *out = viewvec;
    if (out->z <= CDraw::GetNearClipZ() + 1.0f)
        return false;
    if (out->z >= CDraw::GetFarClipZ() && farclip)
        return false;
    float recip = 1.0f / out->z;
    out->x *= SCREEN_WIDTH * recip;
    out->y *= SCREEN_HEIGHT * recip;
    const float fov = 70.0f;
    // this is used to scale correctly if you zoom in with sniper rifle
    float fovScale = fov / CDraw::GetFOV();

    *outw = CDraw::ms_bFixSprites ? (fovScale * recip * SCREEN_HEIGHT) : (fovScale * SCREEN_SCALE_AR(recip) * SCREEN_WIDTH);
    *outh = fovScale * recip * SCREEN_HEIGHT;

    return true;
}

bool __cdecl CalcParticleScreenCoors(const RwV3d* in, RwV3d* out, float* outw, float* outh, bool farclip)
{
    const bool visible = shCalcScreenCoors.unsafe_ccall<bool>(in, out, outw, outh, farclip);
    if (visible)
    {
        // Particle textures use the original 4:3 proportions. Keep the native
        // projection and clipping, and correct only their horizontal size.
        *outw = SCREEN_SCALE_AR(*outw);
    }
    return visible;
}

class Sprite
{
public:
    Sprite()
    {
        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 84 C0 0F 84 ? ? ? ? 0F B7 05");
            shCalcScreenCoors = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CalcScreenCoors);

            // CParticle::Render, including the projection used for particle trails.
            pattern = hook::pattern("E8 ? ? ? ? 83 C4 14 84 C0 D9 EE 0F 84 ? ? ? ? 83 7C 24 14 21");
            injector::MakeCALL(pattern.get_first(0), CalcParticleScreenCoors, true);
            pattern = hook::pattern("E8 ? ? ? ? 83 C4 14 84 C0 D9 EE D9 EE D9 EE 74");
            injector::MakeCALL(pattern.get_first(0), CalcParticleScreenCoors, true);
        };
    }
} Sprite;
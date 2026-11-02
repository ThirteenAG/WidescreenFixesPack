module;

#include <stdafx.h>

export module Hud;

import ComVars;

typedef int Sint32;
typedef Sint32 SCR_f;
typedef unsigned int undefined4;
typedef unsigned int uint;
typedef unsigned short undefined2;

struct CameraPos
{
    SCR_f x;
    SCR_f y;
    SCR_f z;
    int zoom;
};

struct WorldRect
{
    SCR_f left;
    SCR_f right;
    SCR_f top;
    SCR_f bottom;
};

struct CameraOrPhysics
{
    CameraPos cameraPosTarget2;
    CameraPos cameraPosTarget;
    WorldRect cameraBoundaries;
    enum PLAYER_PHYSICS_MOVEMENT movementBitmask;
    struct Ped* ped;
    undefined4 field_0x38;
    int followedPedID;
    int targetElevation;
    int flyTimerMaybe;
    SCR_f altMovingPosX;
    SCR_f altMovingPosY;
    SCR_f altMovingStateUp;
    SCR_f altMovingStateDown;
    SCR_f altMovingStateLeft;
    SCR_f altMovingStateRight;
    uint altMovingArrowsRelated;
    undefined2 field_0x64;
    short altMovingLimit;
    int screenPxWidth;
    int screenPxHeight;
    int screenPxCenterX;
    int screenPxCenter;
    WorldRect cameraBoundariesNonNegative;
    CameraPos cameraPos2;
    CameraPos cameraPos;
    int uiScale;
    CameraPos cameraVelocity;
};

export int posType = 0; // 0 = default, 1 = centered, 2 = unscaled

// The width the HUD box adds to the game's 4:3 HUD: the whole screen's, or that of the HUD Aspect Ratio
float HudBoxOffset()
{
    if (!fHudAspectRatioConstraint || *fHudAspectRatioConstraint <= 0.0f || *fHudAspectRatioConstraint > 32.0f / 9.0f)
        return hud_offset();
    return std::clamp(screen_height() * ClampHudAspectRatio(*fHudAspectRatioConstraint, screen_aspect_ratio()) - screen_height() * default_aspect_ratio, 0.0f, std::max(0.0f, hud_offset()));
}

void RePositionElement(void* addr) {
    static injector::hook_back<int* (__fastcall*)(int*, int, int*, int*)> rePositionElement;
    auto f = [](int* in, int, int* out, int* scale) {
        out = rePositionElement.fun(in, 0, out, scale);

        float x = *out / fone;
        if (posType == 0) {
            // Shift this element approximatively, to the side of the HUD box it belongs to.
            // The side is decided by the element's own position, before any shift.
            float box = HudBoxOffset();
            float f = screen_height() / default_screen_height;
            float original = x;
            x += (hud_offset() - box) / 2;
            if (original > 140.0f * f && original < 490.0f * f) {
                x += box / 2;
            }
            else if (original >= 490.0f * f) {
                x += box;
            }
        }
        else if (posType == 1) {
            x += (hud_offset() / 2);
        }
        else if (posType == 2) {
            x /= hud_scale();
            x *= default_hud_scale();
        }
        posType = 0;

        *out = (uint32_t)(x * one);
        return out;
    };
    rePositionElement.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (int*(__fastcall*)(int*, int, int*, int*))f);
}

export void Rescale(int& scale) {
    float fs = (scale / fone) * hud_scale();
    scale = (int32_t)(fs * one);
}

export void Repos(int& x, int& y) {
    float fx = (x / fone) * hud_scale();
    float fy = (y / fone) * hud_scale();
    fx += ((int32_t)(hud_offset() / 2));

    x = (int32_t)(fx * one);
    y = (int32_t)(fy * one);
}

void Repos3d(int& x, int& y) {
    float fx = (x / fone) * hud_scale();
    float fy = (y / fone);
    //fx += ((int32_t)(hud_offset / 2));

    x = (int32_t)(fx * one);
    y = (int32_t)(fy * one);
}

template<int te>
void SetPosType(void* addr)
{
    static injector::hook_back<int* (__fastcall*)(int*, int, int)> encodeFlt;
    auto f = [](int* a, int, int b) {
        encodeFlt.fun(a, 0, b);
        posType = te;
    };
    encodeFlt.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (void(__fastcall*)(int*, int, int))f);
}

void ReposSpriteCall3D(void* addr)
{
    static injector::hook_back<void(__stdcall*)(int, int, int, int, int, int, const int*, int, int, int, int)> drawSprite;
    auto f = [](int id1, int id2, int x, int y, int angle, int scale, const int* mode, int enableAlpha, int alpha, int a10, int lightFlag) {
        Repos3d(x, y);
        drawSprite.fun(id1, id2, x, y, angle, scale, mode, enableAlpha, alpha, a10, lightFlag);
    };
    drawSprite.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (void(__stdcall*)(int, int, int, int, int, int, const int*, int, int, int, int))f);
}

export namespace GTA2Hud
{
    void Install()
    {
        auto pattern = hook::pattern("B9 2F 00 00 00 F3 A5"); //0x4A80CD 0x4A6257
        struct CameraZoom
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = 0x2F;

                *(int32_t*)(regs.ebp + 0x138) = (uint32_t)(hud_scale() * one);
                *(int32_t*)(regs.ebp + 0x2B0) = (uint32_t)(hud_scale() * one); // for Zaibatsu [It was an Accident!] mission

                *(int32_t*)(regs.esi + 0x8) += (uint32_t)(fZoom * one);
            }
        }; injector::MakeInline<CameraZoom>(pattern.count(2).get(1).get<void*>(0));

        // Ped Shadows
        pattern = hook::pattern("A1 ? ? ? ? 8B 48 38 81 C1 ? ? ? ? E8 ? ? ? ? 84 C0"); //0x5EB4FC
        ptrToGame.SetAddress(*pattern.get_first<uint32_t*>(1));

        pattern = hook::pattern("81 C1 ? ? ? ? E8 ? ? ? ? 8B C8 E8 ? ? ? ? D9 05"); //0x4BE38E
        injector::MakeNOP(pattern.get_first(0), 6, true);
        static auto PedShadowsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto ph1 = PtrWalkthrough<CameraOrPhysics>(ptrToGame.get_ptr(), 0x38, 0x90);

            if (ph1)
            {
                static uint32_t shadowsDummyUIScale = 0;
                auto playerph1 = *ph1;
                auto cameraZPos = (float)playerph1.cameraPos.z / 16384.0f;
                auto shadowsDistance = (1.0f / (cameraZPos + 7.0f)) * 8.0f;
                shadowsDummyUIScale = static_cast<uint32_t>(shadowsDistance * playerph1.uiScale);
                regs.ecx = (uintptr_t)&shadowsDummyUIScale;
                return;
            }

            regs.ecx += 0x0A8;
        });

        // Hud
        pattern = hook::pattern("E8 ? ? ? ? 2B FB");
        RePositionElement(pattern.get_first(-5962)); // 0x4C72AA

        pattern = hook::pattern("E8 ? ? ? ? 8B 76 54");
        RePositionElement(pattern.get_first(-3233)); // 0x4BA2F4

        pattern = hook::pattern("E8 ? ? ? ? 81 3B ? ? ? ?");
        RePositionElement(pattern.get_first(-7509)); // 0x4C71DA

        pattern = hook::pattern("E8 ? ? ? ? C6 44 24 ? ? EB 7C");
        RePositionElement(pattern.get_first(-359)); // 0x44B4BA

        pattern = hook::pattern("E8 ? ? ? ? 81 C6 ? ? ? ? 56");
        SetPosType<1>(pattern.get_first(0)); // Big messages 0x4C8A7D

        pattern = hook::pattern("E8 ? ? ? ? 66 0F B6 86 ? ? ? ?");
        SetPosType<1>(pattern.get_first(0)); // Subtitle sprite 0x4C946F

        pattern = hook::pattern("6A 40 E8 ? ? ? ? 56");
        SetPosType<1>(pattern.get_first(2)); // Subtitle text 0x4C94D7

        pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 0B 8B 0D ? ? ? ? E8 ? ? ? ? FF 15 ? ? ? ?");
        SetPosType<1>(pattern.get_first(450950)); // Quit text 1 0x4C87A5

        pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 0B 8B 0D ? ? ? ? E8 ? ? ? ? FF 15 ? ? ? ?");
        SetPosType<1>(pattern.get_first(451046)); // Quit text 2 0x4C8805

        pattern = hook::pattern("50 E8 ? ? ? ? 57 E8 ? ? ? ? 5F");
        SetPosType<1>(pattern.get_first(1)); // Quit text 3 0x4C8867

        pattern = hook::pattern("E8 ? ? ? ? 56 53");
        SetPosType<1>(pattern.get_first(0)); // Stats sprite 0x4CA1BC

        pattern = hook::pattern("57 E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 5F");
        SetPosType<1>(pattern.get_first(1)); // Stats text 0x4CA40C

        pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8B CB E8 ? ? ? ? 5D");
        SetPosType<2>(pattern.get_first(-14347)); // 3d Text 0x4BAD41

        pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8B CB E8 ? ? ? ? 5D");
        SetPosType<2>(pattern.get_first(-14202)); // 3d Text 0x4BADD2

        pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 89 5C 24 18");
        SetPosType<2>(pattern.get_first(0)); // 3d Text 0x4BAE7B

        pattern = hook::pattern("E8 ? ? ? ? 8B 15 ? ? ? ? 8B 44 24 1C");
        SetPosType<2>(pattern.get_first(0)); // 3d Text 0x4BAF3D

        pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ?");
        SetPosType<1>(pattern.get_first(-2986)); // Zone name 0x4C98F0

        pattern = hook::pattern("55 E8 ? ? ? ? 68 ? ? ? ?");
        SetPosType<1>(pattern.get_first(1)); // Zone name 0x4C9933

        pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ?");
        SetPosType<1>(pattern.get_first(-2844)); // Zone name 0x4C997E

        pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ?");
        SetPosType<1>(pattern.get_first(-2774)); // Zone name 0x4C99C4

        pattern = hook::pattern("E8 ? ? ? ? 03 F5 56");
        SetPosType<1>(pattern.get_first(0)); // Zone name 0x4C9A29

        pattern = hook::pattern("E8 ? ? ? ? 8B 46 04 6A 00");
        ReposSpriteCall3D(pattern.get_first(-495)); // Money messages 0x4B9424

        pattern = hook::pattern("74 7B 68 ? ? ? ?");
        injector::MakeNOP(pattern.get_first(0), 2, true); // 0x4B93AC

        pattern = hook::pattern("74 4D 8D 4C 24 14");
        injector::MakeNOP(pattern.get_first(0), 2, true); // 0x4B93DA
    }
}

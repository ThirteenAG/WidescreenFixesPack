module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Frontend;

// Menus, the title and other screen-space 2D: an 800x600 space stretched over the whole screen. It gets a centered 4:3
// box; pieces as wide as the whole space (backdrops, fades) keep covering the screen.
namespace ScreenSpace
{
    struct Scale
    {
        float x = 1.0f, y = 1.0f;
        float Square() const { return std::min(x, y); }
        float OffsetX() const { return 400.0f * (x - Square()); } // pixels
        float OffsetY() const { return 300.0f * (y - Square()); }
    };

    bool Boxed(const uint8_t* object)
    {
        return *reinterpret_cast<const float*>(object + 128) < 799.0f;
    }

    // Sprites: the scale matrix gets the square scale and the box's offset
    Scale sprite;
    bool spriteBoxed = false;
    using ScaleMatrixFn = int(__thiscall*)(float* matrix, float x, float y, float z);
    ScaleMatrixFn ScaleMatrix = nullptr;
    int __fastcall BoxedScaleMatrix(float* matrix, void*, float x, float y, float z)
    {
        if (!spriteBoxed)
            return ScaleMatrix(matrix, x, y, z);
        const int result = ScaleMatrix(matrix, sprite.Square(), sprite.Square(), z);
        matrix[12] += sprite.OffsetX();
        matrix[13] += sprite.OffsetY();
        return result;
    }

    // Text: the font's scale is the square one, the box's offset moves the text's position
    Scale text;
    SafetyHookInline TextScaleHook;
    float* __cdecl TextScale(float* out)
    {
        TextScaleHook.ccall<float*>(out);
        text = { out[0], out[1] };
        out[0] = out[1] = text.Square();
        return out;
    }

    void MoveText(SafetyHookContext& context)
    {
        const auto object = reinterpret_cast<const uint8_t*>(context.esi);
        if (!(*reinterpret_cast<const uint32_t*>(object + 156) & 0x20))
            return;
        auto position = *reinterpret_cast<float**>(context.esp + 8);
        position[0] += text.OffsetX() / text.Square();
        position[1] += text.OffsetY() / text.Square();
    }
}

export void InstallScreenSpace()
{
    auto spriteA = hook::pattern("F3 0F 59 05 ? ? ? ? F3 0F 2A 4C 24 08 F3 0F 59 0D ? ? ? ? 68 00 00 80 3F 51 F3 0F 11 44 24 0C 8B 4C 24 0C F3 0F 11 0C 24 51 8D 8C 24 D4 00 00 00 E8");
    auto spriteB = hook::pattern("8D 44 24 70 50 E8 ? ? ? ? 8B 4C 24 78 8B 54 24 74 83 C4 04 68 00 00 80 3F 51 52 8D 8C 24 B4 00 00 00 E8");
    auto textA = hook::pattern("8B 8E A4 00 00 00 F3 0F 11 44 24 58 F3 0F 10 44 24 64 F3 0F 11 44 24 5C 8B 49 24 52 50 83 E1 FC E8");
    auto textB = hook::pattern("8B 86 A4 00 00 00 F3 0F 11 84 24 04 01 00 00 F3 0F 10 84 24 10 01 00 00 8D 8C 24 6C 01 00 00 51 F3 0F 11 84 24 0C 01 00 00 8B 48 24 52 83 E1 FC E8");
    auto mouse = hook::pattern("F3 0F 5E D0 F3 0F 10 44 24 10 33 C0 85 D2 F3 0F 5E C1");
    ScreenSpace::ScaleMatrix = reinterpret_cast<ScreenSpace::ScaleMatrixFn>(injector::GetBranchDestination(spriteA.get_first(0x33)).as_int()); //0x4671A0
    ScreenSpace::TextScaleHook = safetyhook::create_inline(injector::GetBranchDestination(spriteB.get_first(5)).as_int(), ScreenSpace::TextScale); //0x8F4950

    // Sprite (0x8F4C40): xmm0/xmm1 hold the 800x600 scale
    static auto spriteScaleA = safetyhook::create_mid(spriteA.get_first(0x16), [](SafetyHookContext& context)
    {
        ScreenSpace::sprite = { context.xmm0.f32[0], context.xmm1.f32[0] };
        ScreenSpace::spriteBoxed = ScreenSpace::Boxed(reinterpret_cast<const uint8_t*>(context.esi));
    });
    injector::MakeCALL(spriteA.get_first(0x33), ScreenSpace::BoxedScaleMatrix, true); //0x8F4D20

    // Sprite (0x8F6CC0): the scale comes from the text scale function, the original one is kept for the stretched case
    static auto spriteScaleB = safetyhook::create_mid(spriteB.get_first(0x15), [](SafetyHookContext& context)
    {
        ScreenSpace::sprite = ScreenSpace::text;
        ScreenSpace::spriteBoxed = ScreenSpace::Boxed(reinterpret_cast<const uint8_t*>(context.esi));
        if (!ScreenSpace::spriteBoxed)
        {
            context.edx = std::bit_cast<uint32_t>(ScreenSpace::text.x);
            context.ecx = std::bit_cast<uint32_t>(ScreenSpace::text.y);
        }
    });
    injector::MakeCALL(spriteB.get_first(0x23), ScreenSpace::BoxedScaleMatrix, true); //0x8F6EED

    static auto textMoveA = safetyhook::create_mid(textA.get_first(0x20), ScreenSpace::MoveText); //0x8FE881
    static auto textMoveB = safetyhook::create_mid(textB.get_first(0x30), ScreenSpace::MoveText); //0x8FF0B1

    // Mouse over: the cursor in the box's 800x600 space
    static auto mouseSpace = safetyhook::create_mid(mouse.get_first(), [](SafetyHookContext& context)
    {
        const ScreenSpace::Scale scale{ context.xmm0.f32[0], context.xmm1.f32[0] };
        context.xmm2.f32[0] -= scale.OffsetX();
        *reinterpret_cast<float*>(context.esp + 0x10) -= scale.OffsetY();
        context.xmm0.f32[0] = context.xmm1.f32[0] = scale.Square();
    });
}


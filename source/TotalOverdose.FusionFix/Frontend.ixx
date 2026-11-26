module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Frontend;

// Menus, the title and other screen-space 2D: an 800x600 space with a centered 4:3 layout. Banners span the screen
// horizontally while keeping their vertical alignment with the labels. Outer masks extend to the screen edges.
namespace ScreenSpace
{
    struct Scale
    {
        float x = 1.0f, y = 1.0f;
        float Square() const { return std::min(x, y); }
        float OffsetX() const { return 400.0f * (x - Square()); } // pixels
        float OffsetY() const { return 300.0f * (y - Square()); }
    };

    struct SpriteLayout
    {
        Scale scale;
        float offsetX{}, offsetY{};
    };

    SpriteLayout Layout(Scale scale, const uint8_t* object, float centerY)
    {
        const auto width = *reinterpret_cast<const float*>(object + 128);
        const auto height = *reinterpret_cast<const float*>(object + 132);
        const auto square = scale.Square();
        const auto offsetY = scale.OffsetY();
        if (width < 799.0f)
            return { { square, square }, scale.OffsetX(), offsetY };
        if (height <= 0.0f || height >= 599.0f)
            return { scale };

        const auto top = centerY - height * 0.5f;
        const auto bottom = centerY + height * 0.5f;
        // Extend the outer edge of each mask while keeping its inner edge aligned with the layout.
        if (offsetY > 0.0f && top <= 1.0f && bottom > 0.0f && bottom < 600.0f)
        {
            const auto y = (offsetY + bottom * square) / height;
            return { { scale.x, y }, 0.0f, -top * y };
        }
        if (offsetY > 0.0f && bottom >= 599.0f && top > 0.0f && top < 600.0f)
        {
            const auto innerEdge = offsetY + top * square;
            const auto y = (600.0f * scale.y - innerEdge) / height;
            return { { scale.x, y }, 0.0f, innerEdge - top * y };
        }
        return { { scale.x, square }, 0.0f, offsetY };
    }

    // Sprites: the scale matrix lays each piece out against the centered box or the whole screen.
    SpriteLayout spriteLayout;
    using ScaleMatrixFn = int(__thiscall*)(float* matrix, float x, float y, float z);
    ScaleMatrixFn ScaleMatrix = nullptr;
    int __fastcall LayoutScaleMatrix(float* matrix, void*, float, float, float z)
    {
        const int result = ScaleMatrix(matrix, spriteLayout.scale.x, spriteLayout.scale.y, z);
        matrix[12] += spriteLayout.offsetX;
        matrix[13] += spriteLayout.offsetY;
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
        const auto centerY = *reinterpret_cast<const float*>(context.esp + 0x78);
        ScreenSpace::spriteLayout = ScreenSpace::Layout({ context.xmm0.f32[0], context.xmm1.f32[0] },
            reinterpret_cast<const uint8_t*>(context.esi), centerY);
    });
    injector::MakeCALL(spriteA.get_first(0x33), ScreenSpace::LayoutScaleMatrix, true); //0x8F4D20

    // Sprite (0x8F6CC0): use the original screen scale saved by the text scale hook.
    static auto spriteScaleB = safetyhook::create_mid(spriteB.get_first(0x15), [](SafetyHookContext& context)
    {
        const auto centerY = *reinterpret_cast<const float*>(context.esp + 0x64);
        ScreenSpace::spriteLayout = ScreenSpace::Layout(ScreenSpace::text,
            reinterpret_cast<const uint8_t*>(context.esi), centerY);
    });
    injector::MakeCALL(spriteB.get_first(0x23), ScreenSpace::LayoutScaleMatrix, true); //0x8F6EED

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


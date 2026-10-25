#pragma once
#include "../Shared/Console/Viewport.hpp"

namespace BullyHud
{
    SafetyHookInline ComponentHook, TextHook, FlushHook, PointHook;
    std::array<SafetyHookMid, 4> VertexHooks;
    uintptr_t* Components = nullptr;
    uint8_t* Quads = nullptr;
    int* QuadCount = nullptr;
    int* Width = nullptr;
    int* Height = nullptr;
    const std::string* Constraint = nullptr;
    thread_local int Anchor = 0;
    thread_local int TextDepth = 0;
    thread_local float PolygonOffset = 0.0f;

    // How far side-anchored elements move in, in pixels; inside a component that is anchored to a side
    float Offset()
    {
        if (!Anchor || !Width || !Height || *Width <= 0 || *Height <= 0 || !Constraint)
            return 0.0f;
        const float aspect = console::parse_hud_aspect(Constraint->c_str());
        const float box = aspect > 0.0f ? std::min(float(*Width), *Height * aspect) : float(*Width);
        return (*Width - box) * 0.5f;
    }

    // The same in the native 640 units; elements left of the centre move right, the others left (the components'
    // own alignment doesn't say which side they sit on: the radar is left-aligned at the right edge)
    float Offset640(float x)
    {
        const float offset = Width && *Width > 0 ? Offset() * 640.0f / *Width : 0.0f;
        return x < 320.0f ? offset : -offset;
    }

    int Classify(uintptr_t component)
    {
        if (!component || !Components)
            return 0;
        for (int i = 0; i < 48; ++i)
        {
            if (Components[i] != component)
                continue;
            switch (i)
            {
                case 5: // projected entity health
                case 7: // newspaper
                case 14: // clothing
                case 16: // photography
                case 18: // barber
                case 19: // tattoo
                case 39: // monitor
                case 41: // world signs
                case 43: // screen fade
                case 44: // diary
                case 45: // inventory
                    return 0;
                case 11:
                    if (*reinterpret_cast<int*>(component + 59272) != 0)
                        return 0;
                    break;
            }
            // Centred components stay centred
            const int alignment = *reinterpret_cast<int*>(component + 12);
            return alignment == 1 ? 0 : 1;
        }
        return 0;
    }

    int __fastcall Draw(uintptr_t component, void*)
    {
        const int previous = std::exchange(Anchor, Classify(component));
        const int result = ComponentHook.thiscall<int>(component);
        Anchor = previous;
        return result;
    }

    char __cdecl Text(float x, float y, uintptr_t first, uintptr_t last, float spacing)
    {
        const float offset = TextDepth == 0 ? Offset640(x) : 0.0f;
        ++TextDepth;
        const char result = TextHook.ccall<char>(x + offset, y, first, last, spacing);
        --TextDepth;
        return result;
    }

    void __cdecl Flush()
    {
        // Queued text already carries its component's translated origin.
        const int previous = std::exchange(Anchor, 0);
        FlushHook.ccall<void>();
        Anchor = previous;
    }

    // Sprite vertices are in screen units (0 to 1, the native 1/640 scale already applied)
    float NormalizedOffset(float x)
    {
        return Offset640(x * 640.0f) / 640.0f;
    }

    // Vertices of fans and strips (the radar), one at a time in 640 units
    void* __cdecl Point(float x, float y, int depth)
    {
        return PointHook.ccall<void*>(x + Offset640(x), y, depth);
    }

    void AdjustQuad()
    {
        if (Offset() == 0.0f || !Quads || !QuadCount || *QuadCount < 0 || *QuadCount >= 500)
            return;
        auto vertices = Quads + *QuadCount * 96;
        float minimumX = FLT_MAX, maximumX = -FLT_MAX, minimumY = FLT_MAX, maximumY = -FLT_MAX;
        for (int i = 0; i < 4; ++i)
        {
            const float x = *reinterpret_cast<float*>(vertices + i * 24);
            const float y = *reinterpret_cast<float*>(vertices + i * 24 + 4);
            if (!std::isfinite(x) || !std::isfinite(y))
                return;
            minimumX = std::min(minimumX, x);
            maximumX = std::max(maximumX, x);
            minimumY = std::min(minimumY, y);
            maximumY = std::max(maximumY, y);
        }
        if (minimumX <= 0.002f && maximumX >= 0.998f && minimumY <= 0.002f && maximumY >= 0.998f)
            return;
        const float offset = NormalizedOffset((minimumX + maximumX) * 0.5f);
        for (int i = 0; i < 4; ++i)
        {
            auto x = reinterpret_cast<float*>(vertices + i * 24);
            *x += offset;
        }
    }

    void AdjustPolygon(SafetyHookContext& regs)
    {
        if (regs.ecx == 1)
        {
            PolygonOffset = 0.0f;
            if (Offset() == 0.0f)
                return;
            if (regs.edi < 3 || regs.edi > 4096)
            {
                PolygonOffset = 0.0f;
                return;
            }
            auto points = reinterpret_cast<const float*>(regs.esi);
            float minimumX = FLT_MAX, maximumX = -FLT_MAX, minimumY = FLT_MAX, maximumY = -FLT_MAX;
            for (uint32_t i = 0; i < regs.edi; ++i)
            {
                const float x = points[i * 2], y = points[i * 2 + 1];
                if (!std::isfinite(x) || !std::isfinite(y))
                {
                    PolygonOffset = 0.0f;
                    return;
                }
                minimumX = std::min(minimumX, x);
                maximumX = std::max(maximumX, x);
                minimumY = std::min(minimumY, y);
                maximumY = std::max(maximumY, y);
            }
            if (minimumX > 0.2f || maximumX < 639.8f || minimumY > 0.2f || maximumY < 479.8f)
                PolygonOffset = NormalizedOffset((minimumX + maximumX) * 0.5f / 640.0f);
        }
        if (PolygonOffset != 0.0f)
        {
            auto x = reinterpret_cast<float*>(regs.eax - 32);
            *x += PolygonOffset;
        }
    }

    void ResetHooks()
    {
        ComponentHook.reset();
        PointHook.reset();
        TextHook.reset();
        FlushHook.reset();
        for (auto& vertex : VertexHooks)
            vertex.reset();
    }

    void Install(int& width, int& height, const std::string& constraint, HMODULE module = GetModuleHandle(nullptr))
    {
        auto component = hook::module_pattern(module, "51 83 3D ? ? ? ? 07 D9 05 ? ? ? ? 56 D9 5C 24 04 8B F1 75 0C");
        auto table = hook::module_pattern(module, "89 04 B5 ? ? ? ? 83 C6 01 83 C4 04 83 FE 30 7C E8");
        auto text = hook::module_pattern(module, "66 A1 ? ? ? ? 83 EC 0C 66 3B 05 ? ? ? ? 74 13 E8");
        auto flush = hook::module_pattern(module, "55 8B EC 83 E4 F8 83 EC 48 81 3D ? ? ? ? ? ? ? ? 53 55 56 57 0F 84");
        auto pool = hook::module_pattern(module, "D9 98 ? ? ? ? 5E D9 C9 89 88 ? ? ? ? D9 98 ? ? ? ? 5B D9 98");
        auto rectangle = hook::module_pattern(module, "74 1F 8B 47 34 8B 48 24 8B 51 5C 83 05 ? ? ? ? 01 89 15");
        auto quad = hook::module_pattern(module, "74 1E 8B 40 34 8B 48 24 8B 51 5C 83 05 ? ? ? ? 01 89 15");
        auto batch = hook::module_pattern(module, "83 C6 01 89 35 ? ? ? ? 81 FE F4 01 00 00 5E 7C 08");
        auto polygon = hook::module_pattern(module, "D9 04 CE 83 C1 01 D8 0D ? ? ? ? 83 C0 18 D9 58 E0 D9 44 CE FC D8 0D");
        auto point = hook::module_pattern(module, "51 83 3D ? ? ? ? 00 74 71 D9 05 ? ? ? ? A1 ? ? ? ? D8 4C 24 08");
        if (component.size() != 1 || table.size() != 1 || text.size() != 1 || flush.size() != 1 || pool.size() != 1 ||
            rectangle.size() != 1 || quad.size() != 1 || batch.size() != 1 || polygon.size() != 1 ||
            *rectangle.get_first<int*>(13) != *quad.get_first<int*>(13) || *quad.get_first<int*>(13) != *batch.get_first<int*>(5))
        {
            OutputDebugStringA("Bully: HUD constraint patterns unavailable.\n");
            return;
        }
        Width = &width;
        Height = &height;
        Constraint = &constraint;
        Components = *table.get_first<uintptr_t*>(3);
        Quads = *pool.get_first<uint8_t*>(2);
        QuadCount = *rectangle.get_first<int*>(13);
        ComponentHook = safetyhook::create_inline(component.get_first(), Draw);
        TextHook = safetyhook::create_inline(text.get_first(), Text);
        FlushHook = safetyhook::create_inline(flush.get_first(), Flush);
        VertexHooks[0] = safetyhook::create_mid(rectangle.get_first(), [](SafetyHookContext&)
        {
            AdjustQuad();
        });
        VertexHooks[1] = safetyhook::create_mid(quad.get_first(), [](SafetyHookContext&)
        {
            AdjustQuad();
        });
        VertexHooks[2] = safetyhook::create_mid(batch.get_first(), [](SafetyHookContext&)
        {
            AdjustQuad();
        });
        VertexHooks[3] = safetyhook::create_mid(polygon.get_first(18), AdjustPolygon);
        if (point.size() == 1)
            PointHook = safetyhook::create_inline(point.get_first(), Point);
        else
            OutputDebugStringA("Bully: HUD vertex pattern unavailable; the radar keeps its place.\n");
        if (!ComponentHook || !TextHook || !FlushHook ||
            std::any_of(VertexHooks.begin(), VertexHooks.end(), [](const auto& hook)
            {
                return !hook;
            }))
        {
            ResetHooks();
            OutputDebugStringA("Bully: HUD constraint hooks unavailable.\n");
        }
    }
}

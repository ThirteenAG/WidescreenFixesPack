module;

#include <stdafx.h>
#include <cmath>
#include <numbers>
#include <string_view>

export module RestoredFeatures;

import ComVars;

namespace RestoredFeatures
{
    template<class T> T& Field(uintptr_t object, size_t offset)
    {
        return *reinterpret_cast<T*>(object + offset);
    }

    template<class Fn> Fn Virtual(void* object, size_t offset)
    {
        return reinterpret_cast<Fn>((*reinterpret_cast<uintptr_t**>(object))[offset / sizeof(uintptr_t)]);
    }

    export void InitEyeAnimations()
    {
        CIniReader ini("");
        if (!ini.ReadInteger("GRAPHICS", "RestoreEyeAnimations", 0))
            return;

        // The gaze code survives the patch. A8.dta instead replaces the moving
        // eye meshes with single triangles and paints static eyes on the face.
        // This affects 29 models: 56 eye meshes collapse to triangles, and the
        // two in Paulie's corpse model disappear. Restore complete A2 models,
        // including their other original geometry and material details.
        // Skip only those A8 models, letting rw_data open their A2 originals.
        // Keep native archive decoding, file handles, timestamps and loose mods.
        auto pattern = find_module_pattern(GetModuleHandleW(L"rw_data.dll"),
            "8B 84 2E 18 01 00 00 85 C0 0F 84 ? ? ? ? 8B 8C 2E 1C 01 00 00");
        if (pattern.size() != 1)
            return;

        static SafetyHookMid hook = safetyhook::create_mid(pattern.get_first(7), [](SafetyHookContext& regs)
        {
            auto archive = regs.ebp + regs.esi;
            // Identify the patch archive by its two native decryption keys.
            if (Field<uint32_t>(archive, 8) != 0x5324ACE5 || Field<uint32_t>(archive, 12) != 0xD8DD8FAC)
                return;
            std::string_view name(reinterpret_cast<const char*>(regs.esp + 0x1D8)); // native uppercase path
            static constexpr std::string_view models[] = {
                "MODELS\\FRANKHIGH.4DS", "MODELS\\MORELLOHIGH.4DS", "MODELS\\NORMANHIGH.4DS",
                "MODELS\\PAULIECORPSE.4DS", "MODELS\\PAULIECTHIGH.4DS", "MODELS\\PAULIEHIGH.4DS", "MODELS\\PAULIEHIGHBLOOD.4DS",
                "MODELS\\RALPHHIGH.4DS", "MODELS\\SALIERIHIGH.4DS", "MODELS\\SALIERIHIGH2.4DS",
                "MODELS\\SAMHIGH.4DS", "MODELS\\SAMHIGHBLOOD1.4DS", "MODELS\\SAMHIGHBLOOD2.4DS",
                "MODELS\\SAMHIGHBLOOD3.4DS", "MODELS\\SAMHIGHBLOOD4.4DS", "MODELS\\SARAH2.4DS",
                "MODELS\\SARAH2HIGH.4DS", "MODELS\\SARAH2HIGHNAHA.4DS", "MODELS\\TOMMYDELNIKHIGH.4DS",
                "MODELS\\TOMMYHIGH.4DS", "MODELS\\TOMMYHIGHBLOOD.4DS", "MODELS\\TOMMYHIGHCOATHAT.4DS",
                "MODELS\\TOMMYHIGHHAT.4DS", "MODELS\\TOMMYNAHAC.4DS", "MODELS\\TOMMYOLD.4DS",
                "MODELS\\TOMMYOLDBLOOD.4DS", "MODELS\\TOMMYRUKAV.4DS", "MODELS\\TOMMYTAXIDRIVERHIGH.4DS",
                "MODELS\\VINCENZOHIGH.4DS"
            };
            if (std::find(std::begin(models), std::end(models), name) == std::end(models))
                return;

            // Fall back to the patched model if its original archive entry is
            // missing. The table's hash and final 16 path bytes are rw_data's
            // own lookup keys. Only search archives after the current one.
            uint32_t count = Field<uint32_t>(regs.esp, 0x28);
            uint32_t hash = Field<uint32_t>(regs.esp, 0x24);
            auto suffix = name.substr(name.size() > 16 ? name.size() - 16 : 0);
            for (uint32_t i = regs.edi + 1; i < count; ++i)
            {
                auto original = regs.ebp + i * 296;
                if (Field<uint32_t>(original, 8) != 0x2D5085D4 || Field<uint32_t>(original, 12) != 0x82A1C97B)
                    continue;
                auto begin = Field<uintptr_t>(original, 280);
                auto end = Field<uintptr_t>(original, 284);
                for (auto entry = begin; entry && entry < end; entry += 28)
                {
                    if (Field<uint32_t>(entry, 0) == hash &&
                        std::string_view(reinterpret_cast<const char*>(entry + 12), suffix.size()) == suffix)
                    {
                        regs.eax = 0; // native empty-table branch advances to the next archive
                        return;
                    }
                }
            }
        });
    }

    export void InitProjectorOpacity()
    {
        CIniReader ini("");
        if (!ini.ReadInteger("GRAPHICS", "RestoreCarScratches", 0))
            return;

        // 1.1/1.2 omit the normalized-alpha -> byte conversion in BOTH default
        // projector paths (perspective and orthographic). 1.0 has no such sequence.
        auto pattern = find_module_pattern<2>(GetLS3DF(),
            "8B 85 E8 01 00 00 89 44 24 10 EB 08 C7 44 24 10 00 00 7F 43 D9 44 24 10 DB 5C 24 10 D9 85 EC 01 00 00");
        if (pattern.size() != 2)
            return;

        static std::vector<SafetyHookMid> hooks;
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            hooks.emplace_back(safetyhook::create_mid(pattern.get(i).get<void>(20), [](SafetyHookContext& regs)
            {
                // Mode 1 is used by the game's scratch projectors. Leave other
                // projector modes, including the later patch's new modes, alone.
                if (Field<uint32_t>(regs.ebp, 516) == 1)
                    Field<float>(regs.esp, 0x10) = std::clamp(Field<float>(regs.ebp, 488), 0.0f, 1.0f) * 255.0f;
            }));
        }
    }

    struct RadarLayout
    {
        size_t texture, scale, matrix, origin, dimensions, center;
    };
    static RadarLayout radar;
    static void** graphics;

    struct RadarVertex
    {
        float x, y, z, rhw;
        uint32_t color, specular;
        float u, v;
    };
    static_assert(sizeof(RadarVertex) == 32);

    void DrawRadarMask(SafetyHookContext& regs)
    {
        auto vertices = Field<const RadarVertex*>(regs.esp, 12);
        float width = vertices[1].x - vertices[0].x;
        float height = vertices[2].y - vertices[0].y;
        if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f) return;
        float x = vertices[0].x + width * 0.5f, y = vertices[0].y + height * 0.5f;
        constexpr size_t segments = 128;
        static const auto circle = []()
        {
            std::array<std::pair<float, float>, segments> result;
            for (size_t i = 0; i < segments; ++i)
            {
                float angle = static_cast<float>(i * 2.0 * std::numbers::pi / segments);
                result[i] = { std::cos(angle), std::sin(angle) };
            }
            return result;
        }();
        static std::array<RadarVertex, segments * 6> mask;
        for (size_t i = 0; i < segments; ++i)
        {
            auto inner = [&](size_t index)
            {
                auto vertex = vertices[0];
                auto [dx, dy] = circle[index % segments];
                // radarclp.bmp has a roughly 29-pixel radius in a 64x64 mask.
                // Keep the opening inside the original rim, but remove its
                // magnified texel steps by writing depth with smooth geometry.
                vertex.x = x + dx * width * 0.45f;
                vertex.y = y + dy * height * 0.45f;
                vertex.u = vertex.v = 0.0f;
                return vertex;
            };
            auto outer = [&](size_t index)
            {
                auto vertex = inner(index);
                auto [dx, dy] = circle[index % segments];
                float scale = 0.5f / std::max(std::abs(dx), std::abs(dy));
                vertex.x = x + dx * width * scale;
                vertex.y = y + dy * height * scale;
                return vertex;
            };
            auto a = inner(i), b = outer(i), c = inner(i + 1), d = outer(i + 1);
            std::array triangles{a, b, c, b, d, c};
            std::copy(triangles.begin(), triangles.end(), mask.begin() + i * 6);
        }
        auto graph = Field<void*>(regs.esp, 0);
        using SetTexture = void(__stdcall*)(void*, void*);
        Virtual<SetTexture>(graph, 12)(graph, nullptr);
        Field<int>(regs.esp, 4) = 3; // triangle list
        Field<int>(regs.esp, 8) = static_cast<int>(segments * 2);
        Field<const RadarVertex*>(regs.esp, 12) = mask.data();
    }

    // Clip the map at its geographical bounds, leaving the game's original radar
    // background visible beyond the city instead of wrapping the map texture.
    size_t ClipMap(std::array<RadarVertex, 12>& polygon, size_t count, bool vertical, float edge, bool lower)
    {
        auto input = polygon;
        size_t output = 0;
        auto coordinate = [vertical](const RadarVertex& vertex) { return vertical ? vertex.v : vertex.u; };
        auto previous = input[count - 1];
        float previousDistance = (coordinate(previous) - edge) * (lower ? 1.0f : -1.0f);
        for (size_t i = 0; i < count; ++i)
        {
            const auto& current = input[i];
            float distance = (coordinate(current) - edge) * (lower ? 1.0f : -1.0f);
            if ((previousDistance >= 0.0f) != (distance >= 0.0f))
            {
                float t = previousDistance / (previousDistance - distance);
                auto intersection = previous;
                intersection.x += (current.x - previous.x) * t;
                intersection.y += (current.y - previous.y) * t;
                intersection.u += (current.u - previous.u) * t;
                intersection.v += (current.v - previous.v) * t;
                if (vertical)
                    intersection.v = edge;
                else
                    intersection.u = edge;
                polygon[output++] = intersection;
            }
            if (distance >= 0.0f)
                polygon[output++] = current;
            previous = current;
            previousDistance = distance;
        }
        return output;
    }

    void DrawRadarMap(SafetyHookContext& regs)
    {
        auto igraph = *graphics;
        if (!igraph)
            return;
        float scale = Field<float>(regs.ebp, radar.scale);
        if (!std::isfinite(scale) || scale <= 0.00001f)
            return;

        auto& texture = Field<void*>(regs.ebp, radar.texture);
        if (!texture)
        {
            // Reuse the full-map slot: the HUD already releases it on shutdown.
            // Throttle failures (e.g. a mission without this asset).
            static DWORD lastFailure;
            DWORD now = GetTickCount();
            if (lastFailure && now - lastFailure < 1000)
                return;
            using OpenTexture = int(__stdcall*)(void*, const char*, const char*, uint32_t, void**);
            if (Virtual<OpenTexture>(igraph, 4)(igraph, "0mapar.bmp", nullptr, 0, &texture) != 0 || !texture)
            {
                lastFailure = now;
                return;
            }
            lastFailure = 0;
        }

        const auto matrix = reinterpret_cast<const float*>(regs.esp + radar.matrix);
        const auto origin = reinterpret_cast<const float*>(regs.esp + radar.origin);
        const auto dimensions = reinterpret_cast<const float*>(regs.esp + radar.dimensions);
        const auto center = reinterpret_cast<const float*>(regs.esp + radar.center);
        std::array<RadarVertex, 12> polygon;
        constexpr std::array<std::pair<float, float>, 4> corners = { {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}} };
        for (size_t i = 0; i < corners.size(); ++i)
        {
            auto& vertex = polygon[i];
            vertex = { origin[0] + corners[i].first * dimensions[0], origin[1] + corners[i].second * dimensions[1],
                0.4f, 1.0f, 0xFFFFFFFF, 0, 0, 0 };
            // Invert the existing radar's orthonormal world-to-radar matrix. This
            // keeps the imagery aligned with native blips, camera yaw and zoom.
            float x = (vertex.x - center[0]) / scale - matrix[12];
            float y = -matrix[13];
            // The native center is an X/Y/Z vector: screen Y occupies Z.
            float z = (center[2] - vertex.y) / scale - matrix[14];
            float worldX = x * matrix[0] + y * matrix[1] + z * matrix[2];
            float worldZ = x * matrix[8] + y * matrix[9] + z * matrix[10];
            // Xbox HUD 0x142900: world coordinates -> 0mapar.bmp UV coordinates.
            vertex.u = worldX * 0.00022986f + 0.73464239f;
            vertex.v = worldZ * -0.00050728f + 0.47024599f;
            if (!std::isfinite(vertex.u) || !std::isfinite(vertex.v))
                return;
        }
        size_t count = 4;
        for (auto [vertical, edge, lower] : { std::tuple{false, 0.0f, true}, {false, 1.0f, false},
            {true, 0.0f, true}, {true, 1.0f, false} })
        {
            count = ClipMap(polygon, count, vertical, edge, lower);
            if (count < 3)
                return;
        }
        using SetTexture = void(__stdcall*)(void*, void*);
        using Draw = void(__stdcall*)(void*, int, int, const RadarVertex*, uint32_t);
        Virtual<SetTexture>(igraph, 12)(igraph, texture);
        // IGraph triangle fan, within the native radar viewport/depth mask.
        // Draw in front of the radar background (0.5), behind the native blips
        // (0.2), and beyond the mask (0.0), with the native LEQUAL depth test.
        // The full-map HUD slot owns the texture and releases it on shutdown.
        Virtual<Draw>(igraph, 88)(igraph, 4, static_cast<int>(count - 2), polygon.data(), 1);
        Virtual<SetTexture>(igraph, 12)(igraph, nullptr);
    }

    export void InitRadar()
    {
        CIniReader ini("");
        if (!ini.ReadInteger("HUD", "RadarMap", 0))
            return;

        // Same HUD draw sequence in all three PC versions, with different layout
        // and stack frame sizes in 1.0. Match the native radar's car-list fields.
        auto old = find_pattern("A1 ? ? ? ? 6A 01 6A 0C 50 8B 10 FF 52 44 8B 9D 94 94 00 00 8B 85 98 94 00 00");
        auto newer = find_pattern("A1 ? ? ? ? 6A 01 6A 0C 50 8B 10 FF 52 44 8B 9D 9C 44 00 00 8B 85 A0 44 00 00");
        if (old.size() + newer.size() != 1)
            return;
        void* target;
        if (old.size() == 1)
        {
            radar = { 37008, 38072, 0xAF4, 0x18, 0x4C, 0x64 };
            graphics = *old.get_first<void**>(1);
            target = old.get_first(15);
        }
        else
        {
            radar = { 16528, 17600, 0xB04, 0x20, 0x6C, 0x74 };
            graphics = *newer.get_first<void**>(1);
            target = newer.get_first(15);
        }
        static SafetyHookMid hook = safetyhook::create_mid(target, DrawRadarMap);
        auto pMask = find_pattern(
            "8B 10 51 50 FF 52 0C A1 ? ? ? ? 8D 8C 24 8C 00 00 00 6A 01 51 8B 10 6A 02 6A 05 50 FF 52 58 D9 44 24 ? D8 05",
            "8B 10 51 50 FF 52 0C A1 ? ? ? ? 8D 4C 24 7C 6A 01 51 8B 10 6A 02 6A 05 50 FF 52 58 D9 44 24 ? D8 05");
        if (pMask.size() == 1)
        {
            static auto RadarMaskHook = safetyhook::create_mid(pMask.get_first(old.size() == 1 ? 26 : 29), DrawRadarMask);
        }
    }
}

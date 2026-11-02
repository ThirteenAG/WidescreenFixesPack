#include "Game.hpp"
#include <cmath>
#include <cstring>

namespace tcny
{
    namespace
    {
        SafetyMipsMid widthHook, subtitleHook, modelHook, mapHook, blipHook, radarDiscHook, scaleHook;
        safetymips::GameInline<uint64_t(float*, float*, uint32_t, float*)> layoutHook;
        safetymips::GameInline<uint64_t(void*, uint64_t)> radarHook;

        // HUD groups are recognised by their frame (x, y, width, height, scale x,
        // scale y as fractions of the 4:3 screen). Groups at the sides of the 4:3
        // screen move to the sides of the HUD box.
        struct Identity
        {
            uint32_t bits[6];
        };
        // Health silhouette and bars, badge, weapon icon and ammunition.
        constexpr Identity rightElements[] = {
            {{0x3F579998, 0x3F340001, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}},
            {{0x3F65FFFE, 0x3F3C0001, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}},
            {{0x3F473332, 0x3F3F6DB7, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}},
            {{0x3EE3FFFE, 0x3CB6DB6E, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}},
            {{0x3F3DFFFE, 0x3D8DB6DC, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}},
            {{0x3F69FFFE, 0x3D8DB6DC, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}}};
        // Phone.
        constexpr Identity leftElements[] = {
            {{0xBDCCCCCC, 0x3D924925, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}}};

        bool Matches(const Identity& lhs, const Identity& rhs)
        {
            float x, y;
            std::memcpy(&x, lhs.bits, sizeof(x));
            std::memcpy(&y, rhs.bits, sizeof(y));
            if (std::fabs(x - y) >= 0.001f)
            {
                return false;
            }
            return std::memcmp(lhs.bits + 1, rhs.bits + 1, sizeof(lhs.bits) - sizeof(uint32_t)) == 0;
        }
        template <size_t Count> bool Any(const Identity (&list)[Count], const Identity& identity)
        {
            for (const auto& entry : list)
            {
                if (Matches(identity, entry))
                {
                    return true;
                }
            }
            return false;
        }

#ifdef WFP_PS2_DEBUG
        // PINE-readable: magic, selected entry, frame, count; then per frame,
        // alignment and caller chain {bits[6], alignment, output x, output width,
        // last frame}. The selected entry is moved right to find it on screen.
        uint32_t tracePage;
        uint32_t hudTrace[4 + 256 * 10] = {0x44555448, uint32_t(-1)};
        // Per entry: game return addresses found on the stack.
        uint32_t hudCallers[4 + 256 * 8] = {0x534C4143};
        __attribute__((noinline)) void Callers(uint32_t* out)
        {
            volatile uint32_t marker = 0;
            const uint32_t* stack = const_cast<const uint32_t*>(&marker);
            unsigned found = 0;
            for (unsigned i = 0; i < 1024 && found < 6; ++i)
            {
                const uint32_t v = stack[i];
                if (v < 0x100008 || v >= 0x4F0000 || (v & 3))
                {
                    continue;
                }
                const uint32_t call = *reinterpret_cast<const uint32_t*>(v - 8);
                if ((call >> 26) == 3 || ((call >> 26) == 0 && (call & 0x3F) == 9))
                {
                    out[found++] = v;
                }
            }
        }
        void Trace(const Identity& identity, uint32_t alignment, float* output, const float* base, const float* child)
        {
            hudTrace[2] = frame;
            uint32_t chain[8] = {};
            Callers(chain);
            chain[6] = tracePage;
            chain[7] = uint32_t(uintptr_t(child));
            uint32_t* entries = hudTrace + 4;
            unsigned i = 0;
            for (; i < hudTrace[3]; ++i)
            {
                uint32_t* e = entries + i * 10;
                if (e[6] == alignment && !std::memcmp(e, identity.bits, sizeof(identity.bits)) &&
                    !std::memcmp(hudCallers + 4 + i * 8, chain, 4 * sizeof(uint32_t)))
                {
                    break;
                }
            }
            if (i == hudTrace[3])
            {
                if (i == 256)
                {
                    return;
                }
                ++hudTrace[3];
                std::memcpy(entries + i * 10, identity.bits, sizeof(identity.bits));
                entries[i * 10 + 6] = alignment;
                std::memcpy(hudCallers + 4 + i * 8, chain, sizeof(chain));
            }
            uint32_t* e = entries + i * 10;
            e[7] = Bits(output[1]);
            e[8] = Bits(output[3]);
            e[9] = frame;
            if (i == hudTrace[1])
            {
                output[1] += 0.3f;
            }
        }
#endif

        // The HUD manager (the 2D drawing object) and the gameplay HUD page. Each
        // widget keeps its page and the manager just before its own rectangle.
        uintptr_t hudPage;
        uintptr_t PageOf(const float* child)
        {
            const auto rectangle = uintptr_t(child);
            for (uintptr_t offset = 4; offset <= 0x40; offset += 4)
            {
                if (Read<uint32_t>(rectangle - offset) == hudManager)
                {
                    return Read<uintptr_t>(rectangle - offset - 8);
                }
            }
            return 0;
        }
        enum Side
        {
            Center,
            Left,
            Right
        };
        Side Classify(const Identity& identity, const float* child, uint32_t alignment, const float* output)
        {
            if (Any(rightElements, identity))
            {
                if (const auto page = PageOf(child))
                {
                    hudPage = page;
                }
                return Right;
            }
            if (Any(leftElements, identity))
            {
                return Left;
            }
            // Elements placed straight on the screen (help boxes, counters) by
            // their 4:3 position: wholly in the left or right part of the screen.
            float x, width;
            std::memcpy(&x, identity.bits, sizeof(x));
            std::memcpy(&width, identity.bits + 2, sizeof(width));
            const auto page = PageOf(child);
            // Widgets point at their page object or at a part of it.
            if (x != 0 || width != 1.0f || alignment % 3 == 1 || !hudPage || page + 0x80 < hudPage || page > hudPage + 0x80)
            {
                return Center;
            }
            if (output[1] + output[3] <= 0.4f)
            {
                return Left;
            }
            return output[1] >= 0.6f ? Right : Center;
        }
        uint64_t Layout(float* base, float* child, uint32_t alignment, float* output)
        {
            Identity identity{};
            if (base)
            {
                std::memcpy(identity.bits, base + 1, sizeof(identity.bits));
            }
            auto result = layoutHook.call(base, child, alignment, output);
            if (base && child && output && alignment < 9)
            {
                float offset = display.normalized;
                // Elements kept outside the 4:3 screen (hidden) stay outside the real screen.
                const bool hidden = output[1] + output[3] <= 0 || output[1] >= 1;
                switch (Classify(identity, child, alignment, output))
                {
                    case Right:
                        offset += hidden ? display.normalized : display.right;
                        break;
                    case Left:
                        offset -= hidden ? display.normalized : display.left;
                        break;
                    default:
                        break;
                }
                output[1] += offset;
#ifdef WFP_PS2_DEBUG
                tracePage = PageOf(child);
                Trace(identity, alignment, output, base, child);
#endif
            }
            return result;
        }
        uint64_t Radar(void* object, uint64_t argument)
        {
            auto& x = *reinterpret_cast<float*>(static_cast<uint8_t*>(object) + 36);
            const float original = x;
            x += display.radar;
            const auto result = radarHook.call(object, argument);
            x = original;
            return result;
        }
        void Width(SafetyMipsContext& regs)
        {
            regs.a1 = uint32_t(display.width * display.hudScale + 0.5f);
        }
        void Scale3D(SafetyMipsContext& regs)
        {
            regs.at = Bits(0.0015625f * display.scale);
        }
        void Subtitle(SafetyMipsContext& regs)
        {
            // Replace only MTC1 and the following LUI, preserving both their outputs.
            uint32_t bits = uint32_t(regs.at);
            float width;
            std::memcpy(&width, &bits, sizeof(width));
            regs.f4 = width / display.hudScale;
            regs.at = 0x3ACC0000;
        }
        void Model(SafetyMipsContext& regs)
        {
            regs.a1 |= 0x2615;
            regs.f0 = regs.f0 * regs.f1 + display.centered;
        }
        void Blips(SafetyMipsContext& regs)
        {
            // A1 contains a 128-bit vector, not a C pointer. Preserve the native
            // transform and choose the map-specific origin from its actual caller.
            const bool menu = uintptr_t(regs.ra) == mapCallers[0] || uintptr_t(regs.ra) == mapCallers[1];
            const float origin = 195.0f + (menu ? (640.0f / display.hudScale - 640.0f) * 0.5f : 0.0f);
            regs.at = Bits(origin);
            regs.f1 = origin;
        }
    } // namespace

    void InstallHud(uintptr_t width, uintptr_t layout, uintptr_t subtitles, uintptr_t models, uintptr_t map,
                    uintptr_t blips, uintptr_t radar, uintptr_t radarDisc, uintptr_t scale3D)
    {
        widthHook = safetymips::create_mid<&Width>(width);
        safetymips::Options layoutOptions;
        layoutOptions.instructions = 3;
        layoutHook = safetymips::create_inline_game(layout, Layout, layoutOptions);
        radarHook = safetymips::create_inline_game(radar, Radar);
        safetymips::Options replace;
        replace.execute_original = false;
        scaleHook = safetymips::create_mid<&Scale3D>(scale3D, nullptr, replace);
        subtitleHook = safetymips::create_mid<&Subtitle>(subtitles, nullptr, replace);
        modelHook = safetymips::create_mid<&Model>(models, nullptr, replace);
        // Keep CVT.W.S in the second slot. Transform before the original ADD.S.
        mapHook = safetymips::create_mid(map, [](SafetyMipsContext& regs) { regs.f0 += display.centered; });
        blipHook = safetymips::create_mid<&Blips>(blips, nullptr, replace);
        radarDiscHook = safetymips::create_mid(radarDisc, [](SafetyMipsContext& regs) { regs.f1 += display.radar; });
    }
} // namespace tcny

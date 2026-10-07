#include "Game.hpp"
#include <array>
#include <cmath>
#include <cstring>

namespace tcny {
namespace {
SafetyMipsMid widthHook, subtitleHook, modelHook, mapHook, blipHook, radarDiscHook, scaleHook;
safetymips::GameInline<uint64_t(float*, float*, uint32_t, float*)> layoutHook;
safetymips::GameInline<uint64_t(void*, uint64_t)> radarHook;
struct Identity { uint32_t bits[6]; };
inline constexpr Identity rightElements[] = {
    {{0x3F579998, 0x3F340001, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}},
    {{0x3F65FFFE, 0x3F3C0001, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}},
    {{0x3F473332, 0x3F3F6DB7, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}},
    {{0x3EE3FFFE, 0x3CB6DB6E, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}},
    {{0x3F3DFFFE, 0x3D8DB6DC, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}},
    {{0x3F69FFFE, 0x3D8DB6DC, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}}
};
inline constexpr Identity phoneElement{{0xBDCCCCCC, 0x3D924925, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000}};
bool Matches(const Identity& lhs, const Identity& rhs) {
    float x, y;
    std::memcpy(&x, lhs.bits, sizeof(x)); std::memcpy(&y, rhs.bits, sizeof(y));
    if (std::fabs(x - y) >= 0.001f) return false;
    return std::memcmp(lhs.bits + 1, rhs.bits + 1, sizeof(lhs.bits) - sizeof(uint32_t)) == 0;
}
uint64_t Layout(float* base, float* child, uint32_t alignment, float* output) {
    Identity identity{};
    if (base) std::memcpy(identity.bits, base + 1, sizeof(identity.bits));
    auto result = layoutHook.call(base, child, alignment, output);
    if (base && child && output && alignment < 9) {
        float offset = display.normalized;
        for (const auto& entry : rightElements) if (Matches(identity, entry)) { offset += display.right; break; }
        if (Matches(identity, phoneElement)) offset -= display.phone;
        output[1] += offset;
    }
    return result;
}
uint64_t Radar(void* object, uint64_t argument) {
    auto& x = *reinterpret_cast<float*>(static_cast<uint8_t*>(object) + 36);
    const float original = x;
    x += display.radar;
    const auto result = radarHook.call(object, argument);
    x = original;
    return result;
}
uint32_t Bits(float value) { uint32_t bits; std::memcpy(&bits, &value, sizeof(bits)); return bits; }
void Width(SafetyMipsContext& regs) { regs.a1 = uint32_t(display.width * display.scale + 0.5f); }
void Scale3D(SafetyMipsContext& regs) { regs.at = Bits(0.0015625f * display.scale); }
void Subtitle(SafetyMipsContext& regs) {
    // Replace only MTC1 and the following LUI, preserving both their outputs.
    uint32_t bits = uint32_t(regs.at);
    float width; std::memcpy(&width, &bits, sizeof(width));
    regs.f4 = width / display.scale;
    regs.at = 0x3ACC0000;
}
void Model(SafetyMipsContext& regs) {
    regs.a1 |= 0x2615;
    regs.f0 = regs.f0 * regs.f1 + display.centered;
}
void Blips(SafetyMipsContext& regs) {
    // A1 contains a 128-bit vector, not a C pointer. Preserve the native
    // transform and choose the map-specific origin from its actual caller.
    const bool menu = uintptr_t(regs.ra) == mapCallers[0] || uintptr_t(regs.ra) == mapCallers[1];
    const float origin = 195.0f + (menu ? (640.0f / display.scale - 640.0f) * 0.5f : 0.0f);
    regs.at = Bits(origin); regs.f1 = origin;
}
}
void InstallHud(uintptr_t width, uintptr_t layout, uintptr_t subtitles, uintptr_t models,
                uintptr_t map, uintptr_t blips, uintptr_t radar, uintptr_t radarDisc, uintptr_t scale3D) {
    widthHook = safetymips::create_mid<&Width>(width);
    safetymips::Options layoutOptions; layoutOptions.instructions = 3;
    layoutHook = safetymips::create_inline_game(layout, Layout, layoutOptions);
    radarHook = safetymips::create_inline_game(radar, Radar);
    safetymips::Options replace; replace.execute_original = false;
    scaleHook = safetymips::create_mid<&Scale3D>(scale3D, nullptr, replace);
    subtitleHook = safetymips::create_mid<&Subtitle>(subtitles, nullptr, replace);
    modelHook = safetymips::create_mid<&Model>(models, nullptr, replace);
    // Keep CVT.W.S in the second slot. Transform before the original ADD.S.
    mapHook = safetymips::create_mid(map, [](SafetyMipsContext& regs) { regs.f0 += display.centered; });
    blipHook = safetymips::create_mid<&Blips>(blips, nullptr, replace);
    radarDiscHook = safetymips::create_mid(radarDisc, [](SafetyMipsContext& regs) { regs.f1 += display.radar; });
}
}

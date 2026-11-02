#include "Game.hpp"

namespace vcs {
namespace {
// Each phase uses the original game routine, with one allocation-free scope.
// Buffered font vertices are transformed when appended, so later buffer flushes
// do not need to infer which HUD element originally produced a glyph.
template<uintptr_t Address, DrawMode Mode, class Signature> struct HudPhase;
template<uintptr_t Address, DrawMode Mode, class Return, class... Args>
struct HudPhase<Address, Mode, Return(Args...)> {
    inline static safetymips::GameInline<Return(Args...)> hook;
    static Return Draw(Args... args) {
        DrawScope scope(Mode);
        return hook.call(args...);
    }
    static void Install() { hook = safetymips::create_inline_game(Address, Draw); }
};
template<uintptr_t Address, DrawMode Mode, class Signature = uint64_t(void*)>
void Phase() { HudPhase<Address, Mode, Signature>::Install(); }
}
void InstallHud() {
    Phase<0x21F348, DrawMode::Center, uint64_t()>(); // Render2DStuff: scripts, empire UI, messages
    Phase<0x21F900, DrawMode::Center, void()>(); // Text rendered after the fade
    Phase<0x272E78, DrawMode::Center, uint64_t()>(); // Credits
    Phase<0x31C628, DrawMode::Center>(); // CHud::Draw
    Phase<0x31CA80, DrawMode::Center>(); // CHud::DrawAfterFade
    Phase<0x31DB20, DrawMode::RightTop, uint64_t(void*, int16_t)>(); // Health value is passed in a1
    Phase<0x31E168, DrawMode::RightTop>(); // Stamina
    Phase<0x31E3F8, DrawMode::RightTop>(); // Armour
    Phase<0x31EB18, DrawMode::RightTop>(); // Cash
    Phase<0x31ED10, DrawMode::RightTop>(); // Time
    Phase<0x31F478, DrawMode::RightTop>(); // Weapon and ammunition
    Phase<0x31FDF0, DrawMode::RightTop>(); // Wanted level
    Phase<0x31FFF0, DrawMode::RightTop, uint64_t()>(); // Media level
    Phase<0x320338, DrawMode::RightBottom>(); // Zone name
    Phase<0x320710, DrawMode::RightBottom>(); // Vehicle name
    Phase<0x3209D0, DrawMode::RightTop>(); // Counter labels
    Phase<0x320C08, DrawMode::RightTop>(); // Mission clock
    Phase<0x320F28, DrawMode::RightTop>(); // Mission counters and bars
    Phase<0x321500, DrawMode::LeftBottom>(); // Radar map, disc, mask and blips
    Phase<0x323018, DrawMode::Center>(); // Screen-space crosshairs
    Phase<0x113D50, DrawMode::World>(); // Keep projected world marker centers
    Phase<0x1E10E8, DrawMode::World, uint64_t()>(); // Pickup labels stay at their projected position
    Phase<0x21E170, DrawMode::Center, void(const char*, const char*, const char*, bool)>(); // Loading artwork and text
}
}

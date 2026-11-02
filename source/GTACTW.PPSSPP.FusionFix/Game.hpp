#pragma once
#include "Addresses.hpp"
#include "../Shared/Console/PSP.hpp"
namespace ctw {
using namespace console::portable;
// CFG writers pad values and the Hub can use textual booleans. Parse the
// complete trimmed token instead of the SDK reader's untrimmed bool prefix.
inline bool Option(const char* key, bool fallback) {
    char buffer[16]{};
    const char* text = inireader.ReadString("MAIN", key, fallback ? "1" : "0", buffer, sizeof(buffer));
    while (*text == ' ' || *text == '\t') ++text;
    char token[6]{};
    unsigned length = 0;
    while (*text && *text != ' ' && *text != '\t' && *text != '\r' && *text != '\n') {
        if (length == sizeof(token) - 1) return fallback;
        const char value = *text++;
        token[length++] = value >= 'A' && value <= 'Z' ? char(value + ('a' - 'A')) : value;
    }
    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') ++text;
    if (*text) return fallback;
    if (!std::strcmp(token, "1") || !std::strcmp(token, "true") || !std::strcmp(token, "yes") ||
        !std::strcmp(token, "on") || !std::strcmp(token, "y")) return true;
    if (!std::strcmp(token, "0") || !std::strcmp(token, "false") || !std::strcmp(token, "no") ||
        !std::strcmp(token, "off") || !std::strcmp(token, "n")) return false;
    return fallback;
}
inline constexpr float nativeAspect = 480.0f / 272.0f;
inline float targetAspect = nativeAspect;
inline bool automaticAspect = true;
// MAIN.HudAspectRatioConstraint: 0 = the HUD uses the whole screen width.
inline float hudAspect = 0.0f;
// MAIN.FOVFactor: multiplies the tangent of the gameplay view angle.
inline float fovFactor = 1.0f;
// MAIN.CutsceneBorders: story scenes keep the native 16:9 frame, bit 0 with
// bars above and below on narrower screens, bit 1 with side bars on wider ones.
inline int cutsceneBorders = 3;
inline float HorizontalScale() { return nativeAspect / targetAspect; }
// 2D layout fitted to the screen: wider screens narrow the native layout,
// narrower screens shrink it vertically, so it always keeps its proportions.
inline float FitX() { const float h = HorizontalScale(); return h < 1.0f ? h : 1.0f; }
inline float FitY() { const float h = HorizontalScale(); return h > 1.0f ? 1.0f / h : 1.0f; }
// Where a screen-edge anchor lands, as a fraction of the half screen width.
inline float HudBox() { return console::hud_edge(2.0f, 2.0f, targetAspect, hudAspect) - 1.0f; }
void RefreshAspect();
template<class T> inline T& at(uintptr_t address) { return *reinterpret_cast<T*>(address); }
inline bool Guest(uintptr_t address, unsigned size = 4) {
    return address >= 0x08000000 && address <= 0x0DD00000 - size;
}
inline uintptr_t Player() {
    const unsigned index = at<unsigned>(Address<0x08B5B238>());
    if (index >= 2) return 0;
    const auto player = at<uintptr_t>(Address<0x08C0D160>() + index * 4);
    return Guest(player, 0x1000) ? player : 0;
}
inline uintptr_t Vehicle() {
    const auto player = Player();
    if (!player) return 0;
    const auto handle = at<uintptr_t>(player + 0x1CC);
    return Guest(handle) ? at<uintptr_t>(handle) : 0;
}
// The PDA's running application is 16 while the gameplay HUD is shown; the
// PDA, apartment, menus and minigame screens use other applications.
inline bool GameplayApp() {
    const auto app = at<uintptr_t>(Address<0x08C11258>());
    return Guest(app, 200) && at<int>(app + 196) == 16;
}
// Story scenes: comic panels and in-engine shots between the game's own
// cinematic bars.
inline bool CutsceneApp() {
    const auto app = at<uintptr_t>(Address<0x08C11258>());
    return Guest(app, 200) && at<int>(app + 196) == 54;
}
inline bool GameplayCamera() {
    const auto player = Player();
    return player && Vehicle() && !at<uint8_t>(player + 0xE31);
}
void ClearOutside(unsigned left, unsigned bottom);
void InstallInput();
void InstallCamera();
void InstallAspect();
void InstallReplay();
bool InstallHud();
void InstallVideo();
}

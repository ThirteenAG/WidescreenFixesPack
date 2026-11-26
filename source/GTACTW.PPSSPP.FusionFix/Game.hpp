#pragma once
#include "Addresses.hpp"
#include "Shared/Console/PSP.hpp"
namespace ctw
{
    using namespace console::portable;
    inline constexpr auto cfgPath = "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/GTACTW.PPSSPP.FusionFix.cfg";
    bool SettingText(const char* section, const char* key, char* value, size_t capacity);
    float SettingFloat(const char* section, const char* key, float fallback);
    int SettingInteger(const char* section, const char* key, int fallback);
    bool Option(const char* key, bool fallback);
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
    // MAIN.ModernControls and MAIN.UnthrottleEmuDuringLoading, read again when the
    // cfg changes.
    inline bool modernControls = true;
    inline bool fastLoading = true;
    inline float HorizontalScale()
    {
        return nativeAspect / targetAspect;
    }
    // 2D layout fitted to the screen: wider screens narrow the native layout,
    // narrower screens shrink it vertically, so it always keeps its proportions.
    inline float FitX()
    {
        const float h = HorizontalScale();
        return h < 1.0f ? h : 1.0f;
    }
    inline float FitY()
    {
        const float h = HorizontalScale();
        return h > 1.0f ? 1.0f / h : 1.0f;
    }
    // Where a screen-edge anchor lands, as a fraction of the half screen width.
    inline float HudBox()
    {
        return console::hud_edge(2.0f, 2.0f, targetAspect, hudAspect) - 1.0f;
    }
    void RefreshAspect();
    template <class T>
    inline T& at(uintptr_t address)
    {
        return *reinterpret_cast<T*>(address);
    }
    inline bool Guest(uintptr_t address, unsigned size = 4)
    {
        return address >= 0x08000000 && address <= 0x0DD00000 - size;
    }
    inline uintptr_t Player()
    {
        const unsigned index = at<unsigned>(Address<0x08B5B238>());
        if (index >= 2)
            return 0;
        const auto player = at<uintptr_t>(Address<0x08C0D160>() + index * 4);
        return Guest(player, 0x1000) ? player : 0;
    }
    inline uintptr_t Vehicle()
    {
        const auto player = Player();
        if (!player)
            return 0;
        const auto handle = at<uintptr_t>(player + 0x1CC);
        return Guest(handle) ? at<uintptr_t>(handle) : 0;
    }
    // The PDA's running application is 16 while the gameplay HUD is shown; the
    // PDA, apartment, menus and minigame screens use other applications.
    inline bool GameplayApp()
    {
        const auto app = at<uintptr_t>(Address<0x08C11258>());
        return Guest(app, 200) && at<int>(app + 196) == 16;
    }
    // Story scenes: comic panels and in-engine shots between the game's own
    // cinematic bars.
    inline bool CutsceneApp()
    {
        const auto app = at<uintptr_t>(Address<0x08C11258>());
        return Guest(app, 200) && at<int>(app + 196) == 54;
    }
    inline bool GameplayCamera()
    {
        if (!GameplayApp())
            return false;
        const auto player = Player();
        return player && Vehicle() && !at<uint8_t>(player + 0xE31);
    }
    int16_t DrivingCameraPitch(uintptr_t camera, float tangentScale);
    void FlushCameraSave(uint32_t now);
    void ClearOutside(unsigned left, unsigned bottom);
    void ReadAspectSettings();
    void ReadHudSettings();
    void SelectIcons(const char* name);
    void PollSettings();
    void InstallIcons();
    void InstallInput();
    void InstallCamera();
    void InstallAspect();
    void InstallReplay();
    bool InstallHud();
    void InstallVideo();
}

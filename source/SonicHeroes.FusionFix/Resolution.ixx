module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Resolution;

import ComVars;
import Camera;

export namespace Resolution
{
    inline float ClampFov(float value)
    {
        return std::isfinite(value) ? std::clamp(value, 0.5f, 2.0f) : 1.0f;
    }

    void UpdateViewScales()
    {
        Screen.fHudZoomFactor = Screen.fAspectRatio < Screen.fConditionalAspectRatio ? Screen.fAspectRatio / Screen.fConditionalAspectRatio : 1.0f;
        Screen.fZoomFactor = Screen.fHudZoomFactor / Screen.fFovFactor;
        Screen.fCameraScaleX = ((4.0f / 3.0f) / Screen.fAspectRatio) * Screen.fZoomFactor;
        Screen.fHudScale = ((4.0f / 3.0f) / Screen.fAspectRatio) * Screen.fHudZoomFactor;
        Screen.HeightHUD = static_cast<int>(Screen.Height * Screen.fHudZoomFactor);
        Screen.OffsetX = static_cast<int>(Screen.Width - Screen.Width * Screen.fHudScale) / 2;
        Screen.OffsetY = static_cast<int>(Screen.Height - Screen.Height * Screen.fHudZoomFactor) / 2;
    }

    // Derives every size from a render size; the game's own resolution variables follow once they are found
    void Apply(int width, int height)
    {
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
            return;

        Screen.Width = width;
        Screen.Height = height;
        Screen.fWidth = float(width);
        Screen.fHeight = float(height);
        Screen.fHalfWidth = Screen.fWidth / 2.0f;
        Screen.fHalfHeight = Screen.fHeight / 2.0f;
        Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
        Screen.Width43 = int(Screen.fHeight * 4.0f / 3.0f);
        UpdateViewScales();

        // Movies are 4:3 pictures shown whole
        const float movieAspect = 4.0f / 3.0f;
        Screen.WidthFMV = Screen.fAspectRatio > movieAspect ? int(Screen.fHeight * movieAspect) : width;
        Screen.HeightFMV = Screen.fAspectRatio < movieAspect ? int(Screen.fWidth / movieAspect) : height;

        Screen.fDustWidth = 40.0f * Screen.fAspectRatio;
        Screen.fWideXPerUnit = 1.0f / (480.0f * Screen.fAspectRatio);
        Screen.fFlareHalfSize = 0.5f * std::min(Screen.fWidth, Screen.fHeight) / 480.0f;

        if (ResolutionX.is_initialized())
        {
            ResolutionX = width;
            ResolutionY = height;
            ResolutionX2 = width;
            ResolutionY2 = height;
            MaestroResX = Screen.fWidth;
            MaestroResY = Screen.fHeight;
        }
    }

    // The render size from the settings: the desktop's when none is set, the monitor's when the borderless window
    // covers it
    void Init(bool coverMonitor)
    {
        auto& settings = FixSettings();
        Screen.fConditionalAspectRatio = settings.Float("MAIN.MinAspectRatio");
        Screen.fFovFactor = ClampFov(settings.Float("MAIN.FOVFactor"));
        GameplayFov::Factor.store(Screen.fFovFactor, std::memory_order_relaxed);
        settings.OnChange("MAIN.FOVFactor", []()
        {
            Screen.fFovFactor = ClampFov(FixSettings().Float("MAIN.FOVFactor"));
            GameplayFov::Factor.store(Screen.fFovFactor, std::memory_order_relaxed);
            GameplayFov::RefreshPending.store(true, std::memory_order_relaxed);
            UpdateViewScales();
        });

        int width = settings.Int("MAIN.ResX");
        int height = settings.Int("MAIN.ResY");
        if (coverMonitor || width <= 0 || height <= 0 || width > 16384 || height > 16384)
            std::tie(width, height) = GetDesktopRes();
        if (width <= 0 || height <= 0)
            std::tie(width, height) = std::make_tuple(640, 480);
        Apply(width, height);

        // disable writes to resolution vars
        uintptr_t loc_427719 = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 85 C0 75 0A C7 05 ? ? ? ? 80 02 00 00").get_first(0));
        injector::MakeJMP(loc_427719, loc_427719 + 0x26);
        uintptr_t sub_444890 = reinterpret_cast<uintptr_t>(hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? 01 00 00 00 C7 05 ? ? ? ? 02 00 00 00 C7 05 ? ? ? ? ? ? ? ? B8 01 00 00 00").get_first(0)) - 0x48;
        injector::MakeNOP(sub_444890 + 2, 5);
        injector::MakeNOP(sub_444890 + 7, 5);
        uintptr_t loc_446B04 = reinterpret_cast<uintptr_t>(hook::pattern("B8 01 00 00 00 83 C4 08 84 C8 74 22").get_first(0)) + 8;
        injector::MakeJMP(loc_446B04, loc_446B04 + 0x4D);

        // The game reads the render size from these
        uintptr_t loc_446DB1 = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 53 68 00 00 C8 00").get_first(0));
        ResolutionX.SetAddress(*reinterpret_cast<uintptr_t*>(loc_446DB1 + 1));
        ResolutionY.SetAddress(*reinterpret_cast<uintptr_t*>(loc_446DB1 + 7));
        uintptr_t loc_446369 = reinterpret_cast<uintptr_t>(hook::pattern("E8 ? ? ? ? 8B D8 33 ED 3B DE 7E 5A").get_first(0)) - 0x10;
        ResolutionX2.SetAddress(*reinterpret_cast<uintptr_t*>(loc_446369) + 4);
        ResolutionY2.SetAddress(*reinterpret_cast<uintptr_t*>(loc_446369) + 8);

        uintptr_t loc_6A9C7D = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 00 00 00 80 3F EB 0A 8B 0D ? ? ? ? 89 4C 24 00 8B 15 ? ? ? ? A1").get_first(0)) + 0x1F;
        uintptr_t loc_6A9C87 = loc_6A9C7D + 0xA;
        MaestroResX.SetAddress(*reinterpret_cast<uintptr_t*>(loc_6A9C7D + 2));
        MaestroResY.SetAddress(*reinterpret_cast<uintptr_t*>(loc_6A9C87 + 2));

        Apply(width, height);

        uintptr_t loc_629F5F = reinterpret_cast<uintptr_t>(hook::pattern("50 FF 92 30 01 00 00 8A 44 24 1C 83 C4 0C A2 ? ? ? ? E9 ? ? ? ? 8B 15 ? ? ? ? 8D 4C 24 10 51 68 ? ? ? ? 50 FF 92 30 01 00 00 8B 4C 24 1C 33 C0 83 C4 0C 3B CD 0F 95 C0 A3").get_first(0)) + 0xE;
        uintptr_t Screen_Size_Selection = *reinterpret_cast<uintptr_t*>(loc_629F5F + 1);
        injector::WriteMemory<uint8_t>(reinterpret_cast<uint8_t*>(Screen_Size_Selection), 7, true); // Screen_Size_Selection -- force it to 7 (1280x960 32-bit)
        // disable the write
        injector::MakeNOP(loc_629F5F, 5);

        // fix for the split screen screen buffer -- force read the current res
        uintptr_t loc_61D422 = reinterpret_cast<uintptr_t>(hook::pattern("BD 80 02 00 00 C7 44 24 18 E0 01 00 00").get_first(0));
        injector::MakeJMP(loc_61D422, loc_61D422 - 8);
    }
}

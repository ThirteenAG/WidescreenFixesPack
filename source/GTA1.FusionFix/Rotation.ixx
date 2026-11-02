module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Rotation;

import ComVars;
import Raster;
import Log;

// Chinatown Wars style camera for GTA1: the view turns so that the player's
// vehicle faces the top of the screen. The software renderer projects the
// world straight down around the screen centre, so a turned view is the same
// picture rotated about that centre. The world is drawn into a square canvas
// large enough to cover the screen at any angle (its side is the screen
// diagonal), arrows and labels are drawn into it as well, and the result is
// rotated into the frame before the upright HUD is composited on top.
export namespace GTA1Rotation
{
    struct Target { uint8_t* pixels = nullptr; int width = 0, height = 0, pitch = 0; };

    SafetyHookInline WorldHook, RangesHook;
    void (__cdecl* SetCanvas)(int, int, void*) = nullptr;
    void (__cdecl* BuildRows)(void*, int, int) = nullptr;
    void (__cdecl* SetClip)(int, int, int, int) = nullptr;   // rasteriser clip: left, top, right, bottom
    int SavedClip[4]{};
    uint8_t* Cars = nullptr;                  // vehicle records, 688 bytes each
    int CarStride = 688, CarHeading = 144;   // London 1961 differs from the other packed builds

    bool RotateVehicle = true, RotateFoot = false;
    float Smoothing = 0.35f;
    bool Rendering = false, HudReady = false;
    int Side = 0;                             // canvas side
    Target Display, World;
    std::vector<uint8_t> Canvas;
    double Frequency = 0.0;
    LONGLONG LastFrame = 0;
    struct SavedView { bool valid = false; int centreX = 0, centreY = 0, width = 0, height = 0; };
    std::array<SavedView, 4> Views{};

    void Bind(const Target& target)
    {
        auto native = reinterpret_cast<int*>(GTA1Raster::OriginalRows);
        SetCanvas(target.width, target.height, target.pixels);
        BuildRows(target.pixels, target.pitch, target.height);
        pDrawPixels = target.pixels;
        nScreenWidth = target.width; nScreenHeight = target.height;
        native[GTA1Build::RasterPitch] = target.pitch; native[GTA1Build::RasterPixels] = target.pitch / std::max(1, (native[GTA1Build::RasterBytes] & 0xFF));
    }

    int* View(int player)
    {
        return reinterpret_cast<int*>(PlayerStates + GTA1Build::ViewOffset + player * GTA1Build::PlayerStride);
    }

    // Heading of the followed player: 1024 units per turn; direction (sin, cos) on screen.
    bool Heading(int player, float& heading)
    {
        auto state = PlayerStates + player * GTA1Build::PlayerStride;
        int type = *reinterpret_cast<int*>(state);   // 0 vehicle, 1 train, 2 pedestrian
        int16_t id = *reinterpret_cast<int16_t*>(state + 4);
        if (id < 0) return false;
        if (type == 0 && RotateVehicle && Cars)
        { heading = *reinterpret_cast<int16_t*>(Cars + size_t(id) * CarStride + CarHeading); return true; }
        if (type == 2 && RotateFoot && GetPed)
        { heading = *reinterpret_cast<int16_t*>(GetPed(id) + GTA1Build::PedHeading); return true; }
        return false;
    }

    void UpdateAngle(int player)
    {
        LARGE_INTEGER counter{}; QueryPerformanceCounter(&counter);
        double elapsed = LastFrame && Frequency ? (counter.QuadPart - LastFrame) / Frequency : 0.0;
        LastFrame = counter.QuadPart;
        elapsed = std::clamp(elapsed, 0.0, 0.1);
        float target = Angle, heading = 0;
        bool follow = Enabled && Available() && Heading(player, heading);
        // Heading 0 points down the screen and grows counterclockwise (1024 per turn).
        if (follow) target = float(heading) * float(M_PI) / 512.0f + float(M_PI);
        if (!Enabled) target = 0.0f;
        float difference = target - Angle;
        while (difference > float(M_PI)) difference -= 2.0f * float(M_PI);
        while (difference < -float(M_PI)) difference += 2.0f * float(M_PI);
        float blend = Smoothing > 0.0f && elapsed > 0.0 ? 1.0f - float(std::exp(-elapsed / Smoothing)) : 1.0f;
        Angle += difference * blend;
        while (Angle > float(M_PI)) Angle -= 2.0f * float(M_PI);
        while (Angle < -float(M_PI)) Angle += 2.0f * float(M_PI);
        if (!Enabled && std::fabs(Angle) < 0.0005f) Angle = 0.0f;
        Sin = std::sin(Angle); Cos = std::cos(Angle);
    }

    // Before the block ranges are built: size the local view for the canvas.
    int __cdecl BuildRanges()
    {
        int player = nLocalPlayer;
        if (player >= 0 && player < int(Views.size()) && PlayerStates)
        {
            UpdateAngle(player);
            int width = nDisplayWidth, height = nDisplayHeight;
            Side = std::min(8816, (int(std::ceil(std::hypot(double(width), double(height)))) + 16) & ~7);
            Active = Angle != 0.0f && width > 0 && height > 0;
            auto view = View(player);
            // Width43 is rounded to an integer. At 800p, 3*1066 != 4*800
            // accidentally enables the legacy 320x200 pixel-aspect correction.
            // Modern output uses square pixels at every resolution.
            reinterpret_cast<uint8_t*>(view)[44] = 0;
            auto& saved = Views[player];
            if (Active)
            {
                if (!saved.valid || view[4] != (Side << 16))
                    saved = { true, view[0], view[1], view[4], view[5] };
                view[0] = Side / 2; view[1] = Side / 2; view[4] = Side << 16; view[5] = Side << 16;
            }
            else if (saved.valid)
            {
                view[0] = saved.centreX; view[1] = saved.centreY; view[4] = saved.width; view[5] = saved.height;
                saved.valid = false;
            }
        }
        return RangesHook.ccall<int>();
    }

    // Rotate the world canvas into the frame and draw into the frame again.
    void Resolve()
    {
        if (!Rendering) return;
        Rendering = false;
        auto native = reinterpret_cast<int*>(GTA1Raster::OriginalRows);
        int bytes = (native[GTA1Build::RasterBytes] & 0xFF);
        SetClip(SavedClip[0], SavedClip[1], SavedClip[2], SavedClip[3]);
        Bind(Display);
        const double half = Side * 0.5;
        const double cx = Display.width * 0.5 - 0.5, cy = Display.height * 0.5 - 0.5;
        // Inverse rotation, in 16.16 fixed point stepping along each row.
        const int64_t stepX = int64_t(std::llround(Cos * 65536.0)), stepY = int64_t(std::llround(-Sin * 65536.0));
        for (int y = 0; y < Display.height; ++y)
        {
            double dy = y - cy, dx = -cx;
            int64_t sx = int64_t(std::llround((half + dx * Cos + dy * Sin) * 65536.0));
            int64_t sy = int64_t(std::llround((half - dx * Sin + dy * Cos) * 65536.0));
            auto output = Display.pixels + size_t(y) * Display.pitch;
            for (int x = 0; x < Display.width; ++x, sx += stepX, sy += stepY)
            {
                int ix = int(sx >> 16), iy = int(sy >> 16);
                if (unsigned(ix) < unsigned(Side) && unsigned(iy) < unsigned(Side))
                    memcpy(output + size_t(x) * bytes, World.pixels + size_t(iy) * World.pitch + size_t(ix) * bytes, bytes);
                else memset(output + size_t(x) * bytes, 0, bytes);
            }
        }
    }

    int __cdecl RenderWorld()
    {
        if (Rendering) Resolve(); // a frame without a HUD pass
        auto native = reinterpret_cast<int*>(GTA1Raster::OriginalRows);
        int bytes = (native[GTA1Build::RasterBytes] & 0xFF);
        if (!Active || (bytes != 2 && bytes != 4) || !HudReady)
            return WorldHook.ccall<int>();
        Display = { reinterpret_cast<uint8_t*>(GTA1Raster::Rows[1]), nDisplayWidth, nDisplayHeight, native[GTA1Build::RasterPitch] };
        if (!Display.pixels || Display.pitch < Display.width * bytes) return WorldHook.ccall<int>();
        Canvas.assign(size_t(Side) * Side * bytes, 0);
        World = { Canvas.data(), Side, Side, Side * bytes };
        Bind(World);
        SavedClip[0] = nClipLeft; SavedClip[1] = nClipTop; SavedClip[2] = nClipRight; SavedClip[3] = nClipBottom;
        SetClip(0, 0, Side - 1, Side - 1);
        Rendering = true;
        return WorldHook.ccall<int>();
    }

    void Install()
    {
        // Changes from FusionFixHub or a preset apply at once, as from the in-game options
        static auto read = []
        {
            SettingsReader ini("");
            Enabled = ini.ReadBoolean("CAMERA", "RotateWithPlayer", true);
            RotateVehicle = ini.ReadBoolean("CAMERA", "RotateInVehicles", true);
            RotateFoot = ini.ReadBoolean("CAMERA", "RotateOnFoot", false);
            Smoothing = std::clamp(ini.ReadFloat("CAMERA", "RotationSmoothing", 0.35f), 0.0f, 2.0f);
            ToggleKey = ini.ReadInteger("CAMERA", "ToggleRotationKey", 'C');
        };
        read();
        for (auto id : { "CAMERA.RotateWithPlayer", "CAMERA.RotateInVehicles", "CAMERA.RotateOnFoot", "CAMERA.RotationSmoothing", "CAMERA.ToggleRotationKey" })
            FixSettings().OnChange(id, read);
        LARGE_INTEGER frequency{}; QueryPerformanceFrequency(&frequency); Frequency = double(frequency.QuadPart);
        if (!GTA1Raster::OriginalRows || !PlayerStates)
        { Log::Write("GTA1 camera rotation needs the raster, presentation and input hooks."); return; }
        using GTA1Build::Find, GTA1Build::Kind;
        auto world = Find("83 EC 0C 53 55 56 57 6A 01 6A 06 E8", "83 EC 0C 53 56 57 55 6A 01 BE 05 00 00 00 6A 06 E8",
            "53 55 56 57 6A 01 6A 06 E8 ? ? ? ? 83 C4 08 B8 01 00 00 00");
        auto ranges = Find("51 A1 ? ? ? ? 53 55 56 57 50 E8 ? ? ? ? 83 C4 04 8B D8 C7 44 24",
            "53 A1 ? ? ? ? 56 57 55 BF C0 01 00 00 50 E8 ? ? ? ? 83 C4 04 8B F0 B9", "51 A1 ? ? ? ? 53 55 56 57 50 E8 ? ? ? ? 83 C4 04 8B D8 C7 44 24");
        auto canvas = GTA1Build::Find("8B 4C 24 04 8B 54 24 08 8B C1 89 15 ? ? ? ? 0F AF C2 A3",
            "8B 4C 24 04 8B 44 24 08 89 0D ? ? ? ? A3 ? ? ? ? 0F AF C1", "8B 44 24 08 8B 4C 24 04 A3 ? ? ? ? 0F AF C1 A3");
        auto rows = hook::pattern("55 8B EC 83 EC 04 53 56 57 C7 45 FC 00 00 00 00 E9 03 00 00 00 FF 45 FC 8B 45 FC 39 45 10");
        // The car getter, with the base of the car array; London 1961 cars are 4 bytes bigger.
        int carsBase = 24;
        CarStride = GTA1Build::CarStride; CarHeading = GTA1Build::CarHeading;
        auto cars = Find("0F BF 4C 24 04 8D 04 CD 00 00 00 00 2B C1 8D 04 40 8D 04 41 C1 E0 04 05",
            "0F BF 44 24 04 8B C8 8D 14 80 8D 04 D1 8D 0C 41 8D 04 CD ? ? ? ? C3",
            "0F BF 44 24 04 8D 0C 80 8D 14 C8 8D 04 50 8D 04 C5 ? ? ? ? C3");
        if (GTA1Build::Current == Kind::Retail) carsBase = 19;
        if (GTA1Build::Current == Kind::London)
        {
            carsBase = 17;
            if (cars.empty())
            {
                cars = hook::pattern("0F BF 44 24 04 8D 0C 80 8D 14 88 C1 E2 03 2B D0 8D 04 95 ? ? ? ? C3");
                carsBase = 19; CarStride = 668; CarHeading = 148;
            }
        }
        auto clip = hook::pattern("55 8B EC 53 56 57 8B 45 08 66 A3 ? ? ? ? 8B 45 08 A3 ? ? ? ? 8B 45 10 66 A3");

        SetCanvas = reinterpret_cast<decltype(SetCanvas)>(canvas.get_first());
        BuildRows = reinterpret_cast<decltype(BuildRows)>(rows.get_first());
        Cars = *cars.get_first<uint8_t*>(carsBase);
        SetClip = reinterpret_cast<decltype(SetClip)>(clip.get_first());
        nClipLeft.SetAddress(*clip.get_first<int*>(19)); nClipRight.SetAddress(*clip.get_first<int*>(36));
        nClipTop.SetAddress(*clip.get_first<int*>(53)); nClipBottom.SetAddress(*clip.get_first<int*>(70));
        RangesHook = safetyhook::create_inline(ranges.get_first(), BuildRanges);
        WorldHook = safetyhook::create_inline(world.get_first(), RenderWorld);
        Installed = true;
        Log::Write("GTA1 rotating camera installed.");
    }
}

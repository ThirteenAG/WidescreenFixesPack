#pragma once

// Chinatown Wars style camera for GTA1: the view turns so that the player's
// vehicle faces the top of the screen. The software renderer projects the
// world straight down around the screen centre, so a turned view is the same
// picture rotated about that centre. The world is drawn into a square canvas
// large enough to cover the screen at any angle (its side is the screen
// diagonal), arrows and labels are drawn into it as well, and the result is
// rotated into the frame before the upright HUD is composited on top.
namespace GTA1Rotation
{
    struct Target { uint8_t* pixels = nullptr; int width = 0, height = 0, pitch = 0; };

    inline SafetyHookInline WorldHook, RangesHook;
    inline void (__cdecl* SetCanvas)(int, int, void*) = nullptr;
    inline void (__cdecl* BuildRows)(void*, int, int) = nullptr;
    inline int *ScreenWidth = nullptr, *ScreenHeight = nullptr;
    inline void (__cdecl* SetClip)(int, int, int, int) = nullptr;   // rasteriser clip: left, top, right, bottom
    inline int *ClipLeft = nullptr, *ClipRight = nullptr, *ClipTop = nullptr, *ClipBottom = nullptr;
    inline int SavedClip[4]{};
    inline uint8_t* Cars = nullptr;                  // vehicle records, 688 bytes each
    inline int CarStride = 688, CarHeading = 144;   // London 1961 differs from the other packed builds

    inline bool Enabled = true, RotateVehicle = true, RotateFoot = false;
    inline float Smoothing = 0.35f;
    inline int ToggleKey = 'C';                     // F6 is the native pause key
    inline bool Installed = false, Active = false, Rendering = false, HudReady = false;
    inline float Angle = 0.0f, Sin = 0.0f, Cos = 1.0f;
    inline int Side = 0;                             // canvas side
    inline Target Display, World;
    inline std::vector<uint8_t> Canvas;
    inline double Frequency = 0.0;
    inline LONGLONG LastFrame = 0;
    struct SavedView { bool valid = false; int centreX = 0, centreY = 0, width = 0, height = 0; };
    inline std::array<SavedView, 4> Views{};

    // A direction on screen (x right, y down) to the direction it has in the world.
    inline void ScreenToWorld(float& x, float& y)
    {
        if (!Active) return;
        float wx = x * Cos + y * Sin, wy = -x * Sin + y * Cos;
        x = wx; y = wy;
    }

    inline void KeyDown(WPARAM key)
    {
        if (Installed && ToggleKey && int(key) == ToggleKey) Enabled = !Enabled;
    }

    inline void Bind(const Target& target)
    {
        auto native = reinterpret_cast<int*>(GTA1Raster::OriginalRows);
        SetCanvas(target.width, target.height, target.pixels);
        BuildRows(target.pixels, target.pitch, target.height);
        *GTA1Presentation::DrawPixels = target.pixels;
        *ScreenWidth = target.width; *ScreenHeight = target.height;
        native[GTA1Build::RasterPitch] = target.pitch; native[GTA1Build::RasterPixels] = target.pitch / std::max(1, (native[GTA1Build::RasterBytes] & 0xFF));
    }

    inline int* View(int player)
    {
        return reinterpret_cast<int*>(GTA1Input::PlayerState + GTA1Build::ViewOffset + player * GTA1Build::PlayerStride);
    }

    // Heading of the followed player: 1024 units per turn; direction (sin, cos) on screen.
    inline bool Heading(int player, float& heading)
    {
        auto state = GTA1Input::PlayerState + player * GTA1Build::PlayerStride;
        int type = *reinterpret_cast<int*>(state);   // 0 vehicle, 1 train, 2 pedestrian
        int16_t id = *reinterpret_cast<int16_t*>(state + 4);
        if (id < 0) return false;
        if (type == 0 && RotateVehicle && Cars)
        { heading = *reinterpret_cast<int16_t*>(Cars + size_t(id) * CarStride + CarHeading); return true; }
        if (type == 2 && RotateFoot && GTA1Input::GetPed)
        { heading = *reinterpret_cast<int16_t*>(GTA1Input::GetPed(id) + GTA1Build::PedHeading); return true; }
        return false;
    }

    inline void UpdateAngle(int player)
    {
        LARGE_INTEGER counter{}; QueryPerformanceCounter(&counter);
        double elapsed = LastFrame && Frequency ? (counter.QuadPart - LastFrame) / Frequency : 0.0;
        LastFrame = counter.QuadPart;
        elapsed = std::clamp(elapsed, 0.0, 0.1);
        float target = Angle, heading = 0;
        bool follow = Enabled && GTA1Input::Available() && Heading(player, heading);
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
    inline int __cdecl BuildRanges()
    {
        int player = GTA1Input::LocalPlayer ? *GTA1Input::LocalPlayer : -1;
        if (player >= 0 && player < int(Views.size()) && GTA1Input::PlayerState)
        {
            UpdateAngle(player);
            int width = *GTA1Presentation::Width, height = *GTA1Presentation::Height;
            Side = std::min(8816, (int(std::ceil(std::hypot(double(width), double(height)))) + 16) & ~7);
            Active = Angle != 0.0f && width > 0 && height > 0;
            auto view = View(player);
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
    inline void Resolve()
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

    inline int __cdecl RenderWorld()
    {
        if (Rendering) Resolve(); // a frame without a HUD pass
        auto native = reinterpret_cast<int*>(GTA1Raster::OriginalRows);
        int bytes = (native[GTA1Build::RasterBytes] & 0xFF);
        if (!Active || (bytes != 2 && bytes != 4) || !HudReady)
            return WorldHook.ccall<int>();
        Display = { reinterpret_cast<uint8_t*>(GTA1Raster::Rows[1]), *GTA1Presentation::Width, *GTA1Presentation::Height, native[GTA1Build::RasterPitch] };
        if (!Display.pixels || Display.pitch < Display.width * bytes) return WorldHook.ccall<int>();
        Canvas.assign(size_t(Side) * Side * bytes, 0);
        World = { Canvas.data(), Side, Side, Side * bytes };
        Bind(World);
        SavedClip[0] = *ClipLeft; SavedClip[1] = *ClipTop; SavedClip[2] = *ClipRight; SavedClip[3] = *ClipBottom;
        SetClip(0, 0, Side - 1, Side - 1);
        Rendering = true;
        return WorldHook.ccall<int>();
    }

    inline void Install()
    {
        CIniReader ini("");
        Enabled = ini.ReadBoolean("CAMERA", "RotateWithPlayer", true);
        RotateVehicle = ini.ReadBoolean("CAMERA", "RotateInVehicles", true);
        RotateFoot = ini.ReadBoolean("CAMERA", "RotateOnFoot", false);
        Smoothing = std::clamp(ini.ReadFloat("CAMERA", "RotationSmoothing", 0.35f), 0.0f, 2.0f);
        ToggleKey = ini.ReadInteger("CAMERA", "ToggleRotationKey", 'C');
        LARGE_INTEGER frequency{}; QueryPerformanceFrequency(&frequency); Frequency = double(frequency.QuadPart);
        if (!GTA1Raster::OriginalRows || !GTA1Presentation::DrawPixels || !GTA1Input::PlayerState || !GTA1Input::LocalPlayer)
        { Log::Write("GTA1 camera rotation needs the raster, presentation and input hooks."); return; }
        using GTA1Build::Find, GTA1Build::Kind;
        auto world = Find("83 EC 0C 53 55 56 57 6A 01 6A 06 E8", "83 EC 0C 53 56 57 55 6A 01 BE 05 00 00 00 6A 06 E8",
            "53 55 56 57 6A 01 6A 06 E8 ? ? ? ? 83 C4 08 B8 01 00 00 00");
        auto ranges = Find("51 A1 ? ? ? ? 53 55 56 57 50 E8 ? ? ? ? 83 C4 04 8B D8 C7 44 24",
            "53 A1 ? ? ? ? 56 57 55 BF C0 01 00 00 50 E8 ? ? ? ? 83 C4 04 8B F0 B9", "51 A1 ? ? ? ? 53 55 56 57 50 E8 ? ? ? ? 83 C4 04 8B D8 C7 44 24");
        auto canvas = GTA1Build::Find("8B 4C 24 04 8B 54 24 08 8B C1 89 15 ? ? ? ? 0F AF C2 A3",
            "8B 4C 24 04 8B 44 24 08 89 0D ? ? ? ? A3 ? ? ? ? 0F AF C1", "8B 44 24 08 8B 4C 24 04 A3 ? ? ? ? 0F AF C1 A3");
        auto rows = hook::pattern("55 8B EC 83 EC 04 53 56 57 C7 45 FC 00 00 00 00 E9 03 00 00 00 FF 45 FC 8B 45 FC 39 45 10");
        auto dimensions = hook::pattern("89 15 ? ? ? ? 8B 40 0C A3 ? ? ? ? C3");
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
        if (world.size() != 1 || ranges.size() != 1 || canvas.size() != 1 || rows.size() != 1 || dimensions.size() != 1 || cars.empty() || clip.size() != 1)
        {
            char message[192]{};
            sprintf_s(message, "GTA1 camera rotation signatures: world=%zu ranges=%zu canvas=%zu rows=%zu dimensions=%zu cars=%zu clip=%zu",
                world.size(), ranges.size(), canvas.size(), rows.size(), dimensions.size(), cars.size(), clip.size());
            Log::Write(message);
            return;
        }
        SetCanvas = reinterpret_cast<decltype(SetCanvas)>(canvas.get_first());
        BuildRows = reinterpret_cast<decltype(BuildRows)>(rows.get_first());
        ScreenWidth = *dimensions.get_first<int*>(2); ScreenHeight = *dimensions.get_first<int*>(10);
        Cars = *cars.get_first<uint8_t*>(carsBase);
        SetClip = reinterpret_cast<decltype(SetClip)>(clip.get_first());
        ClipLeft = *clip.get_first<int*>(19); ClipRight = *clip.get_first<int*>(36);
        ClipTop = *clip.get_first<int*>(53); ClipBottom = *clip.get_first<int*>(70);
        RangesHook = safetyhook::create_inline(ranges.get_first(), BuildRanges);
        WorldHook = safetyhook::create_inline(world.get_first(), RenderWorld);
        Installed = true;
        Log::Write("GTA1 rotating camera installed.");
    }
}

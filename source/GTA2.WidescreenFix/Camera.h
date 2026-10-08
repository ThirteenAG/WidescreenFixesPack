#pragma once

// Chinatown Wars style camera: the view turns so that the player's vehicle (and
// optionally the player on foot) faces the top of the screen. GTA2 renders a
// top-down perspective whose axis is the screen centre, so a rotation of the
// world around that axis is exactly a rotation of every projected vertex around
// the screen centre. The renderer DLL receives screen-space vertices through a
// table of function pointers, which is where the rotation is applied; the map
// and sprite culling ranges are widened so the corners of the rotated view stay
// filled, and world-to-screen projections used for labels are rotated as well.
namespace GTA2Camera
{
    using Draw4 = int(__stdcall*)(uint32_t, uint32_t, float*, uint32_t);
    using Draw5 = int(__stdcall*)(uint32_t, uint32_t, float*, uint32_t, uint32_t);
    inline Draw4 OriginalDrawTile = nullptr, OriginalDrawTilePart = nullptr, OriginalDrawQuad = nullptr, OriginalDrawTriangle = nullptr;
    inline Draw5 OriginalDrawQuadClipped = nullptr;
    inline void** SlotDrawTile = nullptr, ** SlotDrawTilePart = nullptr, ** SlotDrawQuad = nullptr, ** SlotDrawQuadClipped = nullptr, ** SlotDrawTriangle = nullptr;
    inline SafetyHookInline LoaderHook, ProjectionHook, LayersHook;
    inline std::array<SafetyHookInline, 2> WorldToScreenHooks;
    inline SafetyHookMid FrameHook, ArrowHook;
    inline void* (__fastcall* GetCurrentPed)(void*, int) = nullptr;
    inline int* RangeFactor = nullptr;
    inline char* ViewCamera = nullptr;     // camera the world is drawn with (set every frame)

    inline bool Enabled = true, RotateVehicle = true, RotateFoot = false;
    inline float Smoothing = 0.35f;
    inline int ToggleKey = VK_F6;
    inline bool Active = false;          // rotation applied to the current frame
    inline float Angle = 0.0f, Sin = 0.0f, Cos = 1.0f;
    inline float CenterX = 320.0f, CenterY = 240.0f;
    inline float Expand = 1.0f;          // culling range factor for the rotated view
    inline float RangeScale = 1.0f;      // extra multiplier for the culling ranges (tuning)
    inline double Frequency = 0.0;
    inline LONGLONG LastFrame = 0;
    inline bool Logged = false;

    inline float ScreenWidth() { return CenterX * 2.0f; }
    inline float ScreenHeight() { return CenterY * 2.0f; }

    // Direction on screen (x right, y down) to the direction it has in the world.
    inline void ScreenToWorldDirection(float& x, float& y)
    {
        if (!Active) return;
        float wx = x * Cos + y * Sin, wy = -x * Sin + y * Cos;
        x = wx; y = wy;
    }

    inline void Rotate(float& x, float& y)
    {
        float dx = x - CenterX, dy = y - CenterY;
        x = CenterX + dx * Cos - dy * Sin;
        y = CenterY + dx * Sin + dy * Cos;
    }

    // Renderer vertices are 32 bytes: x, y, z, 1/z, colour, specular, u, v.
    inline void RotateVertices(const float* source, float* copy, int count)
    {
        memcpy(copy, source, size_t(count) * 32);
        for (int i = 0; i < count; ++i) Rotate(copy[i * 8], copy[i * 8 + 1]);
    }

    inline int __stdcall DrawTile(uint32_t flags, uint32_t texture, float* vertices, uint32_t extra)
    {
        if (!Active || !vertices) return OriginalDrawTile(flags, texture, vertices, extra);
        float copy[32]; RotateVertices(vertices, copy, 4);
        return OriginalDrawTile(flags, texture, copy, extra);
    }
    inline int __stdcall DrawTilePart(uint32_t flags, uint32_t texture, float* vertices, uint32_t extra)
    {
        if (!Active || !vertices) return OriginalDrawTilePart(flags, texture, vertices, extra);
        float copy[32]; RotateVertices(vertices, copy, 4);
        return OriginalDrawTilePart(flags, texture, copy, extra);
    }
    inline int __stdcall DrawQuad(uint32_t flags, uint32_t texture, float* vertices, uint32_t extra)
    {
        // 0x20000 marks interface quads: HUD, text and menus stay upright.
        if (!Active || !vertices || (flags & 0x20000)) return OriginalDrawQuad(flags, texture, vertices, extra);
        float copy[32]; RotateVertices(vertices, copy, 4);
        return OriginalDrawQuad(flags, texture, copy, extra);
    }
    inline int __stdcall DrawQuadClipped(uint32_t flags, uint32_t texture, float* vertices, uint32_t extra, uint32_t clip)
    {
        if (!Active || !vertices || (flags & 0x20000)) return OriginalDrawQuadClipped(flags, texture, vertices, extra, clip);
        float copy[32]; RotateVertices(vertices, copy, 4);
        return OriginalDrawQuadClipped(flags, texture, copy, extra, clip);
    }
    inline int __stdcall DrawTriangle(uint32_t flags, uint32_t texture, float* vertices, uint32_t extra)
    {
        if (!Active || !vertices || (flags & 0x20000)) return OriginalDrawTriangle(flags, texture, vertices, extra);
        float copy[24]; RotateVertices(vertices, copy, 3);
        return OriginalDrawTriangle(flags, texture, copy, extra);
    }

    inline void Attach()
    {
        auto take = [](void** slot, auto& original, auto replacement)
        {
            if (!slot || !*slot || *slot == reinterpret_cast<void*>(replacement)) return;
            original = reinterpret_cast<std::remove_reference_t<decltype(original)>>(*slot);
            *slot = reinterpret_cast<void*>(replacement);
        };
        take(SlotDrawTile, OriginalDrawTile, DrawTile);
        take(SlotDrawTilePart, OriginalDrawTilePart, DrawTilePart);
        take(SlotDrawQuad, OriginalDrawQuad, DrawQuad);
        take(SlotDrawQuadClipped, OriginalDrawQuadClipped, DrawQuadClipped);
        take(SlotDrawTriangle, OriginalDrawTriangle, DrawTriangle);
    }
    inline int __stdcall LoadRenderer(const char* name)
    {
        // The game resolves the renderer exports into its pointer table here.
        int result = LoaderHook.stdcall<int>(name);
        Attach();
        return result;
    }

    // Native projection: the visible world width is the camera height divided
    // by the zoom, regardless of the viewport, so the game is Vert- by default.
    // Scale the zoom to keep the 4:3 vertical view and extend it sideways.
    // Afterwards widen the visibility rectangles so the rotated view's corners
    // still receive sprites and map blocks.
    // GTA3-style follow: the game still moves its camera (look-ahead, speed-dependent
    // height, cut-scene targets). The view stays locked to the followed ped and a
    // critically damped spring in real time smooths the game's offset and height,
    // removing the per-tick steps of its camera movement.
    inline bool SmoothFollow = true;
    inline float FollowTime = 0.25f, HeightTime = 0.35f, MaxLead = 0.5f;
    struct Spring { double position[3]{}, velocity[3]{}; int native[3]{}; bool smoothed = false; LONGLONG time = 0; };
    inline SafetyHookInline CameraUpdateHook;
    inline std::map<char*, Spring> Springs;
    // Position of the ped a camera follows (its vehicle when driving).
    inline bool FollowedPosition(char* camera, double out[2])
    {
        auto ped = *reinterpret_cast<char**>(camera + 0x34);
        if (!ped) return false;
        char* sprite = nullptr;
        if (auto car = *reinterpret_cast<char**>(ped + 0x16C)) sprite = *reinterpret_cast<char**>(car + 0x50);
        else if (auto object = *reinterpret_cast<char**>(ped + 0x168)) sprite = *reinterpret_cast<char**>(object + 0x80);
        if (!sprite) return false;
        out[0] = *reinterpret_cast<int*>(sprite + 0x14); out[1] = *reinterpret_cast<int*>(sprite + 0x18);
        return true;
    }
    inline void Follow(char* camera)
    {
        auto position = reinterpret_cast<int*>(camera + 0x98); // cameraPos: x, y, z
        auto state = *reinterpret_cast<int*>(camera + 0x3C);   // 1 = follow
        auto& spring = Springs[camera];
        // The projection can run more than once per tick: start again from the
        // game's own position rather than from the smoothed one written last time.
        if (spring.smoothed) memcpy(position, spring.native, sizeof(spring.native));
        memcpy(spring.native, position, sizeof(spring.native));
        spring.smoothed = false;
        LARGE_INTEGER counter{}; QueryPerformanceCounter(&counter);
        double elapsed = spring.time && Frequency ? (counter.QuadPart - spring.time) / Frequency : 0.0;
        spring.time = counter.QuadPart;
        // The camera stays locked to the followed ped; only the game's look-ahead
        // offset and height are smoothed, like GTA3's top-down camera.
        double anchor[2]{};
        bool anchored = FollowedPosition(camera, anchor);
        // The game's own camera position, which trails its look-ahead target.
        double goal[3] = { anchored ? position[0] - anchor[0] : position[0], anchored ? position[1] - anchor[1] : position[1], double(position[2]) };
        bool reset = state != 1 || !anchored || elapsed <= 0.0 || elapsed > 0.25 ||
            std::hypot(goal[0] - spring.position[0], goal[1] - spring.position[1]) / 16384.0 > 6.0;
        if (reset)
        {
            // Teleports, cut-scene cameras and long pauses: start from the game's position.
            spring.position[0] = anchored ? position[0] - anchor[0] : position[0];
            spring.position[1] = anchored ? position[1] - anchor[1] : position[1];
            spring.position[2] = position[2];
            for (auto& velocity : spring.velocity) velocity = 0;
            return;
        }
        for (int i = 0; i < 3; ++i)
        {
            double time = i < 2 ? FollowTime : (goal[i] > spring.position[i] ? HeightTime * 0.5 : HeightTime);
            if (time <= 0.0) { spring.position[i] = goal[i]; spring.velocity[i] = 0; }
            else
            {
                double omega = 2.0 / time, offset = spring.position[i] - goal[i];
                double change = (spring.velocity[i] + omega * offset) * elapsed, decay = std::exp(-omega * elapsed);
                spring.velocity[i] = (spring.velocity[i] - omega * change) * decay;
                spring.position[i] = goal[i] + (offset + change) * decay;
            }
        }
        // Keep the followed vehicle on screen: limit the look-ahead to half the
        // visible half-height (the game's lead grows with speed).
        double zoom = *reinterpret_cast<int*>(camera + 0xA4);
        if (zoom > 0)
        {
            double halfHeight = (8.0 * 16384.0 + spring.position[2]) / zoom * 0.375 * 16384.0;
            double lead = std::hypot(spring.position[0], spring.position[1]), limit = halfHeight * MaxLead;
            if (lead > limit && lead > 0) { spring.position[0] *= limit / lead; spring.position[1] *= limit / lead; }
        }
        for (int i = 0; i < 3; ++i)
            position[i] = int(std::lround(spring.position[i] + (i < 2 ? anchor[i] : 0.0)));
        spring.smoothed = true;
    }
    // The game's camera step starts from its own previous position, not the smoothed one.
    inline int __fastcall UpdateCamera(char* camera, int)
    {
        {
            auto found = Springs.find(camera);
            if (found != Springs.end() && found->second.smoothed)
            {
                memcpy(camera + 0x98, found->second.native, sizeof(found->second.native));
                found->second.smoothed = false;
            }
        }
        return CameraUpdateHook.thiscall<int>(camera);
    }

    inline int* __fastcall UpdateProjection(char* camera, int)
    {
        if (SmoothFollow) Follow(camera);
        auto& zoom = *reinterpret_cast<int*>(camera + 0xA4);
        auto width = *reinterpret_cast<int*>(camera + 0x68), height = *reinterpret_cast<int*>(camera + 0x6C);
        auto original = zoom;
        if (width > 0 && height > 0 && zoom > 0)
            zoom = int(std::lround(double(zoom) * (4.0 / 3.0) / (double(width) / height)));
        auto result = ProjectionHook.thiscall<int*>(camera);
        zoom = original;
        if (Active)
        {
            // A turned view reaches the world through its corners in every
            // direction: make both halves of the visibility box the half-diagonal.
            for (int offset : { 0x20, 0x78 })
            {
                auto rect = reinterpret_cast<int*>(camera + offset); // left, right, top, bottom (16.14)
                double cx = (double(rect[0]) + rect[1]) * 0.5, cy = (double(rect[2]) + rect[3]) * 0.5;
                double radius = std::hypot((double(rect[1]) - rect[0]) * 0.5, (double(rect[3]) - rect[2]) * 0.5) * RangeScale;
                rect[0] = int(std::lround(cx - radius)); rect[1] = int(std::lround(cx + radius));
                rect[2] = int(std::lround(cy - radius)); rect[3] = int(std::lround(cy + radius));
                if (offset == 0x78) { rect[0] = std::max(rect[0], 0); rect[2] = std::max(rect[2], 0); }
            }
        }
        return result;
    }

    inline int __fastcall DrawLayers(void* layers, int)
    {
        if (!RangeFactor || !ViewCamera) return LayersHook.thiscall<int>(layers);
        // The block range is depth / zoom wide and 0.75 of that tall, from the
        // 4:3 screen. Widen it to the screen's aspect ratio, and for a turned view
        // to the screen diagonal in both directions.
        auto& zoom = *reinterpret_cast<int*>(ViewCamera + 0xA4);
        double aspect = std::max(1.0, (double(CenterX) / CenterY) / (4.0 / 3.0));
        // Turned: both half-extents reach the half-diagonal (the 4:3 height is 0.75 of the width).
        double horizontal = aspect * (Active ? Expand : 1.0) * RangeScale;
        double vertical = (Active ? aspect * Expand / 0.75 : 1.0) * RangeScale;
        auto savedZoom = zoom; auto savedFactor = *RangeFactor;
        zoom = std::max(1, int(std::lround(zoom / horizontal)));
        *RangeFactor = int(std::lround(savedFactor * vertical / horizontal));
        auto result = LayersHook.thiscall<int>(layers);
        zoom = savedZoom; *RangeFactor = savedFactor;
        return result;
    }

    inline void RotateProjected(int* outX, int* outY)
    {
        if (!Active || !outX || !outY) return;
        float px = *outX / 16384.0f, py = *outY / 16384.0f;
        Rotate(px, py);
        *outX = int(std::lround(px * 16384.0f)); *outY = int(std::lround(py * 16384.0f));
    }
    inline int __fastcall WorldToScreen0(void* camera, int, int x, int y, int z, int* outX, int* outY)
    {
        auto result = WorldToScreenHooks[0].thiscall<int>(camera, x, y, z, outX, outY);
        RotateProjected(outX, outY);
        return result;
    }

    inline void KeyDown(WPARAM key)
    {
        if (ToggleKey && int(key) == ToggleKey) Enabled = !Enabled;
    }

    // Once per rendered frame, before the world is drawn.
    inline void Frame(void* game)
    {
        auto player = game ? *reinterpret_cast<char**>(static_cast<char*>(game) + 0x1C) : nullptr;
        auto camera = player ? player + 0x14C : nullptr;
        ViewCamera = camera;
        if (camera)
        {
            int width = *reinterpret_cast<int*>(camera + 0x68), height = *reinterpret_cast<int*>(camera + 0x6C);
            if (width > 0 && height > 0) { CenterX = width * 0.5f; CenterY = height * 0.5f; }
            Expand = float(std::sqrt(1.0 + double(CenterY) * CenterY / (double(CenterX) * CenterX))) * RangeScale;
        }
        LARGE_INTEGER counter{}; QueryPerformanceCounter(&counter);
        double elapsed = LastFrame && Frequency ? (counter.QuadPart - LastFrame) / Frequency : 0.0;
        LastFrame = counter.QuadPart;
        elapsed = std::clamp(elapsed, 0.0, 0.1);

        float target = 0.0f;
        bool follow = false;
        auto ped = player && GetCurrentPed ? static_cast<char*>(GetCurrentPed(player, 0)) : nullptr;
        // A new pedestrian (respawn after being wasted or busted, new game) starts upright.
        static char* lastPed = nullptr;
        if (ped != lastPed)
        {
            if (ped && !*reinterpret_cast<char**>(ped + 0x16C)) { Angle = 0.0f; LastFrame = 0; }
            lastPed = ped;
        }
        if (Enabled && ped)
        {
            auto car = *reinterpret_cast<char**>(ped + 0x16C);
            auto object = *reinterpret_cast<char**>(ped + 0x168);
            int heading = 0;
            if (car && RotateVehicle)
            {
                auto sprite = *reinterpret_cast<char**>(car + 0x50);
                if (sprite) { heading = *reinterpret_cast<uint16_t*>(sprite); follow = true; }
            }
            else if (!car && object && RotateFoot)
            {
                heading = *reinterpret_cast<int16_t*>(object + 0x40); follow = true;
            }
            // Headings count 1440 units per turn, zero pointing down the screen and
            // growing counterclockwise; the view turns clockwise by the angle.
            if (follow) target = float(heading) * float(M_PI) / 720.0f - float(M_PI);
        }
        // Like Chinatown Wars, keep the current orientation while not following
        // (on foot by default): turning the view with the pedestrian would make
        // screen-relative movement curve endlessly.
        if (!follow && Enabled) target = Angle;
        float difference = target - Angle;
        while (difference > float(M_PI)) difference -= 2.0f * float(M_PI);
        while (difference < -float(M_PI)) difference += 2.0f * float(M_PI);
        float blend = Smoothing > 0.0f ? 1.0f - float(std::exp(-elapsed / Smoothing)) : 1.0f;
        if (!LastFrame || elapsed <= 0.0) blend = 1.0f;
        Angle += difference * blend;
        while (Angle > float(M_PI)) Angle -= 2.0f * float(M_PI);
        while (Angle < -float(M_PI)) Angle += 2.0f * float(M_PI);
        if (!Enabled && std::fabs(Angle) < 0.0005f) Angle = 0.0f;
        Active = Angle != 0.0f && OriginalDrawTile != nullptr;
        Sin = std::sin(Angle); Cos = std::cos(Angle);
        if (Log::Enabled && !Logged && Active)
        {
            char message[200]{};
            sprintf_s(message, "GTA2 camera rotation active: centre=%.0f,%.0f expand=%.3f range=%.3f zoom=%.3f z=%.3f", CenterX, CenterY, Expand,
                RangeFactor ? *RangeFactor / 16384.0 : 0.0, camera ? *reinterpret_cast<int*>(camera + 0xA4) / 16384.0 : 0.0,
                camera ? *reinterpret_cast<int*>(camera + 0xA0) / 16384.0 : 0.0);
            Log::Write(message); Logged = true;
        }
    }

    inline void** ExportSlot(const char* name)
    {
        // push offset "gbh_Xxx"; mov eax, module; push eax; call GetProcAddress; mov slot, eax
        static auto sites = hook::pattern("68 ? ? ? ? A1 ? ? ? ? 50 FF 15 ? ? ? ? A3 ? ? ? ? 83 3D");
        for (size_t i = 0; i < sites.size(); ++i)
        {
            auto site = sites.get(i).get<uint8_t>();
            auto text = *reinterpret_cast<const char**>(site + 1);
            if (!IsBadReadPtr(text, 32) && strcmp(text, name) == 0) return *reinterpret_cast<void***>(site + 18);
        }
        return nullptr;
    }

    inline void Install(CIniReader& ini)
    {
        Enabled = ini.ReadBoolean("CAMERA", "RotateWithPlayer", true);
        RotateVehicle = ini.ReadBoolean("CAMERA", "RotateInVehicles", true);
        RotateFoot = ini.ReadBoolean("CAMERA", "RotateOnFoot", false);
        Smoothing = std::clamp(ini.ReadFloat("CAMERA", "RotationSmoothing", 0.35f), 0.0f, 2.0f);
        ToggleKey = ini.ReadInteger("CAMERA", "ToggleRotationKey", VK_F6);
        RangeScale = std::clamp(ini.ReadFloat("CAMERA", "RangeScale", 1.0f), 0.5f, 4.0f);
        SmoothFollow = ini.ReadBoolean("CAMERA", "SmoothFollow", true);
        FollowTime = std::clamp(ini.ReadFloat("CAMERA", "FollowTime", 0.25f), 0.0f, 2.0f);
        HeightTime = std::clamp(ini.ReadFloat("CAMERA", "HeightTime", 0.35f), 0.0f, 3.0f);
        MaxLead = std::clamp(ini.ReadFloat("CAMERA", "MaxLookAhead", 0.5f), 0.0f, 1.0f);
        LARGE_INTEGER frequency{}; QueryPerformanceFrequency(&frequency); Frequency = double(frequency.QuadPart);

        auto projection = hook::pattern("83 EC 10 53 55 56 8B F1 57 8D 44 24 14 8B 4E 68 8D BE A4 00 00 00");
        if (projection.size() == 1) ProjectionHook = safetyhook::create_inline(projection.get_first(), UpdateProjection);
        auto update = hook::pattern("51 56 57 8B F1 E8 ? ? ? ? 8D BE A0 00 00 00 8D 44 24 08 57 50 B9");
        if (update.size() == 1) CameraUpdateHook = safetyhook::create_inline(update.get_first(), UpdateCamera);
        else SmoothFollow = false;

        SlotDrawTile = ExportSlot("gbh_DrawTile");
        SlotDrawTilePart = ExportSlot("gbh_DrawTilePart");
        SlotDrawQuad = ExportSlot("gbh_DrawQuad");
        SlotDrawQuadClipped = ExportSlot("gbh_DrawQuadClipped");
        SlotDrawTriangle = ExportSlot("gbh_DrawTriangle");
        auto loader = hook::pattern("55 8B EC B8 80 13 00 00 E8 ? ? ? ? 53 56 57 83 7D 08 00 75 07 33 C0 E9");
        auto layers = hook::pattern("81 EC 80 00 00 00 A0 ? ? ? ? 53 55 56 8B F1 57 84 C0 89 74 24 10");
        auto range = hook::pattern("68 ? ? ? ? 52 8D 4C 24 20 E8 ? ? ? ? 8B 00 8D 4C 24 2C 8D 54 24 4C 51 52");
        auto frame = hook::pattern("8B 41 1C 8B 0D ? ? ? ? 05 4C 01 00 00 A3 ? ? ? ? E8");
        auto world = hook::pattern("83 EC 10 8D 54 24 1C 56 8B F1 57 8D 4C 24 08 8D 86 A0 00 00 00 50 51 8D 44 24 14 52 50 B9 ? ? ? ? E8");
        auto ped = hook::pattern("8B 41 68 83 F8 02 74 0C 83 F8 03 74 07 8B 81 C4 02 00 00 C3");
        if (!SlotDrawTile || !SlotDrawQuad || !SlotDrawTriangle || loader.size() != 1 || layers.size() != 1 ||
            range.size() != 1 || frame.size() != 1 || world.empty() || ped.size() != 1)
        {
            char message[200]{};
            sprintf_s(message, "GTA2 camera rotation signatures: slots=%d,%d,%d,%d,%d loader=%zu layers=%zu range=%zu frame=%zu world=%zu ped=%zu",
                SlotDrawTile != nullptr, SlotDrawTilePart != nullptr, SlotDrawQuad != nullptr, SlotDrawQuadClipped != nullptr,
                SlotDrawTriangle != nullptr, loader.size(), layers.size(), range.size(), frame.size(), world.size(), ped.size());
            Log::Write(message);
            return;
        }
        GetCurrentPed = reinterpret_cast<decltype(GetCurrentPed)>(ped.get_first());
        RangeFactor = *range.get_first<int*>(1);
        LoaderHook = safetyhook::create_inline(loader.get_first(), LoadRenderer);
        LayersHook = safetyhook::create_inline(layers.get_first(), DrawLayers);
        // Only the copy used for labels and on-screen tests: the other one feeds
        // arrows and money pops, which are world quads the draw hook already turns.
        WorldToScreenHooks[0] = safetyhook::create_inline(world.get(0).get<void>(), WorldToScreen0);
        FrameHook = safetyhook::create_mid(frame.get_first(), [](SafetyHookContext& context)
        {
            Frame(reinterpret_cast<void*>(context.ecx));
        });
        Attach(); // the renderer may already be loaded

        // Target arrows. Far targets get a ring of fixed on-screen radius, kept in
        // units of 64 pixels (one block at the default camera height). Near targets
        // place the arrow a set world distance short of the target, but the game
        // still converts that radius through the 64-pixel unit, so the arrow lands
        // too far out whenever the camera is higher than default (issue 2181).
        // Keep near-target radii in world units.
        auto arrow = hook::pattern("8B 08 68 ? ? ? ? 89 4C 24 14 8D 4C 24 18 E8 ? ? ? ? 85 C0 75 ? 8B 46 78 8A 48 20");
        if (arrow.size() == 1)
            ArrowHook = safetyhook::create_mid(arrow.get_first(2), [](SafetyHookContext& context)
            {
                auto arrowState = reinterpret_cast<char*>(context.esi);
                auto target = *reinterpret_cast<char**>(arrowState + 0x78);
                bool close = target && *reinterpret_cast<uint8_t*>(target + 0x20);
                if (close) context.ecx = *reinterpret_cast<uint32_t*>(arrowState + 0x10);
            });
        else Log::Write("GTA2 target arrow signature unavailable.");
        Log::Write("GTA2 rotating camera installed.");
    }
}

#pragma once

namespace GTA1Hud
{
    inline SafetyHookInline DrawHook, ProjectedHook, LabelsHook;
    inline SafetyHookMid GlyphHook, SpriteHook;
    inline void (__cdecl* SetCanvas)(int, int, void*) = nullptr;
    inline void (__cdecl* BuildRows)(void*, int, int) = nullptr;
    inline int *ScreenWidth = nullptr, *ScreenHeight = nullptr;
    inline bool Enabled = true, Drawing = false, LabelsPending = false;
    inline int Width = 0, Height = 0, Bytes = 0;
    inline int SafeArea = 8;
    inline std::vector<uint8_t> Canvas, Alpha;
    inline std::vector<std::pair<int, int>> Bounds;
    inline std::vector<void*> Projected;

    // Preserve coverage separately from colour so opaque black text remains
    // opaque. Native glyphs/sprites use palette index zero for transparency.
    inline void Coverage(int x, int y, const uint8_t* source, int width, int height, int stride)
    {
        if (!Drawing || !source || width <= 0 || height <= 0 || stride < width) return;
        int left = std::max(0, x), right = std::min(Width, x + width);
        int top = std::max(0, y), bottom = std::min(Height, y + height);
        for (int row = top; row < bottom; ++row)
            for (int col = left; col < right; ++col)
                if (source[size_t(row - y) * stride + col - x])
                {
                    Alpha[size_t(row) * Width + col] = 1;
                    Bounds[row].first = std::min(Bounds[row].first, col);
                    Bounds[row].second = std::max(Bounds[row].second, col + 1);
                }
    }

    inline int __cdecl DrawProjected(void* object)
    {
        // World-space arrows retain their actual camera projection.
        if (Drawing) { Projected.push_back(object); return 0; }
        return ProjectedHook.call<int>(object);
    }
    inline char __cdecl DrawLabels()
    {
        if (Drawing) { LabelsPending = true; return 0; }
        return LabelsHook.call<char>();
    }

    inline char __cdecl Draw()
    {
        InputDevices::Frontend = false;
        // With the rotating camera the world is still in its square canvas here.
        const bool rotated = GTA1Rotation::Rendering;
        auto native = reinterpret_cast<int*>(GTA1Raster::OriginalRows);
        auto pixels = rotated ? GTA1Rotation::Display.pixels : reinterpret_cast<uint8_t*>(GTA1Raster::Rows[1]);
        int bytes = (native[GTA1Build::RasterBytes] & 0xFF), pitch = rotated ? GTA1Rotation::Display.pitch : native[GTA1Build::RasterPitch];
        int width = *GTA1Presentation::Width, height = *GTA1Presentation::Height;
        if (!Enabled || !pixels || (bytes != 2 && bytes != 4) || pitch < width * bytes || height <= 480)
        {
            if (rotated) GTA1Rotation::Resolve();
            static bool reported = false;
            if (Log::Enabled && !reported)
            {
                char message[160]{};
                sprintf_s(message, "GTA1 HUD bypass: enabled=%d pixels=%p bytes=%d pitch=%d size=%dx%d", Enabled, pixels, bytes, pitch, width, height);
                Log::Write(message); reported = true;
            }
            return DrawHook.call<char>();
        }
        float scale = std::min(height / 480.0f, width / 640.0f);
        int margin = std::clamp(int(std::lround(SafeArea * scale)), 0, std::min(width, height) / 4);
        int outputWidth = width - margin * 2, outputHeight = height - margin * 2;
        Width = std::clamp(int(std::lround(outputWidth / scale)), 640 - SafeArea * 2, 7680);
        Height = std::clamp(int(std::lround(outputHeight / scale)), 480 - SafeArea * 2, 4320);
        Bytes = bytes;
        Canvas.resize(size_t(Width) * Height * bytes);
        Alpha.assign(size_t(Width) * Height, 0);
        Bounds.assign(Height, {Width, 0});
        std::fill(Canvas.begin(), Canvas.end(), 0);
        Projected.clear();
        LabelsPending = false;
        int oldWidth = *ScreenWidth, oldHeight = *ScreenHeight;
        int oldPitchPixels = native[GTA1Build::RasterPixels];
        *ScreenWidth = Width; *ScreenHeight = Height;
        native[GTA1Build::RasterPitch] = Width * bytes; native[GTA1Build::RasterPixels] = Width;
        *GTA1Presentation::DrawPixels = Canvas.data();
        SetCanvas(Width, Height, Canvas.data());
        BuildRows(Canvas.data(), Width * bytes, Height);
        Drawing = true;
        auto result = DrawHook.call<char>();
        Drawing = false;
        static int lastWidth = 0, lastHeight = 0;
        if (Log::Enabled && (lastWidth != width || lastHeight != height))
        {
            char message[180]{};
            sprintf_s(message, "GTA1 HUD: output=%dx%d canvas=%dx%d bytes=%d pitch=%d coverage=%zu", width, height,
                Width, Height, bytes, pitch, std::count(Alpha.begin(), Alpha.end(), uint8_t(1)));
            Log::Write(message); lastWidth = width; lastHeight = height;
        }
        if (rotated)
        {
            // Arrows and labels belong to the world: draw them before turning it.
            GTA1Rotation::Bind(GTA1Rotation::World);
            for (auto object : Projected) ProjectedHook.call<int>(object);
            if (LabelsPending) LabelsHook.call<char>();
            Projected.clear(); LabelsPending = false;
            GTA1Rotation::Resolve();
        }
        else
        {
            *ScreenWidth = oldWidth; *ScreenHeight = oldHeight;
            native[GTA1Build::RasterPitch] = pitch; native[GTA1Build::RasterPixels] = oldPitchPixels;
            *GTA1Presentation::DrawPixels = pixels;
            SetCanvas(width, height, pixels);
            BuildRows(pixels, pitch, height);
        }
        for (int y = 0; y < outputHeight; ++y)
        {
            int sy = int(int64_t(y) * Height / outputHeight);
            auto [left, right] = Bounds[sy];
            if (left >= right) continue;
            auto output = pixels + size_t(y + margin) * pitch + margin * bytes;
            int first = int((int64_t(left) * outputWidth + Width - 1) / Width);
            int last = int((int64_t(right) * outputWidth + Width - 1) / Width);
            for (int x = first; x < last; ++x)
            {
                size_t index = size_t(sy) * Width + int(int64_t(x) * Width / outputWidth);
                if (Alpha[index])
                {
                    memcpy(output + x * bytes, Canvas.data() + index * bytes, bytes);
                }
            }
        }
        for (auto object : Projected) ProjectedHook.call<int>(object);
        if (LabelsPending) LabelsHook.call<char>();
        return result;
    }

    inline void Install()
    {
        CIniReader ini("");
        SafeArea = std::clamp(ini.ReadInteger("MAIN", "HudSafeArea", 8), 0, 32);
        if (!GTA1Raster::OriginalRows || !GTA1Presentation::DrawPixels) return;
        using GTA1Build::Find;
        auto draw = Find("A1 ? ? ? ? 55 33 ED 3B C5 0F 85 ? ? ? ? A1 ? ? ? ? 53 56 3B C5", "53 A1 ? ? ? ? 56 85 C0 57 55 0F 85 ? ? ? ? A1",
            "A1 ? ? ? ? 85 C0 0F 85 ? ? ? ? A1 ? ? ? ? 85 C0 74 05 E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 05 E8");
        auto canvas = GTA1Build::Find("8B 4C 24 04 8B 54 24 08 8B C1 89 15 ? ? ? ? 0F AF C2 A3",
            "8B 4C 24 04 8B 44 24 08 89 0D ? ? ? ? A3 ? ? ? ? 0F AF C1", "8B 44 24 08 8B 4C 24 04 A3 ? ? ? ? 0F AF C1 A3");
        auto rows = hook::pattern("55 8B EC 83 EC 04 53 56 57 C7 45 FC 00 00 00 00 E9 03 00 00 00 FF 45 FC 8B 45 FC 39 45 10");
        auto dimensions = hook::pattern("89 15 ? ? ? ? 8B 40 0C A3 ? ? ? ? C3");
        auto projected = Find("83 EC 2C 8B 44 24 30 33 D2 53 55 8B 08 56 81 E1 00 00 FF FF 8B 68 04 89 4C 24 30 8B 48 48",
            "8B 54 24 04 83 EC 24 53 8B 02 56 25 00 00 FF FF 57 8B 4A 04 55 81 E1 00 00 FF FF 8B 5A 46",
            "83 EC 38 53 55 56 57 8B 7C 24 4C 33 C0 33 C9 8B 17 8B 77 04 8B 6F 46 81 E2 00 00 FF FF");
        auto labels = Find("83 EC 1C A1 ? ? ? ? 53 55 56 57 50 E8 ? ? ? ? 6A 00 E8 ? ? ? ? 83 C4 08 8B E8 E8",
            "83 EC 24 A1 ? ? ? ? 53 56 57 55 50 E8 ? ? ? ? 83 C4 04 33 F6 6A 00 E8", "83 EC 20 A1 ? ? ? ? 53 55 56 57 50 E8 ? ? ? ? 83 C4 04 6A 00 E8");
        auto glyph = hook::pattern("55 8B EC 83 C4 FC 50 53 51 52 56 57 B8 00 00 00 00 8B 15 ? ? ? ? 8B 5D 18 2B 5D 0C D1 E3");
        auto dispatch = hook::pattern("55 8B EC 80 3D ? ? ? ? 02 75 19 FF 75 18 FF 75 14 FF 75 10 FF 75 0C FF 75 08 E8 ? ? ? ? 83 C4 14 EB 17");
        if (draw.size() != 1 || canvas.size() != 1 || rows.size() != 1 || dimensions.size() != 1 ||
            projected.size() != 1 || labels.size() != 1 || glyph.size() != 1 || dispatch.size() != 2)
        {
            char diagnostic[192]{};
            sprintf_s(diagnostic, "GTA1 HUD signatures: draw=%zu canvas=%zu rows=%zu dimensions=%zu projected=%zu glyph=%zu dispatch=%zu",
                draw.size(), canvas.size(), rows.size(), dimensions.size(), projected.size(), glyph.size(), dispatch.size());
            Log::Write(diagnostic); return;
        }
        SetCanvas = reinterpret_cast<decltype(SetCanvas)>(canvas.get_first());
        BuildRows = reinterpret_cast<decltype(BuildRows)>(rows.get_first());
        ScreenWidth = *dimensions.get_first<int*>(2); ScreenHeight = *dimensions.get_first<int*>(10);
        for (size_t i = 0; i < dispatch.size(); ++i)
        {
            auto entry = dispatch.get(i).get<uint8_t>();
            auto target = entry + 32 + *reinterpret_cast<int32_t*>(entry + 28);
            if (target == glyph.get_first())
                GlyphHook = safetyhook::create_mid(entry, [](SafetyHookContext& context)
                {
                    if (!Drawing) return;
                    auto args = reinterpret_cast<uint32_t*>(context.esp + 4);
                    auto destination = reinterpret_cast<uint8_t*>(args[3]);
                    intptr_t offset = destination - Canvas.data();
                    if (offset < 0 || size_t(offset) >= Canvas.size()) return;
                    Coverage(int(offset / Bytes % Width), int(offset / Bytes / Width),
                        reinterpret_cast<uint8_t*>(args[0]), args[1], args[2], args[1]);
                });
            else
                SpriteHook = safetyhook::create_mid(entry, [](SafetyHookContext& context)
                {
                    if (!Drawing) return;
                    auto args = reinterpret_cast<uint32_t*>(context.esp + 4);
                    Coverage(int(args[0]), int(args[1]), reinterpret_cast<uint8_t*>(args[2]), args[3], args[4], 256);
                });
        }
        if (!GlyphHook || !SpriteHook)
        { Log::Write("GTA1 HUD coverage hooks unavailable."); return; }
        ProjectedHook = safetyhook::create_inline(projected.get_first(), DrawProjected);
        LabelsHook = safetyhook::create_inline(labels.get_first(), DrawLabels);
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
        GTA1Rotation::HudReady = bool(DrawHook);
        Log::Write("GTA1 HUD scaling installed.");
    }
}

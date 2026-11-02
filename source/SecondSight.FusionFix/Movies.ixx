module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>

// Movies (the startup logo videos) keep their aspect ratio and are shown whole with black bars, inside the game's 4:3
// area (Original) or the whole screen (Fill). Skip Intro goes from the start straight to the title, past the logo videos and the
// copyright screens.

export module Movies;


export namespace SecondSightMovies
{
    inline SafetyHookMid DrawHook;
    // Width of the 4:3 box the copyright screen, the title card and the title logo are laid out in
    inline uint32_t BoxWidth = 640;

    inline void Resize(uint32_t width, uint32_t height)
    {
        BoxWidth = (std::min)(width, height * 4 / 3);
    }

    struct Vertex
    {
        float x, y, z, rhw;
        uint32_t color;
        float u, v;
    };

    // Standard-definition videos are 4:3 whatever their stored size
    inline float DisplayAspect(uint32_t width, uint32_t height)
    {
        if (width <= 720 && (height == 480 || height == 576))
            return 4.0f / 3.0f;
        return height ? float(width) / float(height) : 4.0f / 3.0f;
    }

    // The movie is one screen-sized strip of four pre-transformed vertices with the frame as texture 0
    inline void Draw(SafetyHookContext& context)
    {
        auto device = *reinterpret_cast<IDirect3DDevice9**>(context.esp);
        auto vertices = *reinterpret_cast<Vertex**>(context.esp + 12);
        if (!device || !vertices)
            return;
        IDirect3DBaseTexture9* base = nullptr;
        if (FAILED(device->GetTexture(0, &base)) || !base)
            return;
        D3DSURFACE_DESC desc{};
        IDirect3DTexture9* texture = nullptr;
        const bool known = SUCCEEDED(base->QueryInterface(__uuidof(IDirect3DTexture9), reinterpret_cast<void**>(&texture))) &&
            texture && SUCCEEDED(texture->GetLevelDesc(0, &desc));
        if (texture)
            texture->Release();
        base->Release();
        if (!known)
            return;

        const float width = vertices[3].x, height = vertices[3].y;
        if (width <= 0.0f || height <= 0.0f)
            return;
        const float movie = DisplayAspect(desc.Width, desc.Height), screen = width / height;
        const int mode = std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 1);
        const float area = mode == 0 ? 4.0f / 3.0f : screen;
        const float scaleX = std::min(1.0f, area / screen) * std::min(1.0f, movie / area);
        const float scaleY = std::min(1.0f, screen / area) * std::min(1.0f, area / movie);
        for (int i = 0; i < 4; ++i)
        {
            auto& vertex = vertices[i];
            // The whole movie, centred; the cleared screen shows black around it
            vertex.x = width * 0.5f + (vertex.x - width * 0.5f) * scaleX;
            vertex.y = height * 0.5f + (vertex.y - height * 0.5f) * scaleY;
        }
    }

    inline void Init()
    {
        auto draw = hook::pattern("6A 1C 8D 4C 24 ? 51 6A 02 6A 05 50 FF 92 4C 01 00 00");
        DrawHook = safetyhook::create_mid(draw.get_first(12), &Draw);

        for (auto screen : { "A1 ? ? ? ? 0F AF C7 99 81 E2 FF 01 00 00 03 C2 8B E8 A1", "A1 ? ? ? ? 8B 0D ? ? ? ? 69 C0 F6 00 00 00",
            "A1 ? ? ? ? C1 E0 08 99 81 E2 FF 01 00 00 03" })
        {
            auto width = hook::pattern(screen);
            injector::WriteMemory<uint32_t*>(width.get_first<uint32_t*>(1), &BoxWidth, true);
        }

        if (!FixSettings().Bool("MAIN.SkipIntro"))
            return;
        // The start sequence: logo videos, then the copyright screen and a title card, then the title. Its first step
        // is replaced by a jump to the last.
        auto intro = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 C7 05 ? ? ? ? 02 00 00 00");
        auto start = intro.get_first<uint8_t>(0);
        const auto state = *intro.get_first<uint32_t>(25);
        injector::WriteMemory<uint16_t>(start, 0x05C7, true); // mov dword ptr [state], 6
        injector::WriteMemory<uint32_t>(start + 2, state, true);
        injector::WriteMemory<uint32_t>(start + 6, 6, true);
        injector::MakeJMP(start + 10, start + 33, true);
    }
}

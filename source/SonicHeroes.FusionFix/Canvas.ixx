module;

#include <stdafx.h>

export module Canvas;

import ComVars;

// The pop-up windows (Extras, notices, save prompts) and the staff roll text are laid out for a 640x480 screen:
// their windows are sized from the resolution, but borders, buttons, text and logos use fixed 640x480 sizes and
// end up small and stretched. The fix lets them build their layout in 640x480 units and, while they draw, maps
// every 2D vertex from that canvas into the centred 4:3 box of the screen, so the whole layout keeps its shape at
// any resolution and aspect ratio.
export namespace Canvas
{
    struct Im2DVertex
    {
        float x, y, z, rhw;
        uint32_t color;
        float u, v;
    };
    static_assert(sizeof(Im2DVertex) == 28);

    inline int Active = 0;
    inline std::vector<Im2DVertex> Mapped;

    inline SafetyHookInline RenderPrimitive;
    inline SafetyHookInline RenderIndexedPrimitive;

    inline const int CanvasWidth = 640;
    inline const int CanvasHeight = 480;

    inline Im2DVertex* Map(Im2DVertex* vertices, int count)
    {
        if (!Active || !vertices || count <= 0)
            return vertices;
        const float scale = Scale();
        const float x = OffsetX();
        const float y = OffsetY();
        Mapped.assign(vertices, vertices + count);
        for (auto& vertex : Mapped)
        {
            vertex.x = x + vertex.x * scale;
            vertex.y = y + vertex.y * scale;
        }
        return Mapped.data();
    }

    inline int __cdecl Render(int type, Im2DVertex* vertices, int count)
    {
        return RenderPrimitive.ccall<int>(type, Map(vertices, count), count);
    }

    inline int __cdecl RenderIndexed(int type, Im2DVertex* vertices, int count, void* indices, int indexCount)
    {
        return RenderIndexedPrimitive.ccall<int>(type, Map(vertices, count), count, indices, indexCount);
    }

    // A draw method of the game's 2D objects (thiscall, no arguments) that runs on the canvas
    template<int Id>
    struct CanvasDraw
    {
        static inline int(__thiscall* Original)(uintptr_t) = nullptr;
        static inline bool(*Wanted)(uintptr_t) = nullptr;

        static int __fastcall Draw(uintptr_t self, void*)
        {
            const bool canvas = !Wanted || Wanted(self);
            Active += canvas;
            const int result = Original(self);
            Active -= canvas;
            return result;
        }

        static void Install(uintptr_t vtable, bool(*wanted)(uintptr_t) = nullptr)
        {
            auto slot = reinterpret_cast<uintptr_t*>(vtable + 8);
            Original = reinterpret_cast<decltype(Original)>(*slot);
            Wanted = wanted;
            injector::WriteMemory(slot, reinterpret_cast<uintptr_t>(&Draw), true);
        }
    };

    // fmul [address] sites of one function
    inline void Redirect(uintptr_t function, size_t size, uintptr_t constant, size_t expected, float* value)
    {
        const auto bytes = reinterpret_cast<const uint8_t*>(&constant);
        const auto text = std::format("D8 0D {:02X} {:02X} {:02X} {:02X}", bytes[0], bytes[1], bytes[2], bytes[3]);
        auto sites = hook::range_pattern(function, function + size, text);
        if (sites.size() != expected)
            throw std::runtime_error("2D scale sites not found");
        sites.for_each_result([value](hook::pattern_match match)
        {
            injector::WriteMemory(match.get<void>(2), value, true);
        });
    }

    void Init()
    {
        RenderPrimitive = safetyhook::create_inline(hook::pattern("A1 ? ? ? ? FF 60 30 90").get_first(), &Render);
        RenderIndexedPrimitive = safetyhook::create_inline(hook::pattern("A1 ? ? ? ? 8B 40 34 FF E0").get_first(), &RenderIndexed);

        // Pop-up windows: position and size are kept in canvas units
        auto window = hook::pattern("DB 05 ? ? ? ? D8 4D 14 D8 0D ? ? ? ? D9 5B 3C DB 05 ? ? ? ? BF 01 00 00 00 8D 83 20 01 00 00 3B C2 "
            "D8 4D 18 D8 0D ? ? ? ? D9 5B 40 DB 05 ? ? ? ? D8 4D 20 D8 0D ? ? ? ? D9 5B 48 DB 05 ? ? ? ? D8 4D 24");
        for (const auto [offset, size] : { std::pair{ 0x00, &CanvasWidth }, std::pair{ 0x12, &CanvasHeight }, std::pair{ 0x31, &CanvasWidth }, std::pair{ 0x43, &CanvasHeight } })
            injector::WriteMemory(window.get_first(offset + 2), size, true);
        const auto pixelToUnit = *window.get_first<uintptr_t>(0x0B);

        const auto windowVtable = *hook::pattern("C7 03 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 33 D2").get_first<uintptr_t>(2);
        CanvasDraw<0>::Install(windowVtable);

        // Staff roll: its horizontal origin in canvas units; the text and logo page (state 5) draws on the canvas,
        // the full-screen fades of the other states don't
        injector::WriteMemory(hook::pattern("DB 05 ? ? ? ? D8 0F D8 0D ? ? ? ? D9 9E").get_first(2), &CanvasWidth, true);
        const auto staffRollVtable = *hook::pattern("C7 06 ? ? ? ? 66 C7 46 1E A4 00").get_first<uintptr_t>(2);
        CanvasDraw<1>::Install(staffRollVtable, [](uintptr_t self) { return *reinterpret_cast<int32_t*>(self + 0x28) == 5; });

        // The race timer and split screen panels, the loading screen and the movie picture place their 2D across the
        // whole width
        const auto at = [](std::string_view signature) { return reinterpret_cast<uintptr_t>(hook::get_pattern(signature)); };
        Redirect(at("81 EC BC 00 00 00 53 8B 9C 24 C4 00 00 00 83 7B 2C FF"), 0x78D, pixelToUnit, 10, &Screen.fWideXPerUnit);
        Redirect(at("8B 40 04 81 EC B4 00 00 00 56 8B F1 85 F6 75 16"), 0x35F, pixelToUnit, 2, &Screen.fWideXPerUnit);
        Redirect(at("83 EC 1C 53 56 8B F0 C7 44 24 0C 00 00 00 00 E8"), 0x55B, pixelToUnit, 2, &Screen.fWideXPerUnit);

        // Lens flare sprites grow with the resolution like the rest of the picture
        injector::WriteMemory(hook::pattern("D9 05 ? ? ? ? 8B 46 1C D8 4E 10 8B 48 30 8B 56 18 8B 42 14 85 C0 D8 0D").get_first(0x19), &Screen.fFlareHalfSize, true);
    }
}

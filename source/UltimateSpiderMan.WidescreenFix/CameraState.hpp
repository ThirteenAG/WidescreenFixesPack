#pragma once

namespace USMCamera
{
    inline uintptr_t* Game = nullptr;
    inline uintptr_t* ComicCamera = nullptr;
    inline uintptr_t ChaseVtable = 0, ControllerVtable = 0, FixedVtable = 0;
    inline float* Zoom = nullptr;
    inline uint8_t* IgnoreZoom = nullptr;
    inline bool ComicPass = false;

    inline bool IsGameplay()
    {
        if (ComicPass || !Game || !ComicCamera || *ComicCamera || !ChaseVtable || !ControllerVtable || !FixedVtable)
            return false;
        const auto game = *Game;
        if (!game || *reinterpret_cast<const uint8_t*>(game + 370))
            return false;
        const auto world = *reinterpret_cast<const uintptr_t*>(game + 80);
        const auto camera = *reinterpret_cast<const uintptr_t*>(game + 92);
        if (!world || !camera || camera != *reinterpret_cast<const uintptr_t*>(game + 96) ||
            camera != *reinterpret_cast<const uintptr_t*>(world + 564) ||
            *reinterpret_cast<const uintptr_t*>(camera) != ChaseVtable)
            return false;
        const auto hero = *reinterpret_cast<const uintptr_t*>(world + 560);
        if (!hero || !*reinterpret_cast<const uintptr_t*>(hero + 28) ||
            *reinterpret_cast<const uintptr_t*>(camera + 280) != *reinterpret_cast<const uintptr_t*>(hero + 28))
            return false;
        // The chase camera can also be driven by an animation or a scripted fixed view.
        if (*reinterpret_cast<const uintptr_t*>(camera + 116))
            return false;
        const auto controller = *reinterpret_cast<const uintptr_t*>(camera + 416);
        if (!controller || *reinterpret_cast<const uintptr_t*>(controller) != ControllerVtable)
            return false;
        const auto fixed = *reinterpret_cast<const uintptr_t*>(controller + 8);
        if (!fixed || *reinterpret_cast<const uintptr_t*>(fixed) != FixedVtable ||
            *reinterpret_cast<const uint8_t*>(fixed + 36))
            return false;
        return Zoom && IgnoreZoom && (*IgnoreZoom || (std::isfinite(*Zoom) && std::abs(*Zoom - 1.0f) < 0.0001f));
    }

    inline void Initialize(float* zoom, uint8_t* ignoreZoom)
    {
        Game = ComicCamera = nullptr;
        ChaseVtable = ControllerVtable = FixedVtable = 0;
        Zoom = nullptr;
        IgnoreZoom = nullptr;
        auto game = hook::pattern("8B 0D ? ? ? ? 56 6A 00 E8 ? ? ? ? D9 05 ? ? ? ? D8 1D");
        auto comic = hook::pattern("E8 ? ? ? ? 83 C4 04 85 C0 75 ? 8B 47 50 8B 80 34 02 00 00 8B 6F 5C");
        auto chase = hook::pattern("C7 06 ? ? ? ? 33 DB 88 9E BC 01 00 00 88 9E CC 01 00 00");
        auto controller = hook::pattern("89 7E 04 89 46 08 C7 06 ? ? ? ? 8B 4F 08 C1 E9 1C");
        auto fixed = hook::pattern("89 48 04 89 50 08 C7 00 ? ? ? ? 8B 15 ? ? ? ? 8D 48 0C");
        if (game.size() != 1 || comic.size() != 1 || chase.size() != 1 || controller.size() != 1 || fixed.size() != 1)
        {
            OutputDebugStringA("Ultimate Spider-Man: gameplay camera identity unavailable; user FOV remains neutral.\n");
            return;
        }
        const auto call = reinterpret_cast<uintptr_t>(comic.get_first());
        const auto getter = call + 5 + *reinterpret_cast<const int32_t*>(call + 1);
        if (*reinterpret_cast<const uint8_t*>(getter) != 0xA1 || *reinterpret_cast<const uint8_t*>(getter + 5) != 0xC3)
        {
            OutputDebugStringA("Ultimate Spider-Man: comic camera identity unavailable; user FOV remains neutral.\n");
            return;
        }
        Game = *game.get_first<uintptr_t*>(2);
        ComicCamera = *reinterpret_cast<uintptr_t**>(getter + 1);
        ChaseVtable = *chase.get_first<uintptr_t>(2);
        ControllerVtable = *controller.get_first<uintptr_t>(8);
        FixedVtable = *fixed.get_first<uintptr_t>(8);
        Zoom = zoom;
        IgnoreZoom = ignoreZoom;
    }
}

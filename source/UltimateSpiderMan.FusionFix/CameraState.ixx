module;

#include "stdafx.h"

export module CameraState;

import ComVars;

export namespace USMCamera
{
    inline GameRef<uintptr_t>* Game = nullptr;
    inline GameRef<uintptr_t>* ComicCamera = nullptr;
    inline uintptr_t ChaseVtable = 0, ControllerVtable = 0, BridgeVtable = 0, FixedVtable = 0;
    inline GameRef<float>* FOV = nullptr;
    inline GameRef<float>* Zoom = nullptr;
    inline GameRef<uint8_t>* IgnoreZoom = nullptr;
    inline bool ComicPass = false;

    inline float LimitGameplayFactor(float fov, float aspectDiff, float factor)
    {
        if (!std::isfinite(factor))
            return 1.0f;
        factor = std::clamp(factor, 0.5f, 2.0f);
        if (!std::isfinite(fov) || fov <= 0.0f || fov >= 3.14159265359f ||
            !std::isfinite(aspectDiff) || aspectDiff <= 0.0f)
            return 1.0f;
        if (factor <= 1.0f)
            return factor;
        const auto tangent = std::tan(static_cast<double>(fov) * 0.5) * aspectDiff;
        if (!std::isfinite(tangent) || tangent <= 0.0f)
            return 1.0f;
        // Preserve the neutral Hor+ view if the authored camera is already wider.
        constexpr double maximumTangent = 3.73205080757; // tan(150 degrees / 2)
        return static_cast<float>(std::min(static_cast<double>(factor), std::max(1.0, maximumTangent / tangent)));
    }

    inline bool IsGameplay()
    {
        if (ComicPass || !Game || !ComicCamera || !ChaseVtable || !ControllerVtable || !BridgeVtable || !FixedVtable)
            return false;
        const auto game = Game->get();
        if (!game || *reinterpret_cast<const uint8_t*>(game + 370))
            return false;
        const auto world = *reinterpret_cast<const uintptr_t*>(game + 80);
        const auto camera = *reinterpret_cast<const uintptr_t*>(game + 92);
        if (ComicCamera->get() && ComicCamera->get() != camera)
            return false;
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
        const auto bridge = *reinterpret_cast<const uintptr_t*>(controller + 8);
        if (!bridge || *reinterpret_cast<const uintptr_t*>(bridge) != BridgeVtable)
            return false;
        const auto fixed = *reinterpret_cast<const uintptr_t*>(bridge + 8);
        if (!fixed || *reinterpret_cast<const uintptr_t*>(fixed) != FixedVtable ||
            *reinterpret_cast<const uint8_t*>(fixed + 36))
            return false;
        return Zoom && IgnoreZoom && (IgnoreZoom->get() || (std::isfinite(Zoom->get()) && std::abs(Zoom->get() - 1.0f) < 0.0001f));
    }

    inline void Initialize(GameRef<float>* fov, GameRef<float>* zoom, GameRef<uint8_t>* ignoreZoom)
    {
        Game = ComicCamera = nullptr;
        ChaseVtable = ControllerVtable = BridgeVtable = FixedVtable = 0;
        FOV = Zoom = nullptr;
        IgnoreZoom = nullptr;
        auto game = hook::pattern("8B 0D ? ? ? ? 56 6A 00 E8 ? ? ? ? D9 05 ? ? ? ? D8 1D");
        auto comic = hook::pattern("E8 ? ? ? ? 83 C4 04 85 C0 75 ? 8B 47 50 8B 80 34 02 00 00 8B 6F 5C");
        auto chase = hook::pattern("C7 06 ? ? ? ? 33 DB 88 9E BC 01 00 00 88 9E CC 01 00 00");
        auto controller = hook::pattern("89 7E 04 89 46 08 C7 06 ? ? ? ? 8B 4F 08 C1 E9 1C");
        auto bridge = hook::pattern("89 77 04 89 47 08 C7 07 ? ? ? ? 8B DF 8B 4C 24 14");
        auto fixed = hook::pattern("89 48 04 89 50 08 C7 00 ? ? ? ? 8B 15 ? ? ? ? 8D 48 0C");
        const auto call = reinterpret_cast<uintptr_t>(comic.get_first());
        const auto getter = call + 5 + *reinterpret_cast<const int32_t*>(call + 1);
        Game = BindGameRef(::game, *game.get_first<uintptr_t*>(2));
        ComicCamera = BindGameRef(comicCamera, *reinterpret_cast<uintptr_t**>(getter + 1));
        ChaseVtable = *chase.get_first<uintptr_t>(2);
        ControllerVtable = *controller.get_first<uintptr_t>(8);
        BridgeVtable = *bridge.get_first<uintptr_t>(8);
        FixedVtable = *fixed.get_first<uintptr_t>(8);
        FOV = fov;
        Zoom = zoom;
        IgnoreZoom = ignoreZoom;
    }
}

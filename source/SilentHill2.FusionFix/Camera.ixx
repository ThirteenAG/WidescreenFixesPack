module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"
#include <numbers>
#include <bit>


export module Camera;

import ComVars;

export namespace SH2Camera
{
    inline int(__cdecl* ScriptedCamera)() = nullptr;
    inline int(__cdecl* NativeView)() = nullptr;
    inline SafetyHookInline LogicHook;
    inline float Aspect = 4.0f / 3.0f;
    inline uint32_t SavedLens = 0, ProjectedLens = 0;
    inline bool Scoped = false;

    inline float ScaleLens(float lens, float aspect, float factor)
    {
        if (factor == 1.0f || !std::isfinite(factor) || factor <= 0.0f || !std::isfinite(lens) || lens <= 0.0f || !std::isfinite(aspect) || aspect <= 0.0f)
            return lens;
        // The native perspective matrix uses 2 * atan(256 / lens) vertically.
        const float halfWidth = 256.0f * aspect;
        const float angle = 2.0f * std::atan(halfWidth / lens);
        constexpr float limit = 150.0f * std::numbers::pi_v<float> / 180.0f;
        const float widened = std::clamp(angle * factor, 0.01f, std::max(angle, limit));
        return halfWidth / std::tan(widened * 0.5f);
    }

    inline void Restore()
    {
        if (!Scoped)
            return;
        if (std::bit_cast<uint32_t>(CameraLens.get()) == ProjectedLens)
            CameraLens = std::bit_cast<float>(SavedLens);
        Scoped = false;
    }

    inline void BeginView()
    {
        Restore();
        if (GameMode.get() != 13 || GamePhase.get() != 4 || ScriptedCamera() != 0)
            return;
        const float original = CameraLens.get();
        const float projected = ScaleLens(original, Aspect, FixSettings().Float("MAIN.FOVFactor"));
        SavedLens = std::bit_cast<uint32_t>(original);
        ProjectedLens = std::bit_cast<uint32_t>(projected);
        if (SavedLens == ProjectedLens)
            return;
        CameraLens = projected;
        Scoped = true;
    }

    inline int __cdecl View()
    {
        // Camera updates have finished; visibility and rendering share this temporary lens.
        BeginView();
        return NativeView();
    }

    inline int __cdecl Logic()
    {
        // Also restore when a lost device skipped presentation.
        Restore();
        return LogicHook.ccall<int>();
    }

    inline void Init(float aspect)
    {
        auto cache = hook::pattern("51 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? A1 ? ? ? ? 89 0D ? ? ? ? D9 05 ? ? ? ? D8 0D ? ? ? ? A3");
        auto world = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 6A 00 E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? A1");
        auto logic = hook::pattern("83 EC 10 53 55 56 33 F6 57 89 74 24 10 89 74 24 18 E8 ? ? ? ? F6 05 ? ? ? ? 20 75 05 E8");
        auto mode = hook::pattern("83 3D ? ? ? ? 0D 8B 15 ? ? ? ? 75 1F 83 FA 04 75 1A");
        auto scripted = hook::pattern("83 EC 50 E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? E8");
        CameraLens.SetAddress(*cache.get_first<float*>(23));
        GameMode.SetAddress(*mode.get_first<int*>(2));
        GamePhase.SetAddress(*mode.get_first<int*>(9));
        auto scene = reinterpret_cast<const uint8_t*>(injector::GetBranchDestination(scripted.get_first(29)).as_int());
        ScriptedCamera = reinterpret_cast<decltype(ScriptedCamera)>(const_cast<uint8_t*>(scene));
        Aspect = aspect;
        NativeView = reinterpret_cast<decltype(NativeView)>(injector::GetBranchDestination(world.get_first(15)).as_int());
        LogicHook = safetyhook::create_inline(logic.get_first(), &Logic);
        injector::MakeCALL(world.get_first(15), &View, true);
        auto& present = SHDisplay::OnPresent();
        present.insert(present.begin(), &Restore);
        SHDisplay::OnBeforeReset().push_back([](IDirect3DDevice8*) { Restore(); });
    }
}

module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module HUD;

import Bindings;
import ComVars;
import Cutscenes;

export namespace SaboteurHUD
{
    inline std::atomic<float> Constraint{ 0.0f };
    inline GameRef<uintptr_t>* Renderer = nullptr;
    inline GameRef<float>* MinimapBounds = nullptr;
    inline SafetyHookMid BeginHook, EndHook;
    inline SafetyHookInline MinimapUpdateHook;

    struct Value
    {
        uint32_t type{ 0 };
        uint32_t padding{ 0 };
        union
        {
            double number{ 0.0 };
            const char* text;
        };
    };
    static_assert(sizeof(Value) == 16);

    inline Value Text(const char* text)
    {
        Value value;
        value.type = 4;
        value.text = text;
        return value;
    }

    inline bool Invoke(void* movie, const char* name, std::initializer_list<Value> values, Value* result = nullptr)
    {
        const auto invoke = reinterpret_cast<bool(__thiscall*)(void*, const char*, Value*, const Value*, unsigned)>(
            (*reinterpret_cast<void***>(movie))[18]);
        return invoke(movie, name, result, values.begin(), static_cast<unsigned>(values.size()));
    }

    inline bool Get(void* movie, const char* name, double& result)
    {
        Value value;
        const auto get = reinterpret_cast<bool(__thiscall*)(void*, Value*, const char*)>((*reinterpret_cast<void***>(movie))[12]);
        if (!get(movie, &value, name) || value.type != 3 || !std::isfinite(value.number))
            return false;
        result = value.number;
        return true;
    }

    inline void Set(void* movie, const char* name, double number)
    {
        Value value{ 3, 0, number };
        const auto set = reinterpret_cast<bool(__thiscall*)(void*, const char*, Value*, int)>((*reinterpret_cast<void***>(movie))[11]);
        set(movie, name, &value, 1);
    }

    inline void* InterfaceMovie = nullptr;
    inline ULONGLONG NextInterfaceRequest = 0;

    inline void EnsureInterface(void* movie)
    {
        double version = 0;
        if (Get(movie, "_global.FFInterfaceVersion", version) && version == 2)
        {
            SaboteurCutscenes::ReadyMovie = reinterpret_cast<uintptr_t>(movie);
            return;
        }
        SaboteurCutscenes::ReadyMovie = 0;
        if (InterfaceMovie != movie)
        {
            InterfaceMovie = movie;
            NextInterfaceRequest = 0;
        }
        const auto now = GetTickCount64();
        if (now < NextInterfaceRequest)
            return;
        NextInterfaceRequest = now + 1000;
        Value depth;
        if (Invoke(movie, "_root.getNextHighestDepth", {}, &depth) && depth.type == 3 &&
            std::isfinite(depth.number) && depth.number >= 0 && depth.number < 2147483647.0 &&
            Invoke(movie, "_root.createEmptyMovieClip", { Text("ffFusionFix"), depth }))
            Invoke(movie, "_root.ffFusionFix.loadMovie", { Text("FusionFixInterface.gfx") });
    }

    inline bool Mask(void* movie, float inset, float scale)
    {
        if (!Renderer || !Renderer->get() || !std::isfinite(scale) || scale <= 0)
            return false;
        const auto width = *reinterpret_cast<uint32_t*>(Renderer->get() + 1652) / scale;
        const auto height = *reinterpret_cast<uint32_t*>(Renderer->get() + 1656) / scale;
        return Invoke(movie, "_global.FFHudMask", { { 3, 0, inset }, { 3, 0, (1280.0 - width) * 0.5 },
            { 3, 0, (720.0 - height) * 0.5 }, { 3, 0, width }, { 3, 0, height } });
    }

    struct NativeOffsets
    {
        std::vector<std::tuple<float*, float, float>> native;

        void Move(float* address, float delta)
        {
            const auto original = *address;
            if (std::isfinite(original))
            {
                native.emplace_back(address, original, original + delta);
                *address = original + delta;
            }
        }

        ~NativeOffsets()
        {
            for (const auto& [address, original, applied] : native)
                if (*address == applied)
                    *address = original;
        }
    };

    struct Scope : NativeOffsets
    {
        using NativeOffsets::Move;
        struct Position
        {
            const char* name;
            double original;
            double applied;
        };
        void* movie;
        float scale;
        bool masked{ false };
        std::vector<Position> positions;

        void Move(const char* name, double delta)
        {
            double original = 0.0;
            if (Get(movie, name, original))
            {
                positions.push_back({ name, original, original + delta });
                Set(movie, name, original + delta);
            }
        }

        Scope(uintptr_t manager, float inset, float scale) : movie{ *reinterpret_cast<void**>(manager + 2688) }, scale{ scale }
        {
            masked = Mask(movie, inset, scale);
            if (!masked)
                return;
            for (const auto name : { "Tutorial._x", "Mail._x", "Minimap._x" })
                Move(name, inset);
            for (const auto name : { "ObjectiveTray._x", "Inventory._x", "RaceHUD._x", "Pickup._x" })
                Move(name, -inset);
            Move("TimerHUD._x", *reinterpret_cast<float*>(manager + 48204) - inset);
            const auto minimap = *reinterpret_cast<uintptr_t*>(manager + 52 + 22 * 4);
            if (minimap && *reinterpret_cast<int*>(minimap + 4) == 22)
            {
                Move(reinterpret_cast<float*>(minimap + 144), inset * scale);
                Move(MinimapBounds->get_ptr(), inset * scale);
                Move(MinimapBounds->get_ptr() + 2, inset * scale);
            }
        }

        ~Scope()
        {
            for (const auto& position : positions)
            {
                double current = 0.0;
                if (Get(movie, position.name, current) && std::abs(current - position.applied) < 0.0001)
                    Set(movie, position.name, position.original);
            }
            if (masked)
                Mask(movie, 0, scale);
        }
    };
    inline thread_local std::optional<Scope> Current;

    inline void __fastcall UpdateMinimap(uintptr_t self, void* edx, float deltaTime)
    {
        NativeOffsets offsets;
        const auto hud = SaboteurBindings::HUD();
        if (hud && hud->get() && SaboteurCutscenes::Ready() && !Current && Renderer && Renderer->get() && MinimapBounds &&
            *reinterpret_cast<uintptr_t*>(hud->get() + 52 + 22 * 4) == self)
        {
            const auto constraint = Constraint.load();
            const auto width = *reinterpret_cast<uint32_t*>(Renderer->get() + 1652);
            const auto height = *reinterpret_cast<uint32_t*>(Renderer->get() + 1656);
            if (constraint && width && height)
            {
                const auto chosen = ClampHudAspectRatio(constraint, static_cast<float>(width) / height);
                const auto inset = (width - height * chosen) * 0.5f;
                offsets.Move(reinterpret_cast<float*>(self + 144), inset);
                offsets.Move(MinimapBounds->get_ptr(), inset);
                offsets.Move(MinimapBounds->get_ptr() + 2, inset);
            }
        }
        MinimapUpdateHook.unsafe_fastcall(self, edx, deltaTime);
    }

    struct FrameScope
    {
        void* movie;
        ~FrameScope()
        {
            Invoke(movie, "_global.FFCinemaEnd", {});
        }
    };
    inline thread_local std::optional<FrameScope> Frame;

    inline void Begin(uintptr_t manager)
    {
        Current.reset();
        Frame.reset();
        const auto constraint = Constraint.load();
        if (!manager || !Renderer || !Renderer->get() || !MinimapBounds || !*reinterpret_cast<void**>(manager + 2688))
            return;
        EnsureInterface(*reinterpret_cast<void**>(manager + 2688));
        const auto width = *reinterpret_cast<uint32_t*>(Renderer->get() + 1652);
        const auto height = *reinterpret_cast<uint32_t*>(Renderer->get() + 1656);
        const auto scale = *reinterpret_cast<float*>(manager + 48212);
        if (!width || !height || !std::isfinite(scale) || scale <= 0.0f)
            return;
        const auto movie = *reinterpret_cast<void**>(manager + 2688);
        if (SaboteurCutscenes::Ready() && Invoke(movie, "_global.FFCinemaBegin", {
            { 3, 0, static_cast<double>(SaboteurCutscenes::Borders.load()) },
            { 3, 0, SaboteurCutscenes::Animated.load() ? 1.0 : 0.0 },
            { 3, 0, SaboteurCutscenes::Active() ? 1.0 : 0.0 },
            { 3, 0, static_cast<double>(*reinterpret_cast<uint8_t*>(manager + 3212) & 1) },
            { 3, 0, (1280.0 - width / scale) * 0.5 }, { 3, 0, (720.0 - height / scale) * 0.5 },
            { 3, 0, width / scale }, { 3, 0, height / scale } }))
            Frame.emplace(movie);
        const auto aspect = static_cast<float>(width) / height;
        const auto chosen = constraint ? ClampHudAspectRatio(constraint, aspect) : aspect;
        const auto inset = (width - height * chosen) * 0.5f / scale;
        if (inset > 0.0001f || std::abs(*reinterpret_cast<float*>(manager + 48204)) > 0.0001f)
            Current.emplace(manager, inset, scale);
    }

    inline void Install()
    {
        const auto update = []()
        {
            Constraint.store(ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint")).value_or(0.0f));
        };
        update();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", update);
        Renderer = SaboteurBindings::Renderer();
        auto begin = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8D 7E 34 BD 29 00 00 00 8D 9B 00 00 00 00 83 3F 00 74 16 8B 0F 8B 11 8B 42 18");
        auto end = hook::pattern("68 ? ? ? ? FF D3 D9 86 AC BB 00 00 D9 5C 24 20 83 EC 10");
        auto bounds = hook::pattern("D9 1D ? ? ? ? D9 44 24 20 D9 1D ? ? ? ? D9 86 94 00 00 00 D8 C1");
        auto updateMinimap = hook::pattern("D9 44 24 04 83 EC 40 53 55 56 8B F1 D8 86 A4 00 00 00 33 DB 39 5E 10 57 D9 9E A4 00 00 00");
        MinimapBounds = BindGameRef(minimapBounds, *bounds.get_first<float*>(2));
        MinimapUpdateHook = safetyhook::create_inline(updateMinimap.get_first(), UpdateMinimap);
        EndHook = safetyhook::create_mid(end.get_first(), [](SafetyHookContext&) { Current.reset(); Frame.reset(); });
        if (EndHook && MinimapUpdateHook)
            BeginHook = safetyhook::create_mid(begin.get_first(), [](SafetyHookContext& context) { Begin(context.esi); });
        const bool available = BeginHook && EndHook && MinimapUpdateHook;
        FixSettings().Availability("MAIN.HudAspectRatioConstraint", [available](std::string_view value)
        {
            return value == "Auto" || available;
        });
    }
}

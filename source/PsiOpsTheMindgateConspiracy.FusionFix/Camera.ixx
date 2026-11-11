module;
#include <stdafx.h>
#include "FixSettings.hpp"

export module Camera;
import ComVars;
import CameraMath;
import FloatingPoint;


export namespace PsiOpsCamera
{
    inline SafetyHookInline constructorHook, buildHook, projectionHook, controlHook, updateHook;
    inline std::atomic<void*> worldCamera{ nullptr };
    inline std::atomic<float> baseAngle{ 0.0f }, factor{ 1.0f };
    inline std::atomic<int> borders{ 3 };
    inline std::atomic<bool> controlsDisabled{ false }, changed{ false };
    inline thread_local void* drawingCamera = nullptr;

    inline float GameplayAngle()
    {
        const float degrees = gameplayFov.is_initialized() ? gameplayFov.get() : 0.0f;
        return std::isfinite(degrees) && degrees > 0.0f && degrees < 180.0f
            ? degrees * (3.14159265358979323846f / 180.0f) : baseAngle.load();
    }

    inline bool Cinematic()
    {
        return cinematicState.is_initialized() && cinematicState != 0;
    }

    inline void* __fastcall Construct(void* self, void*, float degrees)
    {
        {
            PsiOps::FloatingPointScope floatingPoint;
            baseAngle = degrees * (3.14159265358979323846f / 180.0f);
        }
        worldCamera = self;
        controlsDisabled = false;
        changed = true;
        return constructorHook.unsafe_thiscall<void*>(self, degrees);
    }

    inline void* __fastcall Build(void* self, void*)
    {
        struct Scope
        {
            void* previous;
            Scope(void* camera) : previous(std::exchange(drawingCamera, camera)) {}
            ~Scope() { drawingCamera = previous; }
        } scope(self);
        return buildHook.unsafe_thiscall<void*>(self);
    }

    inline void* __fastcall Projection(void* self, void*, void* output, float angle, float nearPlane, float farPlane, int clipping)
    {
        volatile float adjusted = angle;
        if (drawingCamera && drawingCamera == worldCamera.load() && !controlsDisabled.load() && !Cinematic())
        {
            PsiOps::FloatingPointScope floatingPoint;
            if (std::abs(angle - GameplayAngle()) <= 0.0001f)
            {
                const auto viewport = *(reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(self) + 532));
                const auto width = *(reinterpret_cast<int*>(viewport + 8));
                const auto height = *(reinterpret_cast<int*>(viewport + 12));
                const auto pixelAspect = *(reinterpret_cast<float*>(static_cast<uint8_t*>(self) + 16));
                if (width > 0 && height > 0)
                    adjusted = Multiply(angle, float(width) / float(height) * pixelAspect, factor.load());
            }
        }
        const auto result = projectionHook.unsafe_thiscall<void*>(self, output, float(adjusted), nearPlane, farPlane, clipping);
        if (drawingCamera && drawingCamera == worldCamera.load() && output && Cinematic())
        {
            PsiOps::FloatingPointScope floatingPoint;
            const auto viewport = *(reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(self) + 532));
            const auto width = *(reinterpret_cast<int*>(viewport + 8));
            const auto height = *(reinterpret_cast<int*>(viewport + 12));
            const auto framing = Frame(width, height, borders);
            for (size_t row = 0; row < 4; ++row)
            {
                auto x = static_cast<float*>(output) + row * 4;
                auto y = static_cast<float*>(output) + row * 4 + 1;
                *x *= framing.x;
                *y *= framing.y;
            }
        }
        return result;
    }

    inline void __fastcall Control(void* self, void*, uint8_t disabled)
    {
        controlsDisabled = disabled != 0;
        changed = true;
        controlHook.unsafe_thiscall<void>(self, disabled);
    }

    inline uintptr_t __fastcall Update(void* self, void*, uint8_t force, uint8_t parent)
    {
        if (self == worldCamera.load() && changed.exchange(false))
            injector::WriteMemory<uint8_t>(static_cast<uint8_t*>(self) + 41, 1, true);
        return updateHook.unsafe_thiscall<uintptr_t>(self, force, parent);
    }

    inline void Init()
    {
        auto cinematic = hook::pattern("C6 80 54 02 00 00 01 C6 05 ? ? ? ? 00 8B E5 5D C3");
        cinematicState.SetAddress(*cinematic.get_first<uint8_t*>(9));
        auto gameplay = hook::pattern("68 00 00 48 42 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 8B 8D 68 FE FF FF E8");
        // The getter is push ebp; mov ebp, esp; mov eax, imm32 (the FOV address); pop ebp; ret
        const auto getter = injector::GetBranchDestination(gameplay.get_first<uint8_t>(5)).as_int();
        gameplayFov.SetAddress(*reinterpret_cast<float**>(getter + 4));
        auto constructor = hook::pattern("68 00 00 20 42 8B 4D E8 E8 ? ? ? ? 89 45 B8 EB 07");
        auto build = hook::pattern("55 8B EC 81 EC 88 01 00 00 56 57 89 8D 78 FE FF FF 8B 8D 78 FE FF FF E8 ? ? ? ? E8 ? ? ? ? 89 45 FC");
        auto projection = hook::pattern("55 8B EC 83 EC 14 89 4D EC 8B 45 EC 8B 88 14 02 00 00 89 4D F0 8B 4D F0 E8");
        auto control = hook::pattern("55 8B EC 51 89 4D FC 0F B6 45 08 85 C0 74 0E 6A 00 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 8B 4D FC 8A 55 08 88 91 24 03 00 00");
        auto update = hook::pattern("55 8B EC 51 89 4D FC 0F B6 45 08 85 C0 74 0A 8B 4D FC E8 ? ? ? ? EB 0B 8B 4D FC 8B 11 8B 4D FC FF 52 3C");
        const auto target = injector::GetBranchDestination(constructor.get_first<uint8_t>(8)).as_int();
        constructorHook = safetyhook::create_inline(reinterpret_cast<void*>(target), Construct);
        buildHook = safetyhook::create_inline(build.get_first(), Build);
        projectionHook = safetyhook::create_inline(projection.get_first(), Projection);
        controlHook = safetyhook::create_inline(control.get_first(), Control);
        updateHook = safetyhook::create_inline(update.get_first(), Update);
        const auto updateFactor = []()
        {
            factor = FixSettings().Float("MAIN.FOVFactor");
            changed = true;
        };
        const auto updateBorders = []()
        {
            borders = FixSettings().Int("MAIN.CutsceneBorders");
            changed = true;
        };
        updateBorders();
        FixSettings().OnChange("MAIN.CutsceneBorders", updateBorders);
        updateFactor();
        FixSettings().OnChange("MAIN.FOVFactor", updateFactor);
    }
}

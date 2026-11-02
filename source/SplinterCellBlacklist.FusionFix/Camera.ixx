module;

#include <stdafx.h>
#include <stdexcept>
#include <mutex>
#include "FixSettings.hpp"

export module Camera;

import ComVars;
import Cinematics;
import CameraContracts;
import CameraFrame;

export struct CameraDebugState
{
    bool available = false, valid = false, gameplay = false, cinematic = false;
    bool sampled = false;
    uint64_t calls = 0;
    uintptr_t owner = 0;
    int kind = -1, mode = -1;
    float target = 0, current = 0, factor = 1;
};

namespace
{
    struct Context
    {
        uintptr_t viewport = 0;
        uintptr_t controller = 0;
        ULONGLONG updated = 0;
        bool gameplay = false;
        bool cinematic = false;
        float originalTarget = 0.0f;
        int kind = -1, mode = -1;
        float current = 0.0f;
    };
    std::array<Context, 8> Contexts;
    std::mutex ContextMutex;
    uintptr_t(__cdecl* GetEngine)() = nullptr;
    uintptr_t(__thiscall* GetActorCamera)(uintptr_t) = nullptr;
    SafetyHookInline SceneOne, SceneTwo, SceneTwoNearFar, SceneOneNearFar;
    SafetyHookMid TargetHook;
    const float* Factor = nullptr;
    CameraDebugState LastSample;
    uint64_t TargetCalls = 0;

    float Multiplier()
    {
        return std::isfinite(*Factor) ? std::clamp(*Factor, 0.5f, 2.0f) : 1.0f;
    }

    uintptr_t Viewport(uintptr_t controller, uintptr_t wanted = 0, uintptr_t cameraActor = 0)
    {
        const auto engine = GetEngine ? GetEngine() : 0;
        const auto client = engine ? *reinterpret_cast<const uintptr_t*>(engine + 0x44) : 0;
        const auto count = client ? *reinterpret_cast<const int32_t*>(client + 0x30) : 0;
        const auto views = client ? *reinterpret_cast<const uintptr_t* const*>(client + 0x2C) : nullptr;
        if ((!controller && !cameraActor) || !views || count <= 0 || count > int(Contexts.size()))
            return 0;
        for (int32_t i = 0; i < count; ++i)
        {
            const auto viewport = views[i];
            if (!viewport || (wanted && wanted != viewport))
                continue;
            const auto actor = *reinterpret_cast<const uintptr_t*>(viewport + 0x30);
            if (cameraActor)
            {
                if (actor && GetActorCamera && (*reinterpret_cast<const uint32_t*>(actor + 0x60) & (1u << 18)) && GetActorCamera(actor) == cameraActor)
                    return viewport;
            }
            else if (actor == controller)
                return viewport;
        }
        return 0;
    }

    float Aspect(uintptr_t viewport)
    {
        const auto width = *reinterpret_cast<const int32_t*>(viewport + 0x4A0);
        const auto height = *reinterpret_cast<const int32_t*>(viewport + 0x4A4);
        return width > 0 && height > 0 ? float(width) / height : CameraFrame::Reference;
    }

    void Target(SafetyHookContext& regs)
    {
        const auto camera = regs.esi;
        const auto owner = *reinterpret_cast<const uintptr_t*>(camera + 0x1C);
        const auto viewport = Viewport(0, 0, owner);
        const auto controller = viewport ? *reinterpret_cast<const uintptr_t*>(viewport + 0x30) : 0;
        const auto entity = *reinterpret_cast<const uintptr_t*>(camera + 0x2C);
        const int kind = entity ? *reinterpret_cast<const int*>(entity + 8) : -1;
        const int mode = entity ? *reinterpret_cast<const int*>(entity + 12) : -1;
        const bool cinematic = CameraFrame::Cinematic(kind, mode) || (controller && (*reinterpret_cast<const uint32_t*>(controller + 0x3A4) & 4));
        const bool gameplay = !cinematic && CameraFrame::Gameplay(kind, mode);
        auto& angle = *reinterpret_cast<float*>(regs.ebp - 4);
        const float originalTarget = angle;
        const auto now = GetTickCount64();
        {
            std::lock_guard lock(ContextMutex);
            LastSample.sampled = true;
            LastSample.calls = ++TargetCalls;
            LastSample.owner = owner;
            LastSample.kind = kind;
            LastSample.mode = mode;
            LastSample.target = originalTarget;
            LastSample.current = *reinterpret_cast<const float*>(camera + 0x34);
            if (!viewport || !entity)
                return;
            auto selected = std::find_if(Contexts.begin(), Contexts.end(), [&](const Context& context) { return context.viewport == viewport; });
            if (selected == Contexts.end())
                selected = std::min_element(Contexts.begin(), Contexts.end(), [](const Context& a, const Context& b) { return a.updated < b.updated; });
            *selected = { viewport, controller, now, gameplay, cinematic, originalTarget, kind, mode,
                *reinterpret_cast<const float*>(camera + 0x34) };
        }
        if (!gameplay)
            return;
        const auto aspect = Aspect(viewport);
        angle = CameraFrame::Target(angle, Multiplier(), aspect);
    }

    Context Current(uintptr_t viewport, uintptr_t controller)
    {
        std::lock_guard lock(ContextMutex);
        for (const auto& context : Contexts)
        {
            if (context.viewport == viewport && context.controller == controller && context.updated)
                return context;
        }
        return {};
    }

    bool Cinematic(uintptr_t viewport)
    {
        if (!viewport)
            return false;
        const auto controller = *reinterpret_cast<const uintptr_t*>(viewport + 0x30);
        return Viewport(controller, viewport) &&
            (Current(viewport, controller).cinematic || (*reinterpret_cast<const uint32_t*>(controller + 0x3A4) & 4) != 0);
    }

    float Forward(uintptr_t viewport, float angle, bool updateCull)
    {
        if (!viewport)
            return angle;
        const auto controller = *reinterpret_cast<const uintptr_t*>(viewport + 0x30);
        if (!Viewport(controller, viewport))
            return angle;
        const auto aspect = Aspect(viewport);
        const bool nativeCorrection = *reinterpret_cast<const int32_t*>(viewport + 0x520) != 0;
        const auto context = Current(viewport, controller);
        const bool limit = context.gameplay;
        const auto result = Cinematics::Active() ?
            Cinematics::Angle(angle, aspect, nativeCorrection) :
            CameraFrame::SceneInput(angle, aspect, nativeCorrection, true, limit);
        if (updateCull && UpdateCameraCullBias && CameraFrame::Valid(angle, aspect))
        {
            const auto horizontal = nativeCorrection && aspect > CameraFrame::Reference ? CameraFrame::Horizontal(result, aspect) : result;
            const auto reference = CameraFrame::Valid(context.originalTarget, aspect) ? context.originalTarget : angle;
            UpdateCameraCullBias(std::max(1.0f, horizontal / reference));
        }
        return result;
    }

    uintptr_t __fastcall One(uintptr_t scene, void* edx, uintptr_t viewport, uintptr_t actor, uintptr_t position, uintptr_t rotation, float angle)
    {
        Cinematics::SceneScope shot(viewport, Cinematic(viewport));
        return SceneOne.fastcall<uintptr_t>(scene, edx, viewport, actor, position, rotation, Forward(viewport, angle, true));
    }

    uintptr_t __fastcall Two(uintptr_t scene, void* edx, uintptr_t viewport, uintptr_t actor, uintptr_t position, uintptr_t rotation, float angle, float secondary)
    {
        Cinematics::SceneScope shot(viewport, Cinematic(viewport));
        return SceneTwo.fastcall<uintptr_t>(scene, edx, viewport, actor, position, rotation,
            Forward(viewport, angle, true), Forward(viewport, secondary, false));
    }

    uintptr_t __fastcall TwoNearFar(uintptr_t scene, void* edx, uintptr_t viewport, uintptr_t actor, uintptr_t position, uintptr_t rotation, float angle, float secondary, float nearPlane, float farPlane)
    {
        Cinematics::SceneScope shot(viewport, Cinematic(viewport));
        return SceneTwoNearFar.fastcall<uintptr_t>(scene, edx, viewport, actor, position, rotation,
            Forward(viewport, angle, true), Forward(viewport, secondary, false), nearPlane, farPlane);
    }

    uintptr_t __fastcall OneNearFar(uintptr_t scene, void* edx, uintptr_t viewport, uintptr_t actor, uintptr_t position, uintptr_t rotation, float angle, float nearPlane, float farPlane)
    {
        Cinematics::SceneScope shot(viewport, Cinematic(viewport));
        return SceneOneNearFar.fastcall<uintptr_t>(scene, edx, viewport, actor, position, rotation,
            Forward(viewport, angle, true), nearPlane, farPlane);
    }
}

export CameraDebugState GetCameraDebugState()
{
    CameraDebugState state;
    state.available = bool(TargetHook) && bool(SceneOne) && bool(SceneTwo) && bool(SceneTwoNearFar) && bool(SceneOneNearFar);
    state.factor = Factor ? Multiplier() : 1.0f;
    std::lock_guard lock(ContextMutex);
    state.sampled = LastSample.sampled;
    state.calls = LastSample.calls;
    state.owner = LastSample.owner;
    state.kind = LastSample.kind;
    state.mode = LastSample.mode;
    state.target = LastSample.target;
    state.current = LastSample.current;
    const Context* selected = nullptr;
    for (const auto& context : Contexts)
    {
        if (context.updated && (!selected || context.viewport == GameViewport || context.updated > selected->updated))
        {
            selected = &context;
            if (context.viewport == GameViewport)
                break;
        }
    }
    if (selected)
    {
        state.valid = true;
        state.gameplay = selected->gameplay;
        state.cinematic = selected->cinematic;
        state.kind = selected->kind;
        state.mode = selected->mode;
        state.target = selected->originalTarget;
        state.current = selected->current;
    }
    return state;
}

export void InitCamera()
{
    auto target = hook::pattern("F3 0F 10 46 34 0F 57 E4 0F 2E C4 D9 5D FC 9F F6 C4 44");
    auto engine = hook::pattern("83 3D ? ? ? ? 00 75 09 A1 ? ? ? ? 85 C0 75 05 A1 ? ? ? ? C3");
    auto actorCamera = hook::pattern("80 79 4D 02 75 03 33 C0 C3 8B 81 A4 06 00 00 C3");
    // The DX11 and DX9 executables differ only in the helper call inside each constructor.
    const auto& contracts = hook::pattern(CameraContracts::DX11[0].pattern).empty() ? CameraContracts::DX9 : CameraContracts::DX11;
    std::array<void*, 4> constructors{};
    for (size_t i = 0; i < constructors.size(); ++i)
        constructors[i] = hook::pattern(contracts[i].pattern).get_first();
    GetEngine = reinterpret_cast<decltype(GetEngine)>(engine.get_first());
    GetActorCamera = reinterpret_cast<decltype(GetActorCamera)>(actorCamera.get_first());
    Cinematics::SetEngine(GetEngine);
    Factor = &FixSettings().Float("MAIN.FOVFactor");
    SceneOne = safetyhook::create_inline(constructors[0], One);
    SceneTwo = safetyhook::create_inline(constructors[1], Two);
    SceneTwoNearFar = safetyhook::create_inline(constructors[2], TwoNearFar);
    SceneOneNearFar = safetyhook::create_inline(constructors[3], OneNearFar);
    TargetHook = safetyhook::create_mid(target.get_first(14), Target);
    if (!SceneOne || !SceneTwo || !SceneTwoNearFar || !SceneOneNearFar || !TargetHook)
    {
        TargetHook.reset();
        SceneOne.reset();
        SceneTwo.reset();
        SceneTwoNearFar.reset();
        SceneOneNearFar.reset();
        throw std::runtime_error("Camera hook installation failed");
    }
}

module;

#include <stdafx.h>

export module Camera;

import ComVars;
import Cutscene;

export namespace Camera
{
    const float* FOVFactor;

    void Init(const float& factor)
    {
        FOVFactor = &factor;
        // The world projection builder also serves portrait, frontend and debug views; only the player camera's
        // frame setup and the restore after the shadow passes are widened. Another function shares the builder's
        // prologue, so the builder is taken from those calls.
        auto cameraProjection = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 50 6A 00 68 00 00 7A 44 51 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ?");
        auto restoreProjection = hook::pattern("8B 15 ? ? ? ? 8B 82 00 02 00 00 8B 15 ? ? ? ? 51 6A 00 68 00 00 7A 44 52 8D 48 1C 51 83 C0 28 50 E8 ? ? ? ?");
        const auto builder = injector::GetBranchDestination(cameraProjection.get_first(30), true).as_int();
        auto projection = hook::range_pattern(builder, builder + 10, "A0 ? ? ? ? 83 EC 50 84 C0").get_first<uint8_t>();
        {
            static const auto cameraReturn = reinterpret_cast<uintptr_t>(cameraProjection.get_first(35));
            static const auto restoreReturn = reinterpret_cast<uintptr_t>(restoreProjection.get_first(40));
            byte_9132CC.SetAddress(*reinterpret_cast<uint8_t**>(projection + 1));
            struct FOVHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.eax = byte_9132CC.get();
                    const auto caller = *reinterpret_cast<uintptr_t*>(regs.esp);
                    if (caller != cameraReturn && caller != restoreReturn)
                        return;
                    auto& fov = *(float*)(regs.esp + 0x0C);
                    if (std::isfinite(fov) && fov > 0.0f && fov < 180.0f)
                    {
                        const bool unframedCutscene = Cutscene::Active && Cutscene::Active() && !Cutscene::Bars();
                        const float factor = !unframedCutscene && std::isfinite(*FOVFactor) ? std::clamp(*FOVFactor, 0.5f, 2.0f) : 1.0f;
                        float corrected = std::min(AdjustFOV(fov, Screen.fAspectRatio) * factor, 150.0f);
                        // Cutscenes keep their own framing; the multiplier fades out while the bars slide in.
                        const float cutscene = Cutscene::Amount();
                        if (cutscene > 0.0f)
                            corrected += (Cutscene::FieldOfView(fov, Screen.fAspectRatio) - corrected) * cutscene;
                        if (std::isfinite(corrected) && corrected > 0.0f)
                            fov = std::min(corrected, 150.0f);
                    }
                }
            }; injector::MakeInline<FOVHook>(projection);
        }

    }
}

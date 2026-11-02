module;

#include <stdafx.h>
#include <minidx8/d3d8.h>

export module Camera;

import ComVars;
import Hud;

export namespace GameplayFov
{
    inline std::atomic<float> Factor{ 1.0f };
    inline std::atomic<bool> ProjectionAdjusted{ false };
    inline std::atomic<bool> RefreshPending{ true };
    inline uintptr_t(__cdecl* CurrentCamera)() = nullptr;
    inline void(__cdecl* MarkDirty)(uintptr_t) = nullptr;
    inline bool(__cdecl* SetTransform)(int, const D3DMATRIX*) = nullptr;

    bool __cdecl SetProjection(int type, const D3DMATRIX* matrix)
    {
        const auto state = GameplayState.get();
        const auto camera = RenderCamera.get();
        const float factor = Factor.load(std::memory_order_relaxed);
        const bool gameplay = !SonicHud::Drawing && state && *reinterpret_cast<int32_t*>(state + 0x38) == 2 && camera &&
            camera == CurrentCamera() && *reinterpret_cast<int32_t*>(camera + 0x14) == 1;
        if (gameplay && RefreshPending.exchange(false, std::memory_order_relaxed))
        {
            const auto frame = *reinterpret_cast<uintptr_t*>(camera + 4);
            if (frame)
                MarkDirty(frame);
            else
                RefreshPending.store(true, std::memory_order_relaxed);
        }
        if (gameplay && matrix && factor != 1.0f)
        {
            D3DMATRIX adjusted = *matrix;
            adjusted._11 /= factor;
            adjusted._22 /= factor;
            adjusted._31 /= factor;
            adjusted._32 /= factor;
            adjusted._41 /= factor;
            adjusted._42 /= factor;
            const bool result = SetTransform(type, &adjusted);
            ProjectionAdjusted.store(true, std::memory_order_relaxed);
            return result;
        }
        const bool result = SetTransform(type, matrix);
        ProjectionAdjusted.store(false, std::memory_order_relaxed);
        return result;
    }
}

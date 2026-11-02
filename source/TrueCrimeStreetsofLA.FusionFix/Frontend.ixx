module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include <atomic>


export module Frontend;

import Window;
import Device;

export namespace TrueCrimeFrontend
{
    inline SafetyHookInline DrawHook, BatchHook;
    inline thread_local bool Drawing = false;
    inline std::atomic_bool Active{ false };

    struct Layout
    {
        float left, top, scaleX, scaleY;
    };

    inline Layout Fit()
    {
        const float width = static_cast<float>(TrueCrimeWindow::Width);
        const float height = static_cast<float>(TrueCrimeWindow::Height);
        const float scale = std::min(width / 640.0f, height / 480.0f);
        return { (width - 640.0f * scale) * 0.5f, (height - 480.0f * scale) * 0.5f,
            640.0f * scale / width, 480.0f * scale / height };
    }

    // 2D vertex batch: buffer at +16, write position at +20; the buffer's stride at +20 and vertex count at +44
    inline int __fastcall Batch(uintptr_t renderer, void*)
    {
        if (Drawing)
        {
            const auto buffer = *reinterpret_cast<uintptr_t*>(renderer + 16);
            const auto end = *reinterpret_cast<uintptr_t*>(renderer + 20);
            if (buffer && end)
            {
                const auto stride = *reinterpret_cast<uint32_t*>(buffer + 20);
                const auto count = *reinterpret_cast<uint32_t*>(buffer + 44);
                if (stride == 36 && count > 0 && count <= 65536 && end >= count * stride)
                {
                    // x, y, z, rhw, color, two texture coordinates
                    auto vertices = reinterpret_cast<float*>(end - count * stride);
                    bool black = true;
                    const float width = static_cast<float>(TrueCrimeWindow::Width);
                    const float height = static_cast<float>(TrueCrimeWindow::Height);
                    float minX = width, minY = height, maxX = 0.0f, maxY = 0.0f;
                    for (uint32_t i = 0; i < count; ++i)
                    {
                        const float* vertex = vertices + i * 9;
                        black &= (*reinterpret_cast<const uint32_t*>(vertex + 4) & 0xFFFFFF) == 0;
                        minX = std::min(minX, vertex[0]); maxX = std::max(maxX, vertex[0]);
                        minY = std::min(minY, vertex[1]); maxY = std::max(maxY, vertex[1]);
                    }
                    // Full-screen fades stay full screen
                    const bool fade = black && minX <= 0.0f && minY <= 0.0f && maxX >= width - 1 && maxY >= height - 1;
                    if (!fade)
                    {
                        const auto fit = Fit();
                        for (uint32_t i = 0; i < count; ++i)
                        {
                            float* vertex = vertices + i * 9;
                            vertex[0] = (vertex[0] + 0.5f) * fit.scaleX + fit.left - 0.5f;
                            vertex[1] = (vertex[1] + 0.5f) * fit.scaleY + fit.top - 0.5f;
                        }
                    }
                }
            }
        }
        return BatchHook.thiscall<int>(renderer);
    }

    inline int __fastcall Draw(uintptr_t manager, void*, char flags)
    {
        Active.store(manager && *reinterpret_cast<uintptr_t*>(manager), std::memory_order_relaxed);
        const auto fit = Fit();
        if (auto device = TrueCrimeDevice::Device)
        {
            D3DRECT rectangles[4]{};
            DWORD count = 0;
            const LONG width = TrueCrimeWindow::Width;
            const LONG height = TrueCrimeWindow::Height;
            const LONG left = static_cast<LONG>(std::lround(fit.left));
            const LONG top = static_cast<LONG>(std::lround(fit.top));
            if (left > 0)
            {
                rectangles[count++] = { 0, 0, left, height };
                rectangles[count++] = { width - left, 0, width, height };
            }
            if (top > 0)
            {
                rectangles[count++] = { 0, 0, width, top };
                rectangles[count++] = { 0, height - top, width, height };
            }
            if (count && Active.load(std::memory_order_relaxed))
                device->Clear(count, rectangles, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
        }
        // Effects that slide in from outside the 640x480 area stay inside the box.
        auto device = Active.load(std::memory_order_relaxed) ? TrueCrimeDevice::Device : nullptr;
        RECT scissor{};
        DWORD scissorTest = FALSE;
        if (device)
        {
            device->GetScissorRect(&scissor);
            device->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissorTest);
            const RECT box{ static_cast<LONG>(std::lround(fit.left)), static_cast<LONG>(std::lround(fit.top)),
                static_cast<LONG>(std::lround(fit.left + fit.scaleX * TrueCrimeWindow::Width)),
                static_cast<LONG>(std::lround(fit.top + fit.scaleY * TrueCrimeWindow::Height)) };
            device->SetScissorRect(&box);
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
        }
        Drawing = true;
        const int result = DrawHook.thiscall<int>(manager, flags);
        Drawing = false;
        if (device)
        {
            device->SetScissorRect(&scissor);
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, scissorTest);
        }
        return result;
    }

    inline LPARAM Mouse(LPARAM position)
    {
        if (!Active.load(std::memory_order_relaxed))
            return position;
        const auto fit = Fit();
        const auto x = static_cast<int>(std::lround((static_cast<short>(LOWORD(position)) - fit.left) / fit.scaleX));
        const auto y = static_cast<int>(std::lround((static_cast<short>(HIWORD(position)) - fit.top) / fit.scaleY));
        return MAKELPARAM(static_cast<short>(x), static_cast<short>(y));
    }

    inline void Init()
    {
        auto draw = hook::pattern("55 8B EC 83 E4 F0 81 EC 94 00 00 00 53 56 57 8B F9 89 7C 24 5C E8");
        if (draw.empty()) // TrueCrimeMB.exe
            draw = hook::pattern("55 8B EC 83 E4 F0 83 EC 74 53 56 57 8B F9 89 7C 24 ? E8");
        auto batch = hook::pattern("53 55 56 57 8B F9 8B 47 18 83 F8 01 8B 4F 10 8B 59 20");
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
        BatchHook = safetyhook::create_inline(batch.get_first(), Batch);
        TrueCrimeWindow::MapMouse = Mouse;
    }
}

module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include <atomic>
#include <optional>


export module Frontend;

import ComVars;
import Window;
import Device;
import Hud;

export namespace TrueCrimeFrontend
{
    // The front end (title, menus, pause menu and their screens) is laid out in pixels for 640x480, with elements at
    // fixed distances from the screen width and from the HUD bounds. While it opens screens, updates and draws, the
    // game's screen size getters report 640x480 and the bounds keep their margins on that screen. Its 2D vertices go
    // into a centred 4:3 box scaled to the screen, so menus keep their 640x480 proportions at any resolution.
    inline SafetyHookInline DrawHook, BatchHook, OpenHook, UpdateHook, DisplayHook, ViewerHook;
    inline thread_local int Depth = 0;
    inline std::atomic_bool Active{ false };

    struct Layout
    {
        float left, top, scale;
    };

    inline Layout Fit()
    {
        const float width = static_cast<float>(TrueCrimeWindow::Width);
        const float height = static_cast<float>(TrueCrimeWindow::Height);
        const float scale = std::min(width / 640.0f, height / 480.0f);
        return { (width - 640.0f * scale) * 0.5f, (height - 480.0f * scale) * 0.5f, scale };
    }

    struct Scope
    {
        std::optional<TrueCrimeHud::FixedScreen> screen;
        int right = 0, bottom = 0, virtualRight = 0, virtualBottom = 0;

        Scope()
        {
            if (Depth++)
                return;
            const auto fit = Fit();
            const int realWidth = TrueCrimeHud::WidthHook.ccall<int>();
            const int realHeight = TrueCrimeHud::HeightHook.ccall<int>();
            screen.emplace(640, 480, fit.scale, fit.left, fit.top);
            right = HudRightBound.get();
            bottom = HudBottomBound.get();
            virtualRight = 640 - (realWidth - right);
            virtualBottom = 480 - (realHeight - bottom);
            HudRightBound = virtualRight;
            HudBottomBound = virtualBottom;
        }

        ~Scope()
        {
            if (--Depth)
                return;
            // A display mode change inside has set the bounds for the new screen
            if (HudRightBound.get() == virtualRight && HudBottomBound.get() == virtualBottom)
            {
                HudRightBound = right;
                HudBottomBound = bottom;
            }
        }

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
    };

    // Black quads over the whole 640x480 screen (fades) cover the whole real screen; x, y, z, rhw, color, two texture
    // coordinates
    inline bool Fade(float* vertices, uint32_t count)
    {
        bool black = true;
        float minX = 640.0f, minY = 480.0f, maxX = 0.0f, maxY = 0.0f;
        for (uint32_t i = 0; i < count; ++i)
        {
            const float* vertex = vertices + i * 9;
            black &= (*reinterpret_cast<const uint32_t*>(vertex + 4) & 0xFFFFFF) == 0;
            minX = std::min(minX, vertex[0]); maxX = std::max(maxX, vertex[0]);
            minY = std::min(minY, vertex[1]); maxY = std::max(maxY, vertex[1]);
        }
        if (!black || minX > 0.0f || minY > 0.0f || maxX < 639.0f || maxY < 479.0f)
            return false;
        const float scaleX = static_cast<float>(TrueCrimeWindow::Width) / 640.0f;
        const float scaleY = static_cast<float>(TrueCrimeWindow::Height) / 480.0f;
        for (uint32_t i = 0; i < count; ++i)
        {
            float* vertex = vertices + i * 9;
            vertex[0] = (vertex[0] + 0.5f) * scaleX - 0.5f;
            vertex[1] = (vertex[1] + 0.5f) * scaleY - 0.5f;
        }
        return true;
    }

    // 2D vertex batch: buffer at +16, write position at +20; the buffer's stride at +20 and vertex count at +44
    inline int __fastcall Batch(uintptr_t renderer, void*)
    {
        if (TrueCrimeHud::Scaling())
        {
            const auto buffer = *reinterpret_cast<uintptr_t*>(renderer + 16);
            const auto end = *reinterpret_cast<uintptr_t*>(renderer + 20);
            if (buffer && end)
            {
                const auto stride = *reinterpret_cast<uint32_t*>(buffer + 20);
                const auto count = *reinterpret_cast<uint32_t*>(buffer + 44);
                if (stride >= 16 && count > 0 && count <= 65536 && end >= count * stride)
                {
                    auto vertices = reinterpret_cast<float*>(end - count * stride);
                    if (!(Depth && stride == 36 && Fade(vertices, count)))
                        TrueCrimeHud::Transform(vertices, count, stride);
                }
            }
        }
        return BatchHook.thiscall<int>(renderer);
    }

    inline int __fastcall Draw(uintptr_t manager, void*, char flags)
    {
        Active.store(manager && *reinterpret_cast<uintptr_t*>(manager), std::memory_order_relaxed);
        const auto fit = Fit();
        auto device = Active.load(std::memory_order_relaxed) ? TrueCrimeDevice::Device : nullptr;
        RECT scissor{};
        DWORD scissorTest = FALSE;
        if (device)
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
            if (count)
                device->Clear(count, rectangles, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
            // Effects that slide in from outside the 640x480 area stay inside the box.
            device->GetScissorRect(&scissor);
            device->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissorTest);
            const RECT box{ left, top, static_cast<LONG>(std::lround(fit.left + 640.0f * fit.scale)),
                static_cast<LONG>(std::lround(fit.top + 480.0f * fit.scale)) };
            device->SetScissorRect(&box);
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
        }
        int result = 0;
        {
            Scope scope;
            result = DrawHook.thiscall<int>(manager, flags);
        }
        if (device)
        {
            device->SetScissorRect(&scissor);
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, scissorTest);
        }
        return result;
    }

    // Opens a front end screen, creating the front end first when needed
    inline int __cdecl Open(int screen, int option)
    {
        Scope scope;
        return OpenHook.ccall<int>(screen, option);
    }

    inline int __fastcall Update(uintptr_t manager, void*)
    {
        Scope scope;
        return UpdateHook.thiscall<int>(manager);
    }

    // Display options: applying a mode resets the device and sets the 3D clipping and the HUD bounds for the new screen
    inline int __fastcall Display(uintptr_t menu, void*, char apply)
    {
        TrueCrimeHud::RealScreen screen;
        return DisplayHook.thiscall<int>(menu, apply);
    }

    // Model viewer: a 3D view on the whole screen
    inline int __fastcall Viewer(uintptr_t viewer, void*)
    {
        TrueCrimeHud::RealScreen screen;
        return ViewerHook.thiscall<int>(viewer);
    }

    // The cursor's window position on the 640x480 screen
    inline LPARAM Mouse(LPARAM position)
    {
        if (!Active.load(std::memory_order_relaxed))
            return position;
        const auto fit = Fit();
        const auto x = static_cast<int>(std::lround((static_cast<short>(LOWORD(position)) - fit.left) / fit.scale));
        const auto y = static_cast<int>(std::lround((static_cast<short>(HIWORD(position)) - fit.top) / fit.scale));
        return MAKELPARAM(static_cast<short>(x), static_cast<short>(y));
    }

    inline void Init()
    {
        auto draw = hook::pattern("55 8B EC 83 E4 F0 81 EC 94 00 00 00 53 56 57 8B F9 89 7C 24 5C E8");
        auto viewer = hook::pattern("55 8B EC 83 E4 F0 81 EC 74 01 00 00 53 56 57 8B F1 E8");
        const char* open = "A1 ? ? ? ? 85 C0 75 ? 68 C0 00 00 00 68 ? ? ? ? 6A 10 68 E0 01 00 00 E8";
        if (draw.empty()) // TrueCrimeMB.exe
        {
            draw = hook::pattern("55 8B EC 83 E4 F0 83 EC 74 53 56 57 8B F9 89 7C 24 ? E8");
            open = "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 A1 ? ? ? ? 85 C0 75 ? 68 C0 00 00 00";
            viewer = hook::pattern("55 8B EC 83 E4 F0 81 EC 84 01 00 00 A1 ? ? ? ? 53 56 57 89 84 24");
        }
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
        BatchHook = safetyhook::create_inline(hook::pattern("53 55 56 57 8B F9 8B 47 18 83 F8 01 8B 4F 10 8B 59 20").get_first(), Batch);
        OpenHook = safetyhook::create_inline(hook::pattern(open).get_first(), Open);
        UpdateHook = safetyhook::create_inline(hook::pattern("51 56 8B F1 E8 ? ? ? ? 85 C0 89 44 24 04 DB 44 24 04 7D 06 D8 05").get_first(), Update);
        DisplayHook = safetyhook::create_inline(hook::pattern("8A 44 24 04 84 C0 56 57 8B F1 0F 84").get_first(), Display);
        ViewerHook = safetyhook::create_inline(viewer.get_first(), Viewer);
        TrueCrimeWindow::MapMouse = Mouse;
    }
}

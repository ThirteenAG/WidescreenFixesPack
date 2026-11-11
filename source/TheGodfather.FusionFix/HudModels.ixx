module;

#include <stdafx.h>
#include "HudLayout.hpp"
#include "FloatingPoint.hpp"
#include <d3d9.h>
#include <stdexcept>

// The 3D models in the HUD (weapon picture) are drawn after the HUD tree with their own camera. The game fits that
// camera to the screen width, so the models grow with the screen and keep their 4:3 fractions of the width. They are
// drawn here in a 4:3 area at their HUD group's place instead.
export module HudModels;

import Window;
import HudAspect;

namespace GodfatherHudModels
{
    SafetyHookInline drawHook;
    int(__cdecl* screenSize)(int*, int){};
    void*(__cdecl* setViewWindow)(void*, const float*){};
    void*(__cdecl* beginUpdate)(void*){};

    bool TargetSize(UINT& width, UINT& height)
    {
        width = UINT(std::max(GodfatherWindow::width, 0));
        height = UINT(std::max(GodfatherWindow::height, 0));
        return GodfatherWindow::device && width && height;
    }

    bool Wide(UINT width, UINT height)
    {
        return uint64_t(width) * 3 > uint64_t(height) * 4;
    }

    // The models' vertical placement uses the display's height-to-width ratio; a 4:3 area needs 3:4.
    int __cdecl ScreenSize(int* size, int mode)
    {
        const auto result = screenSize(size, mode);
        if (result && size[0] > 0 && size[1] > 0 && Wide(UINT(size[0]), UINT(size[1])))
            size[0] = int(std::lround(size[1] * (4.0 / 3.0)));
        return result;
    }

    // Begins the model camera with the original 60 degree lens over a 4:3 area. The camera is made once at the
    // starting resolution; its frame and depth buffer sizes follow the screen like the game's other screen cameras,
    // else it sets a viewport larger than the back buffer after a resolution change.
    void* __cdecl BeginModels(void* camera)
    {
        UINT width{}, height{};
        if (camera && TargetSize(width, height))
        {
            for (const auto offset : { 96u, 100u })
            {
                const auto raster = *reinterpret_cast<char**>(static_cast<char*>(camera) + offset);
                if (!raster)
                    continue;
                *reinterpret_cast<int*>(raster + 12) = int(width);
                *reinterpret_cast<int*>(raster + 16) = int(height);
            }
            Godfather::FloatingPointScope floatingPoint;
            const auto across = std::tan(0.5235988f);
            const float window[2] = { across, across * (Wide(width, height) ? 0.75f : float(height) / float(width)) };
            setViewWindow(camera, window);
        }
        return beginUpdate(camera);
    }

    void __fastcall Draw(void* node, void*)
    {
        const auto device = GodfatherWindow::device;
        D3DVIEWPORT9 original{};
        UINT width{}, height{};
        if (!device || !TargetSize(width, height) || !Wide(width, height) || FAILED(device->GetViewport(&original)))
        {
            drawHook.unsafe_thiscall<void>(node);
            return;
        }
        Godfather::FloatingPointScope floatingPoint;
        const auto area = float(height) * (4.0f / 3.0f);
        const auto offset = GodfatherHud::Offset(float(width), float(height),
            GodfatherHud::constraint.load(std::memory_order_relaxed), GodfatherHud::ModelAnchor(node));
        const auto left = std::clamp((float(width) - area) * 0.5f + offset, 0.0f, float(width) - area);
        auto viewport = original;
        viewport.X = DWORD(std::lround(left));
        viewport.Y = 0;
        viewport.Width = std::min(DWORD(std::lround(area)), DWORD(width) - viewport.X);
        viewport.Height = height;
        device->SetViewport(&viewport);
        drawHook.unsafe_thiscall<void>(node);
        device->SetViewport(&original);
    }

    export void Init()
    {
        auto draw = hook::pattern("56 8B F1 8B 46 50 85 C0 74 69 8B 46 54 57 33 FF 85 C0 76 5E 8B 46 50 8B 0C B8 6A 00 68");
        auto begin = hook::pattern("8B 4E 6C 85 C9 0F 8E ? ? ? ? 50 E8 ? ? ? ? 8B 56 0C 52 E8");
        auto window = hook::pattern("8B 44 24 08 D9 05 ? ? ? ? 8B 08 56 8B 74 24 08 89 4E 68 D8 76 68 8B 50 04 89 56 6C");
        auto animate = hook::pattern("89 4E 34 89 56 3C E8 ? ? ? ? 50 8D 44 24 ? 50 E8 ? ? ? ? DB 44 24");
        auto place = hook::pattern("5D E8 ? ? ? ? 50 8D 54 24 ? 52 E8 ? ? ? ? DB 44 24 ? 8B 46 54");
        drawHook = safetyhook::create_inline(draw.get_first(), Draw);
        setViewWindow = reinterpret_cast<decltype(setViewWindow)>(window.get_first());
        beginUpdate = reinterpret_cast<decltype(beginUpdate)>(injector::GetBranchDestination(begin.get_first(12)).as_int());
        screenSize = reinterpret_cast<decltype(screenSize)>(injector::GetBranchDestination(place.get_first(12)).as_int());
        injector::MakeCALL(begin.get_first(12), BeginModels, true);
        injector::MakeCALL(animate.get_first(17), ScreenSize, true);
        injector::MakeCALL(place.get_first(12), ScreenSize, true);
    }
}

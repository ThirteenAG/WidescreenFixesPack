module;
#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <map>

export module Cinematics;
import Camera;
import CameraMath;
import FloatingPoint;
import Window;
import NativeMenu;


export namespace PsiOpsCinematics
{
    inline std::map<PsiOpsMenu::Node, bool> nativeBars;
    inline std::atomic<bool> animated{ true };

    inline bool Capture(void* self, PsiOpsMenu::Node node, void* color)
    {
        const auto found = nativeBars.find(node);
        if (found == nativeBars.end() || nativeBars.size() != 2 || !PsiOpsCamera::projectionHook)
            return false;
        if (!(node[3] & 1) || !(PsiOpsCamera::borders.load() & 1))
            return true;
        PsiOps::FloatingPointScope floatingPoint;
        const auto transform = PsiOpsMenu::Pointer(node[5]);
        if (!transform)
            return false;
        const auto position = *(reinterpret_cast<float*>(transform) + 7);
        const auto offset = *(reinterpret_cast<float*>(static_cast<uint8_t*>(self) + 20));
        const auto height = animated ? PsiOpsCamera::BarHeight(position, offset, found->second)
            : PsiOpsCamera::Cinematic() ? 60.0f : 0.0f;
        if (height <= 0.0f)
            return true;
        if (!PsiOpsWindow::device || !color
            || *(reinterpret_cast<float*>(color) + 5) != 1.0f
            || *(reinterpret_cast<float*>(transform) + 15) != 1.0f)
            return false;
        IDirect3DSurface9* surface = nullptr;
        if (FAILED(PsiOpsWindow::device->GetRenderTarget(0, &surface)))
            return false;
        D3DSURFACE_DESC description{};
        bool drawn = false;
        if (SUCCEEDED(surface->GetDesc(&description)))
        {
            const auto width = LONG(description.Width), targetHeight = LONG(description.Height);
            const auto framing = PsiOpsCamera::Frame(width, targetHeight, PsiOpsCamera::borders);
            const auto outside = float(targetHeight) * (1.0f - 1.0f / framing.y) * 0.5f;
            const auto pixels = LONG(std::ceil(outside + height * float(targetHeight) / framing.y / 465.0f));
            const RECT rectangle = found->second ? RECT{ 0, 0, width, pixels }
                : RECT{ 0, targetHeight - pixels, width, targetHeight };
            drawn = SUCCEEDED(PsiOpsWindow::device->ColorFill(surface, &rectangle, D3DCOLOR_XRGB(0, 0, 0)));
        }
        surface->Release();
        return drawn;
    }

    inline void Draw(IDirect3DDevice9* device)
    {
        const bool cinematic = PsiOpsCamera::Cinematic() && PsiOpsCamera::worldCamera.load();
        const auto mode = PsiOpsCamera::borders.load();
        if (!cinematic || !mode)
            return;
        IDirect3DSurface9* surface = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
            return;
        D3DSURFACE_DESC description{};
        if (SUCCEEDED(surface->GetDesc(&description)))
        {
            const auto width = LONG(description.Width), height = LONG(description.Height);
            const auto framing = PsiOpsCamera::Frame(width, height, mode);
            const auto horizontal = LONG(std::ceil(float(width) * (1.0f - 1.0f / framing.x) * 0.5f));
            const auto vertical = LONG(std::ceil(float(height) * (1.0f - 1.0f / framing.y) * 0.5f));
            const RECT rectangles[]{ { 0, 0, horizontal, height }, { width - horizontal, 0, width, height },
                { 0, 0, width, vertical }, { 0, height - vertical, width, height } };
            for (const auto& rectangle : rectangles)
                if (rectangle.right > rectangle.left && rectangle.bottom > rectangle.top)
                    device->ColorFill(surface, &rectangle, D3DCOLOR_XRGB(0, 0, 0));
        }
        surface->Release();
    }

    inline void Init(bool nativeAvailable)
    {
        const auto refresh = []() { animated = FixSettings().Bool("MAIN.AnimatedCutsceneBorders"); };
        refresh();
        FixSettings().OnChange("MAIN.AnimatedCutsceneBorders", refresh);
        FixSettings().Availability("MAIN.AnimatedCutsceneBorders",
            [available = nativeAvailable && bool(PsiOpsCamera::projectionHook)](std::string_view) { return available; });
        const auto previous = PsiOpsWindow::frame;
        PsiOpsWindow::frame = [previous](IDirect3DDevice9* device)
        {
            Draw(device);
            if (previous) previous(device);
        };
    }
}


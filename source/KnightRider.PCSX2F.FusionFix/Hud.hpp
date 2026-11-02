#pragma once
#include "Draw.hpp"

// Menus, loading screens and the HUD are NetImmerse scenes seen through 4:3
// cameras (NIF files) plus screen textures and polygons in 4:3 pixels. The
// cameras are widened so their 4:3 frame sits in the middle of the screen.
// HUD views (CAppCameraPropertyStatic*) then move their elements to the
// screen or HUD box edges.
namespace kr::hud
{
    // Anchor of the HUD view being drawn: 0 none (menus, centered views),
    // -1/1 the whole view sits at the left/right edge, 2 elements are sorted
    // into left, middle and right by position, 3 the cockpit dashboard.
    inline int view;
    // The KittInfo scene is being drawn region by region (see Scene).
    inline bool split;
    // CAppCameraPropertyStaticSubtitles is being drawn: its camera has no
    // scene, only the black bands of scripted scenes and the text.
    inline bool subtitles;
#ifdef WFP_PS2_DEBUG
    inline void (*trace2D)(uint32_t, uint32_t, float, float, float, float);
#endif
    // Screen textures and polygons keep their GS packets until their content
    // changes; after a layout change they are rebuilt for a short while.
    inline uint32_t rebuild2D;

    inline int Classify(float center)
    {
        return center < 0.35f ? -1 : center > 0.65f ? 1 : 0;
    }
    inline int ElementAnchor(float center)
    {
        if (view == 1 || view == -1)
        {
            return view;
        }
        return view == 2 ? Classify(center) : 0;
    }

    using Render = safetymips::GameInline<void(uint32_t, uint32_t, uint32_t)>;
    inline Render eventView, timeView, mapView, inspectView, subtitlesView, targetView, kittInfo;
    template <Render& Original, int Anchor> void Draw(uint32_t self, uint32_t frame, uint32_t pass)
    {
        const int previous = view;
        view = Anchor;
        Original.call(self, frame, pass);
        view = previous;
    }
    inline void Subtitles(uint32_t self, uint32_t frame, uint32_t pass)
    {
        subtitles = true;
        Draw<subtitlesView, -1>(self, frame, pass);
        subtitles = false;
    }
    // The bands above and below scripted scenes, without Letterbox.
    inline bool HiddenBand(float left, float right, float top, float bottom)
    {
        return subtitles && Cinematic() && !(cutsceneBorders & 1) && left <= 0.01f && right >= 0.99f &&
               bottom - top < 0.3f && (top <= 0.01f || bottom >= 0.99f);
    }

    // CAppCameraPropertyStaticKittInfo: one instance draws the info panels,
    // the other (+20 = 1) the cockpit dashboard.
    inline void KittInfo(uint32_t self, uint32_t frame, uint32_t pass)
    {
        const int previous = view;
        view = Read<uint8_t>(self + 20) == 1 ? 3 : 2;
        if (view == 3 && (pass & 0xFF) == 1)
        {
            cockpitFrame = kr::frame;
        }
        kittInfo.call(self, frame, pass);
        view = previous;
    }

    // NiCamera: +0xA0 world location, +0x154 frustum {left, right, top,
    // bottom, near, far}, +0x16C viewport {left, right, top, bottom}, +0x1B0
    // renderer. Renderer: +0x828/+0x82C frame size in pixels.
    inline safetymips::GameInline<void(uint32_t)> scene; // 0x129510: clear depth in the viewport, Click
    // KittInfo.nif holds both info panels in one mesh, so its elements are
    // moved region by region: each region is drawn with its own frustum part
    // and a viewport (scissor) that cuts off the rest.
    inline void Scene(uint32_t owner)
    {
        const auto camera = Read<uintptr_t>(owner + 0x10);
        const float k = Fit(), ky = VerticalFit();
        if (!camera || (view != 2 && view != 3) || (k == 1 && ky == 1))
        {
            return scene.call(owner);
        }
        const auto renderer = Read<uintptr_t>(camera + 0x1B0);
        auto frustum = reinterpret_cast<float*>(camera + 0x154);
        auto port = reinterpret_cast<float*>(camera + 0x16C);
        const float left = frustum[0], right = frustum[1];
        const float saved[4]{port[0], port[1], port[2], port[3]};
        const float shift = view == 3 ? 0 : HudShift();
        const float width = renderer ? float(Read<int32_t>(renderer + 0x828)) : 0;
        split = true;
        if (shift <= 0 || !(width > 0))
        {
            // The dashboard and its gauges keep the centered 4:3 frame. Its
            // mesh ends a little inside that frame: one pixel per side stays
            // black, like the world view behind it.
            const float inset = view == 3 && k < 1 && width > 0 ? 1.0f / width : 0;
            const float trim = (right - left) * inset / k;
            frustum[0] = left + trim;
            frustum[1] = right - trim;
            port[0] = 0.5f - 0.5f * k + inset;
            port[1] = 0.5f + 0.5f * k - inset;
            port[2] = 0.5f + 0.5f * ky;
            port[3] = 0.5f - 0.5f * ky;
            scene.call(owner);
            frustum[0] = left;
            frustum[1] = right;
        }
        else
        {
            // Region edges on whole pixels, so the scissors meet exactly.
            auto edge = [&](float x) { return (std::round(x * width) + 0.25f) / width; };
            auto part = [&](float x0, float x1, int side)
            {
                // Screen x of design u: 0.5 + (u - 0.5) * k + side * shift.
                const float u0 = 0.5f + (x0 - 0.5f - float(side) * shift) / k;
                const float u1 = 0.5f + (x1 - 0.5f - float(side) * shift) / k;
                frustum[0] = left + u0 * (right - left);
                frustum[1] = left + u1 * (right - left);
                port[0] = x0;
                port[1] = x1;
                port[2] = 0.5f + 0.5f * ky;
                port[3] = 0.5f - 0.5f * ky;
                scene.call(owner);
            };
            const float middle = 0.15f * k;
            part(edge(0.5f - middle), edge(0.5f + middle), 0);
            part(0, edge(0.5f - middle - shift), -1);
            part(edge(0.5f + middle + shift), 1, 1);
            frustum[0] = left;
            frustum[1] = right;
        }
        split = false;
        std::copy(saved, saved + 4, port);
    }

    // The 4:3 camera currently drawn widened (see CameraData).
    struct WideCamera
    {
        bool active;
        float stretch, halfWidth; // 1 / fit, original right/near ratio
        float location[3], direction[3], right[3];
    };
    inline WideCamera wide;

    inline float frustum[6];
    struct Restore
    {
        uintptr_t camera;
        float port[4];
    };
    inline Restore restore;
    inline void SetPort(uintptr_t camera, float left, float right, float top, float bottom)
    {
        auto port = reinterpret_cast<float*>(camera + 0x16C);
        restore.camera = camera;
        std::copy(port, port + 4, restore.port);
        port[0] = left;
        port[1] = right;
        port[2] = top;
        port[3] = bottom;
    }
    // NiCamera::Click epilogue.
    inline void ClickEnd()
    {
        wide.active = false;
        if (restore.camera)
        {
            std::copy(restore.port, restore.port + 4, reinterpret_cast<float*>(restore.camera + 0x16C));
            restore.camera = 0;
        }
    }

    // 0x3233BC in NiCamera::Click, before renderer->SetCameraData(..., t1 =
    // frustum, t2 = viewport); s2 = camera. Gameplay cameras are already set
    // up for the screen by the projection hooks; 4:3 frustums are widened on
    // a copy, so the camera objects keep their stored values.
    inline void CameraData(SafetyMipsContext& regs)
    {
        const float k = Fit(), ky = VerticalFit();
        if (split || (k == 1 && ky == 1))
        {
            return;
        }
        const auto camera = uintptr_t(regs.s2);
        const auto source = reinterpret_cast<const float*>(uintptr_t(regs.t1));
        const auto port = reinterpret_cast<const float*>(camera + 0x16C);
        const float width = source[1] - source[0], height = source[2] - source[3];
        if (!width || !height || !(source[4] > 0))
        {
            return;
        }
        if (port[0] > 0.001f || port[1] < 0.999f || port[2] < 0.999f || port[3] > 0.001f)
        {
            // Inset view (truck view, minimap, portraits): its frame belongs to
            // the HUD, so map the viewport into the 4:3 area and move it with
            // its HUD element. The frustum is left alone, so the view keeps its
            // proportions. t2 is set in the delay slot of the call, so the
            // viewport is changed in place and restored at the end of Click.
            const float offset = float(ElementAnchor(0.5f * (port[0] + port[1]))) * HudShift();
            SetPort(camera, 0.5f + (port[0] - 0.5f) * k + offset, 0.5f + (port[1] - 0.5f) * k + offset,
                    0.5f + (port[2] - 0.5f) * ky, 0.5f + (port[3] - 0.5f) * ky);
            return;
        }
        // 4:3 views and square normalized 2D views; other ratios (gameplay
        // cameras) are set up by the FOV helpers for the screen.
        const float ratio = std::fabs(width / height);
        if (std::fabs(ratio - 4.0f / 3.0f) > 0.01f && std::fabs(ratio - 1) > 0.01f)
        {
            return;
        }
        std::memcpy(frustum, source, sizeof(frustum));
        const float center = 0.5f * (source[0] + source[1]);
        // A side-anchored view moves by its share of the extra width.
        const float offset = view == 1 || view == -1 ? float(view) * HudShift() * width / k : 0;
        frustum[0] = center + (source[0] - center) / k - offset;
        frustum[1] = center + (source[1] - center) / k - offset;
        const float centerY = 0.5f * (source[2] + source[3]);
        frustum[2] = centerY + (source[2] - centerY) / ky;
        frustum[3] = centerY + (source[3] - centerY) / ky;
        regs.t1 = uint32_t(uintptr_t(frustum));
        if (offset)
        {
            return;
        }
        // Camera world matrix rows: direction (local X), up, right (local Z), location.
        for (unsigned i = 0; i < 3; ++i)
        {
            wide.direction[i] = Read<float>(camera + 0x70 + 4 * i);
            wide.right[i] = Read<float>(camera + 0x90 + 4 * i);
            wide.location[i] = Read<float>(camera + 0xA0 + 4 * i);
        }
        wide.stretch = 1 / k;
        wide.halfWidth = 0.5f * std::fabs(width); // NiFrustum is at unit distance
        wide.active = true;
    }

    // Backgrounds, fades and bands that cover the whole 4:3 frustum are stretched
    // along the camera's right axis so they still cover the widened frustum.
    // 0x35F810 loads the object's model-to-world matrix (rows X, Y, Z, T as used
    // by VU0) from a1; called from the NiPS2Renderer geometry dispatchers with the
    // geometry data (model bound center at +0x10, radius at +0x1C) in s3.
    inline float stretched[16] __attribute__((aligned(16)));
    // The bound sphere test also accepts HUD meshes that pack several panels
    // into one geometry (both gameplay HUD boxes are one mesh with a centered
    // bound). Check that the vertices really reach both sides of the 4:3 view.
    // NiGeometryData: +8 vertex count (u16), +0x20 vertex array.
    inline bool SpansView(uintptr_t data, const float* m)
    {
        const unsigned count = Read<uint16_t>(data + 8);
        const auto vertices = Read<uintptr_t>(data + 0x20);
        if (!count || count > 4096 || !vertices || vertices >= 0x02000000)
        {
            return true;
        }
        float side[4], depth[4];
        for (unsigned row = 0; row < 4; ++row)
        {
            side[row] = depth[row] = 0;
            for (unsigned i = 0; i < 3; ++i)
            {
                const float v = m[4 * row + i] - (row == 3 ? wide.location[i] : 0);
                side[row] += v * wide.right[i];
                depth[row] += v * wide.direction[i];
            }
        }
        const float limit = 0.95f * wide.halfWidth;
        bool left = false, right = false;
        const auto* v = reinterpret_cast<const float*>(vertices);
        for (unsigned n = 0; n < count && !(left && right); ++n, v += 3)
        {
            const float d = v[0] * depth[0] + v[1] * depth[1] + v[2] * depth[2] + depth[3];
            if (!(d > 0))
            {
                continue;
            }
            const float s = v[0] * side[0] + v[1] * side[1] + v[2] * side[2] + side[3];
            if (s <= -limit * d)
            {
                left = true;
            }
            if (s >= limit * d)
            {
                right = true;
            }
        }
        return left && right;
    }
    inline void ObjectMatrix(SafetyMipsContext& regs)
    {
        if (!wide.active)
        {
            return;
        }
        const uint32_t ra = regs.ra;
        if (ra != 0x3653D4 && ra != 0x365474 && ra != 0x365514 && ra != 0x3655B4)
        {
            return;
        }
        const auto data = uintptr_t(regs.s3);
        const auto m = reinterpret_cast<const float*>(uintptr_t(regs.a1));
        if (!data || !m)
        {
            return;
        }
        float center[3], scale = 0;
        for (unsigned i = 0; i < 3; ++i)
        {
            center[i] = m[12 + i];
            for (unsigned j = 0; j < 3; ++j)
            {
                center[i] += Read<float>(data + 0x10 + 4 * j) * m[4 * j + i];
            }
            scale += m[i] * m[i];
        }
        const float radius = Read<float>(data + 0x1C) * std::sqrt(scale);
        float depth = 0, side = 0;
        for (unsigned i = 0; i < 3; ++i)
        {
            depth += (center[i] - wide.location[i]) * wide.direction[i];
            side += (center[i] - wide.location[i]) * wide.right[i];
        }
        if (!(depth > 0 && radius >= wide.halfWidth * depth + std::fabs(side)) || !SpansView(data, m))
        {
            return;
        }
        const float s = wide.stretch - 1;
        for (unsigned row = 0; row < 4; ++row)
        {
            const float* v = m + 4 * row;
            float* out = stretched + 4 * row;
            float along = 0;
            for (unsigned i = 0; i < 3; ++i)
            {
                along += (v[i] - (row == 3 ? wide.location[i] : 0)) * wide.right[i];
            }
            for (unsigned i = 0; i < 3; ++i)
            {
                out[i] = v[i] + s * along * wide.right[i];
            }
            out[3] = v[3];
        }
        regs.a1 = uint32_t(uintptr_t(stretched));
    }

    // NiScreenPolygon (normalized vertices) and NiScreenTexture (pixel
    // rectangles) are converted to GS packets by NiPS2Renderer and kept until
    // their dirty count (+0x1C / +0x18) is set again.
    inline void Rebuild(SafetyMipsContext& regs, unsigned dirty)
    {
        if (rebuild2D && regs.a1)
        {
            --rebuild2D;
            Write<uint16_t>(regs.a1 + dirty, 1);
        }
    }

    // 0x360C5C in NiPS2Renderer::RenderScreenPolygon: f21 + f23 * x is the GS X
    // of a normalized vertex x. s7 = polygon, s6 = vertex count.
    inline void ScreenPolygon(SafetyMipsContext& regs)
    {
        const float k = Fit(), ky = VerticalFit();
        const auto polygon = uintptr_t(regs.s7);
        const unsigned count = Read<uint16_t>(polygon + 12);
        const auto vertices = Read<uintptr_t>(polygon + 16);
        if ((k == 1 && ky == 1) || !count || !vertices)
        {
            return;
        }
        float left = Read<float>(vertices), right = left, top = Read<float>(vertices + 4), bottom = top;
        for (unsigned i = 1; i < count; ++i)
        {
            const float x = Read<float>(vertices + 12 * i), y = Read<float>(vertices + 12 * i + 4);
            left = std::min(left, x);
            right = std::max(right, x);
            top = std::min(top, y);
            bottom = std::max(bottom, y);
        }
#ifdef WFP_PS2_DEBUG
        trace2D(1, polygon, left, right, top, bottom);
#endif
        if (HiddenBand(left, right, top, bottom))
        {
            regs.f23 = regs.f24 = 0;
            return;
        }
        if (left <= 0.01f && right >= 0.99f)
        {
            return; // fades and backgrounds
        }
        // Outside HUD views screen polygons are projected markers placed by the
        // widened 3D camera: keep their position, fit their shape. In HUD views
        // they are HUD elements in 4:3 positions.
        const float center = 0.5f * (left + right);
        const float pivot = view ? 0.5f : center;
        const float offset = view ? float(ElementAnchor(center)) * HudShift() : 0;
        // x' = pivot + (x - pivot) * k + offset
        regs.f21 = regs.f21 + regs.f23 * (pivot * (1 - k) + offset);
        regs.f23 = regs.f23 * k;
        const float pivotY = 0.5f * (top + bottom);
        regs.f22 += regs.f24 * pivotY * (1 - ky);
        regs.f24 *= ky;
    }

    // 0x3607E0 in NiPS2Renderer::RenderScreenTexture, after the sprite packet is
    // built: fp = packet, s1 = rectangle count. Each 0x28-byte record after the
    // 0x20-byte header holds RGBAQ, UV, XYZ2, UV, XYZ2 (X in 12.4 with offset).
    inline void ScreenTexture(SafetyMipsContext& regs)
    {
        const float k = Fit(), ky = VerticalFit();
        const auto renderer = uintptr_t(regs.s6), packet = uintptr_t(regs.fp);
        const unsigned count = regs.s1;
        const int width = Read<int32_t>(renderer + 0x828), origin = Read<int32_t>(renderer + 0x9A4) * 16;
        if ((k == 1 && ky == 1) || !count || !packet || width <= 0)
        {
            return;
        }
        auto x = [&](unsigned i, unsigned offset)
        { return reinterpret_cast<uint16_t*>(packet + 0x20 + 0x28 * i + offset); };
        int left = *x(0, 0x10), right = *x(0, 0x20), top = *x(0, 0x12), bottom = *x(0, 0x22);
        for (unsigned i = 1; i < count; ++i)
        {
            left = std::min(left, int(*x(i, 0x10)));
            right = std::max(right, int(*x(i, 0x20)));
            top = std::min(top, int(*x(i, 0x12)));
            bottom = std::max(bottom, int(*x(i, 0x22)));
        }
        const float scale = 1.0f / (16.0f * float(width));
        const float l = float(left - origin + 4) * scale, r = float(right - origin + 4) * scale;
        const int height = Read<int32_t>(renderer + 0x82C), originY = Read<int32_t>(renderer + 0x9A8) * 16;
#ifdef WFP_PS2_DEBUG
        trace2D(2, uintptr_t(regs.s5), l, r, float(top - originY) / (16.0f * float(height)),
                float(bottom - originY) / (16.0f * float(height)));
#endif
        if (height > 0 && HiddenBand(l, r, float(top - originY) / (16.0f * float(height)),
                                     float(bottom - originY) / (16.0f * float(height))))
        {
            for (unsigned i = 0; i < count; ++i)
            {
                *x(i, 0x20) = *x(i, 0x10);
                *x(i, 0x22) = *x(i, 0x12);
            }
            return;
        }
        if (l <= 0.01f && r >= 0.99f)
        {
            return; // fades and backgrounds
        }
        const float offset = float(ElementAnchor(0.5f * (l + r))) * HudShift() * 16.0f * float(width);
        const float pivot = float(origin) + 8.0f * float(width);
        for (unsigned i = 0; i < count; ++i)
        {
            for (unsigned field : {0x10u, 0x20u})
            {
                const float value = pivot + (float(*x(i, field)) - pivot) * k + offset;
                *x(i, field) = uint16_t(int(value + 0.5f));
            }
        }
        if (height > 0 && ky != 1)
        {
            const float pivotY = float(Read<int32_t>(renderer + 0x9A8) * 16) + 8.0f * float(height);
            for (unsigned i = 0; i < count; ++i)
            {
                for (unsigned field : {0x12u, 0x22u})
                {
                    auto* value = x(i, field);
                    *value = uint16_t(int(pivotY + (float(*value) - pivotY) * ky + 0.5f));
                }
            }
        }
    }

    inline void Install()
    {
        eventView.create(0x1581A0, &Draw<eventView, -1>);
        timeView.create(0x1094B0, &Draw<timeView, 1>);
        mapView.create(0x10ADF0, &Draw<mapView, 2>);
        inspectView.create(0x14FDA0, &Draw<inspectView, -1>);
        subtitlesView.create(0x147370, &Subtitles);
        targetView.create(0x14E920, &Draw<targetView, 2>);
        kittInfo.create(0x1548A0, &KittInfo);
        scene.create(0x129510, &Scene);
    }
} // namespace kr::hud

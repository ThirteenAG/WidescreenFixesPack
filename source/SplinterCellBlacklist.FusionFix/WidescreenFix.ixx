module;

#include <stdafx.h>
#include <d3d9.h>

export module WidescreenFix;

import ComVars;

namespace l3d
{
    using LeadOptions = float*;
    LeadOptions* (*GetResource)() = nullptr;
    LeadOptions Lead_GetOptions(LeadOptions* lead)
    {
        return lead[2];
    }
}

// Scaleform batches (sub_17B1F30): the frame (ebx) has the vertices at +0Ch and the stride at +10h, the batch the vertex count at +14h.
// Vertex positions are int16 for the 4, 8 and 12 bytes formats (1, 2, 7), floats otherwise.
// A full screen overlay (blood, damage) is a single shape of up to 8 vertices (a quad, a frame) that reaches every edge of the stage.
bool IsStageOverlayUnsafe(uintptr_t frame, uintptr_t batch, const float* m)
{
    auto vertices = *reinterpret_cast<uint8_t**>(frame + 0xC);
    auto stride = *reinterpret_cast<int*>(frame + 0x10);
    auto count = *reinterpret_cast<int*>(batch + 20);
    if (!vertices || count < 4 || count > 8)
        return false;
    float minX = FLT_MAX, maxX = -FLT_MAX, minY = FLT_MAX, maxY = -FLT_MAX;
    for (int i = 0; i < count; i++)
    {
        auto vertex = vertices + i * stride;
        auto x = stride <= 12 ? float(*reinterpret_cast<int16_t*>(vertex)) : *reinterpret_cast<float*>(vertex);
        auto y = stride <= 12 ? float(*reinterpret_cast<int16_t*>(vertex + 2)) : *reinterpret_cast<float*>(vertex + 4);
        auto clipX = m[0] * x + m[1] * y + m[3];
        auto clipY = m[4] * x + m[5] * y + m[7];
        minX = std::min(minX, clipX); maxX = std::max(maxX, clipX);
        minY = std::min(minY, clipY); maxY = std::max(maxY, clipY);
    }
    constexpr float edge = 0.99f;
    if (minY > -edge || maxY < edge)
        return false;

    // split screen: a player's overlay covers their half
    if (bSplitscreen && ((minX <= -edge && std::abs(maxX) < 0.02f) || (std::abs(minX) < 0.02f && maxX >= edge)))
        return true;
    return minX <= -edge && maxX >= edge;
}

bool IsStageOverlay(uintptr_t frame, uintptr_t batch, const float* m)
{
    __try { return IsStageOverlayUnsafe(frame, batch, m); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

float fScreenCullBias = 0.0f;
float fFOVDiff = 1.0f;
SafetyHookInline shTriggerScreenCullBias{};
void __cdecl TriggerScreenCullBias(char bStickyCamState)
{
    auto options = l3d::Lead_GetOptions(l3d::GetResource());

    // original code
    //if (bStickyCamState)
    //    options[130] = 0.3f;
    //else
    //    options[130] = 1.0f;

    if (!fScreenCullBias) // disable culling
        options[130] = 0.0001f;
    else
        options[130] = fScreenCullBias / fFOVDiff;
}

// a menu, dialog or loading screen covers the game
bool bPillarbox = false;
// a screen narrower than 16:9 letterboxes the 3D view
bool bLetterbox = false;

// the HUD viewport was cut to 16:9 (ScaleHUD)
bool bHUDClipped = false;
// a full screen movie was cut to 16:9, the screen around it is cleared
bool bClearMovie = false;

bool IsWide() { return GetAspectRatio() > fDefaultAspectRatio + 0.01f; }
bool IsNarrow() { return GetAspectRatio() < fDefaultAspectRatio - 0.01f; }
bool ClipUI() { return (IsWide() && bPillarbox) || (IsNarrow() && (bPillarbox || bLetterbox)); }
bool ScaleUI() { return (IsWide() || IsNarrow()) && !ClipUI(); }

SafetyHookInline shLetterbox{};
BOOL __fastcall Letterbox(uintptr_t device, void* edx, int* x, int* y, int* width, int* height)
{
    shLetterbox.fastcall<BOOL>(device, edx, x, y, width, height);
    auto screenWidth = *reinterpret_cast<int*>(device + 180);
    auto screenHeight = *reinterpret_cast<int*>(device + 184);
    *x = 0;
    *y = 0;
    *width = screenWidth;
    *height = screenHeight;
    if (bLetterbox && screenHeight > 0 && float(screenWidth) / float(screenHeight) < fDefaultAspectRatio - 0.01f)
    {
        *height = int(screenWidth / fDefaultAspectRatio);
        *y = (screenHeight - *height) / 2;
    }
    return *width != screenWidth || *height != screenHeight;
}

// UI::HudScene::GetViewport2MoviePosition: a viewport position stretched over the 1280x720 stage, or in the letterboxed 3D view
// (letterbox rect, virtual +80h). The reticle is at the camera's reticle position (sub_D4CB80, a fraction of the viewport, the shots
// go there) and drawn on the 16:9 stage in the middle of the screen, its position is made for that. World markers are drawn stretched.
bool(__cdecl* IsSplitscreen)() = nullptr;
const float* reticlePosition = nullptr;
SafetyHookInline shReticlePosition{};
float* __fastcall ReticlePosition(uintptr_t camera, void* edx, float* out)
{
    auto result = shReticlePosition.fastcall<float*>(camera, edx, out);
    reticlePosition = result;
    return result;
}

SafetyHookInline shViewport2Movie{};
float* __fastcall Viewport2Movie(uintptr_t scene, void* edx, float* out, const float* position, uintptr_t viewport)
{
    auto result = shViewport2Movie.fastcall<float*>(scene, edx, out, position, viewport);

    // Split screen: the HUD elements are scaled around the middle of their half (ScaleRow), the positions on the 3D view (markers)
    // are moved the other way, they end up where they were
    if (bSplitscreen && IsWide())
    {
        auto scale = fDefaultAspectRatio / GetAspectRatio();
        auto x = out[0] / 640.0f - 1.0f;
        auto center = x < -0.05f ? -0.5f : x > 0.05f ? 0.5f : 0.0f;
        out[0] = ((x - center) / scale + center + 1.0f) * 640.0f;
        reticlePosition = nullptr;
        return result;
    }

    if (position != std::exchange(reticlePosition, nullptr) || (IsSplitscreen && IsSplitscreen()))
        return result;
    if (IsWide())
    {
        auto movieWidth = 720.0f * GetAspectRatio();
        out[0] = out[0] * movieWidth / 1280.0f - (movieWidth - 1280.0f) * 0.5f;
    }
    else if (IsNarrow() && !bLetterbox)
    {
        auto movieHeight = 1280.0f / GetAspectRatio();
        out[1] = out[1] * movieHeight / 720.0f - (movieHeight - 720.0f) * 0.5f;
    }
    return result;
}

export void InitWidescreenFix()
{
    CIniReader iniReader("");
    auto bUltraWideSupport = iniReader.ReadInteger("MAIN", "UltraWideSupport", 1) != 0;
    bLetterbox = iniReader.ReadInteger("MAIN", "Letterbox", 0) != 0;
    static auto fFOVFactor = std::clamp(iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f), 0.5f, 2.5f);
    fScreenCullBias = std::clamp(iniReader.ReadFloat("MAIN", "ScreenCullBias", 0.0f), 0.0f, 1.0f);

    if (bUltraWideSupport)
    {
        auto pattern = find_pattern("F3 0F 10 45 ? 0F 5A C9 0F 5A C0 F2 0F 5E C1 F2 0F 59 05", "F3 0F 10 45 ? 8B 46 34 0F 5A C9");
        injector::MakeNOP(pattern.get_first(), 5, true);
        static auto BootSeqCommon__Render_Bink_textures = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm0.f32[0] = (((*(float*)(regs.ebp + 0x14)) * fDefaultAspectRatio) - regs.xmm1.f32[0]) * (-0.5f);
            else
                regs.xmm0.f32[0] = *(float*)(regs.ebp + 0x20);
        });

        // BinkVideoPlayer render (sub_1648F50): full screen movies (the player's +100h set by sub_1648450) are drawn at -1..1 in clip space,
        // the rect (x0, y0, x1, y1 at [ebp-60h]) is cut to 16:9 here, the D3DVideoRenderer clears the screen black.
        // Movies drawn in the UI (the title screen background) are already in the 16:9 HUD viewport when it's clipped.
        pattern = hook::pattern("8B 8E BC 00 00 00 89 55 B8 89 45 B4 8B 11 8B 52 28");
        static auto BinkFullScreenRect = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (*reinterpret_cast<uint8_t*>(regs.esi + 0x100) == 0 || bHUDClipped)
                return;
            auto rect = reinterpret_cast<float*>(regs.ebp - 0x60);
            bClearMovie = IsWide() || IsNarrow();
            if (IsWide())
            {
                auto scale = fDefaultAspectRatio / GetAspectRatio();
                rect[0] *= scale;
                rect[2] *= scale;
            }
            else if (IsNarrow())
            {
                auto scale = GetAspectRatio() / fDefaultAspectRatio;
                rect[1] *= scale;
                rect[3] *= scale;
            }
        });

        // D3DVideoRenderer render (sub_16DD470) clears the screen only when it draws to a viewport's render target, the bars around
        // a cut full screen movie would show the last frame. Cleared to black before the movie is drawn (esi is the device).
        pattern = hook::pattern("6A 01 6A 00 6A 00 8B CE E8 ? ? ? ? 33 C9 51 51 6A 3E 56 E8");
        if (!pattern.empty())
        {
            static auto Clear = reinterpret_cast<void(__fastcall*)(uintptr_t, void*, uint32_t, void*, uint32_t, uint32_t, float, uint32_t)>(injector::GetBranchDestination(pattern.get_first(8)).as_int());
            static auto MovieClear = safetyhook::create_mid(pattern.get_first(15), [](SafetyHookContext& regs)
            {
                if (std::exchange(bClearMovie, false))
                    Clear(regs.esi, nullptr, 0, nullptr, 1, 0xFF000000, 1.0f, 0);
            });
        }
        else
        {
            pattern = hook::pattern("6A 01 6A 00 6A 00 50 FF D2 33 D2 52 52 6A 3E 56 E8");
            static auto MovieClear = safetyhook::create_mid(pattern.get_first(11), [](SafetyHookContext& regs)
            {
                if (std::exchange(bClearMovie, false))
                {
                    auto device = *reinterpret_cast<IDirect3DDevice9**>(regs.esi + 0x34);
                    device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
                }
            });
        }

        pattern = hook::pattern("F2 0F 10 0D ? ? ? ? F2 0F 10 15 ? ? ? ? 89 47 10");
        injector::MakeNOP(pattern.get_first(), 8, true);
        static auto FCanvasUtil__SetViewport = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm1.f64[0] = 2.0 / (GetAspectRatio() / fDefaultAspectRatio);
            else
                regs.xmm1.f64[0] = 2.0;
        });

        // UD3DRenderDevice
        pattern = hook::pattern("76 23 8B 06");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jbe -> jmp

        // UD3DRenderDevice letterbox (sub_A804E0, x, y, width, height of the 3D view): the game uses the MonitorAspect setting
        // (video options, the pixel aspect ratio above is 1 now) and letterboxes and pillarboxes the 3D view to 16:9.
        // Here only a screen narrower than 16:9 is letterboxed, with Letterbox on.
        pattern = hook::pattern("55 8B EC 83 EC 08 8B 81 B4 00 00 00 53 56 8B 75 10 89 06 8B 91 B8 00 00 00 8D 99 B4 00 00 00");
        shLetterbox = safetyhook::create_inline(pattern.get_first(), Letterbox);

        // The HUD viewport is full screen and the UI projection is scaled to keep the 1280x720 stage 16:9 in the middle (ScaleUI),
        // what the movies draw outside of it is shown. Screens covering the game (menus, dialogs, loading screens, bPillarbox) and
        // the letterboxed 3D view are cut to 16:9 instead (ClipUI), with black bars.
        static auto ScaleHUD = [](auto& x, auto& y, auto& w, auto& h)
        {
            bHUDClipped = ClipUI();
            if (!bHUDClipped)
                return;
            if (IsWide())
            {
                auto width = (float)h * fDefaultAspectRatio;
                x = (int)(((float)w - width) * 0.5f);
                w = (int)width;
            }
            else
            {
                auto height = (float)w / fDefaultAspectRatio;
                y = (int)(((float)h - height) * 0.5f);
                h = (int)height;
            }
        };

        pattern = hook::pattern("F3 0F 11 04 24 50 8B 85 ? ? ? ? 51 52 50 57");
        if (!pattern.count_hint(1).empty())
        {
            static auto D3DRenderer__RenderHUD = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                auto& x = *(uint32_t*)(regs.ebp - 0xAC);
                auto& y = regs.edx;
                auto& w = regs.ecx;
                auto& h = regs.eax;

                ScaleHUD(x, y, w, h);
            });
        }
        else
        {
            pattern = hook::pattern("F3 0F 11 04 24 52 8B 95");
            static auto D3DRenderer__RenderHUD = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                auto& x = *(uint32_t*)(regs.ebp - 0xB4);
                auto& y = regs.ecx;
                auto& w = regs.eax;
                auto& h = regs.edx;

                ScaleHUD(x, y, w, h);
            });
        }

        // UI projection: the HUD primitives (D3DRenderer::RenderHUDPrimitives, sub_17B14E0) have a 4x4 matrix, transposed before it's set,
        // the Scaleform batches (sub_17B1F30, from sub_16D8CC0) a 128 bytes block per instance starting with the 2 rows of the 2x4 view matrix.
        // A row is scaled to put the 1280x720 stage in the middle 16:9 of the screen. sub_16D8CC0 also draws the Flash render
        // targets (buffer 12, in-world screens), only the HUD buffer (11) and batches without their own render target (+24h) are changed.
        // the x row (wider than 16:9) or the y row (narrower) of the view matrix
        // Split screen: the stage covers the screen and each player's HUD is laid out in its half for a 16:9 screen, every element
        // (its own matrix) is scaled around the middle of its half (x -0.5 or 0.5, the middle of the screen stays)
        static auto ScaleRow = [](float* m)
        {
            if (bSplitscreen && IsWide())
            {
                auto scale = fDefaultAspectRatio / GetAspectRatio();
                auto center = m[3] < -0.05f ? -0.5f : m[3] > 0.05f ? 0.5f : 0.0f;
                for (int i = 0; i < 3; i++) m[i] *= scale;
                m[3] = m[3] * scale + center * (1.0f - scale);
                return;
            }
            if (IsWide())
                for (int i = 0; i < 4; i++) m[i] *= fDefaultAspectRatio / GetAspectRatio();
            else
                for (int i = 4; i < 8; i++) m[i] *= GetAspectRatio() / fDefaultAspectRatio;
        };
        static bool bHUDBuffer = false;

        // UI::Manager::UpdateCoveredScenes (sub_1877780): a scene covering the screen (menus, dialogs, loading screens, +211h) is shown, bl
        pattern = hook::pattern("84 DB 0F 84 ? ? ? ? 8D 86 20 01 00 00");
        static auto CoveringScene = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            bPillarbox = (regs.ebx & 0xFF) != 0 || IsMenuCursor(); // dialogs over the game use the menu cursor
        });

        // HUD primitive matrix, transposed at [ebp-84h]
        pattern = find_pattern("E8 ? ? ? ? A1 ? ? ? ? 6A 40 8D 55 BC 52 83 E8 80 B9 10 00 00 00 8D B5 7C FF FF FF", "B9 10 00 00 00 8D 75 BC 8D BD 7C FF FF FF F3 A5");
        static auto HUDPrimitiveMatrix = safetyhook::create_mid(pattern.get_first(*pattern.get_first<uint8_t>() == 0xE8 ? 5 : 16), [](SafetyHookContext& regs)
        {
            if (ScaleUI())
                ScaleRow(reinterpret_cast<float*>(regs.ebp - 0x84));
        });

        // the buffer sub_16D8CC0 draws
        pattern = find_pattern("55 8B EC 83 EC 54 A1 ? ? ? ? 33 C5 89 45 FC 8B 45 08 89 4D C0 8B 4D 0C 56 8B 75 10", "55 8B EC 83 EC 5C A1 ? ? ? ? 33 C5 89 45 FC 8B 45 0C 56 8B 75 10");
        static auto RenderUIBuffer = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            bHUDBuffer = *reinterpret_cast<int*>(regs.esp + 4) == 11;
        });

        // Scaleform batch matrices, the batch is [ebx+8]. The DX11 exe copies them to the shader constants (scaled after the memcpy),
        // the DX9 one sets them from the batch (edi, a scaled copy is set instead)
        // Overlays covering the whole stage in the game (blood, damage) are stretched over the screen instead
        static auto IsHUDBatch = [](SafetyHookContext& regs, uint8_t* matrices, uint32_t size)
        {
            auto batch = *reinterpret_cast<uintptr_t*>(regs.ebx + 8);
            if (!bHUDBuffer || !ScaleUI() || *reinterpret_cast<uintptr_t*>(batch + 36))
                return false;
            return !(size == 128 && IsStageOverlay(regs.ebx, batch, reinterpret_cast<float*>(matrices)));
        };
        pattern = hook::pattern("E8 ? ? ? ? 8B 15 ? ? ? ? 8B 4B 08 F3 0F 7E 01");
        if (!pattern.empty())
        {
            static auto UIBatchMatrices = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
            {
                auto data = *reinterpret_cast<uint8_t**>(regs.esp);
                auto size = *reinterpret_cast<uint32_t*>(regs.esp + 8);
                if (!IsHUDBatch(regs, data, size))
                    return;
                for (uint32_t offset = 0; offset < size; offset += 128)
                    ScaleRow(reinterpret_cast<float*>(data + offset));
            });
        }
        else
        {
            pattern = hook::pattern("8B 7B 18 8B 08 03 D2 03 D2 03 D2 52 57 6A 29");
            static auto UIBatchMatrices = safetyhook::create_mid(pattern.get_first(12), [](SafetyHookContext& regs)
            {
                static std::vector<uint8_t> matrices;
                auto batch = *reinterpret_cast<uintptr_t*>(regs.ebx + 8);
                auto size = *reinterpret_cast<uint32_t*>(batch + 0x1C) * 128;
                auto data = reinterpret_cast<uint8_t*>(regs.edi);
                if (!IsHUDBatch(regs, data, size))
                    return;
                matrices.assign(data, data + size);
                for (uint32_t offset = 0; offset < size; offset += 128)
                    ScaleRow(reinterpret_cast<float*>(matrices.data() + offset));
                regs.edi = reinterpret_cast<uintptr_t>(matrices.data());
            });
        }

        // Screens covering the game get black bars: RenderHUD draws them before the UI when [ebp-A1h] is set, at the sides
        // (width [ebp-ACh] / [ebp-B4h]) and at the top and bottom (height [ebp-C0h] / [ebp-ACh]). The screen size is in edi, esi / ebx, edi.
        {
            static bool dx9 = false;
            static int sidesSaved = 0, topSaved = 0;
            pattern = hook::pattern("80 B8 ? 02 00 00 00 75 0D 80 BD 5F FF FF FF 00");
            dx9 = *pattern.get_first<uint8_t>(2) == 0xAF;
            static ptrdiff_t sides = dx9 ? 0xB4 : 0xAC;
            static ptrdiff_t top = dx9 ? 0xAC : 0xC0;
            // before the split screen check (+2B3h, DX9 +2AFh), split screen skips the [ebp-A1h] check
            static auto Bars = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                auto& sidesWidth = *reinterpret_cast<int*>(regs.ebp - sides);
                auto& topHeight = *reinterpret_cast<int*>(regs.ebp - top);
                sidesSaved = sidesWidth;
                topSaved = topHeight;
                if (!bPillarbox || (!IsWide() && !IsNarrow()))
                    return;
                auto width = float(dx9 ? regs.ebx : regs.edi), height = float(dx9 ? regs.edi : regs.esi);
                if (IsWide())
                    sidesWidth = std::max(sidesWidth, int((width - height * fDefaultAspectRatio) / 2.0f));
                else
                    topHeight = std::max(topHeight, int((height - width / fDefaultAspectRatio) / 2.0f));
                *reinterpret_cast<uint8_t*>(regs.ebp - 0xA1) = 1;
            });

            pattern = find_pattern("8B 43 40 80 B8 A8 05 00 00 00 0F 84", "8B 8D 58 FF FF FF 8B 49 40 80 B9 A4 05 00 00 00 0F 84");
            static auto BarsDone = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                *reinterpret_cast<int*>(regs.ebp - sides) = sidesSaved;
                *reinterpret_cast<int*>(regs.ebp - top) = topSaved;
            });
        }

        pattern = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 11 4C 24 ? 05 ? ? ? ? 66 0F 5A C0 F3 0F 11 04 24 50 8B 45 08 50 E8 ? ? ? ? 83 C4 10 5D C3");
        injector::MakeNOP(pattern.count(2).get(0).get<void>(0), 8, true);
        static auto ScreenFade1 = safetyhook::create_mid(pattern.count(2).get(0).get<void>(0), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm1.f32[0] = 1.0f / (GetAspectRatio() / fDefaultAspectRatio);
            else
                regs.xmm1.f32[0] = 1.0f;
        });

        static auto ScreenFade2 = safetyhook::create_mid(pattern.count(2).get(1).get<void>(0), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm1.f32[0] = 1.0f / (GetAspectRatio() / fDefaultAspectRatio);
            else
                regs.xmm1.f32[0] = 1.0f;
        });

        pattern = hook::pattern("55 8B EC 83 EC 5C 89 4D A4 83 7D 10 00 75 0B 8B 4D A4 E8");
        IsSplitscreen = reinterpret_cast<decltype(IsSplitscreen)>(injector::GetBranchDestination(hook::pattern("E8 ? ? ? ? 88 45 EB E8").get_first()).as_int());
        shViewport2Movie = safetyhook::create_inline(pattern.get_first(), Viewport2Movie);
        pattern = hook::pattern("E8 ? ? ? ? 50 8D 45 B8 50 8B 8D 6C FF FF FF E8");
        shReticlePosition = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), ReticlePosition);

        // Mouse cursor viewport to movie position: the 1280x720 stage is 16:9 in the middle of the viewport
        pattern = hook::pattern("F3 0F 11 40 ? 5B 5E 8B E5 5D C3");
        injector::MakeNOP(pattern.get_first(0), 5, true);
        static auto UI_Alignment = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
        {
            auto width = (float)*(int*)(regs.ebp - 8);
            auto height = (float)*(int*)(regs.ebp - 4);
            auto x = *(float*)(regs.esi + 0x3C);
            auto y = *(float*)(regs.esi + 0x40);
            auto aspect = width / height;
            auto movieWidth = 1280.0f, movieHeight = 720.0f;
            if (aspect > fDefaultAspectRatio)
                movieWidth = 720.0f * aspect;
            else if (bLetterbox)
                height = width / fDefaultAspectRatio; // the position is in the letterboxed 3D view already
            else
                movieHeight = 1280.0f / aspect;
            *(float*)(regs.eax + 0) = x * movieWidth / width - (movieWidth - 1280.0f) * 0.5f;
            *(float*)(regs.eax + 4) = y * movieHeight / height - (movieHeight - 720.0f) * 0.5f;
        });

        pattern = find_pattern("F3 0F 10 46 ? F3 0F 11 85 ? ? ? ? 8D 85", "F3 0F 10 05 ? ? ? ? F3 0F 11 44 24 ? 8D 14 24");
        static auto UI__ScenePause__PerformPauseOption = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm0.f32[0] = GetAspectRatio();
            else
                regs.xmm0.f32[0] = *(float*)(regs.esi + 0x50);
        });

    }

    // FOV
    {
        auto pattern = hook::pattern("F3 0F 10 46 ? 0F 57 E4 0F 2E C4");
        struct CameraImpl__UpdateFOV
        {
            void operator()(injector::reg_pack& regs)
            {
                float val = 0.0f;

                _asm { fstp dword ptr[val] }

                fFOVDiff = val;
                if (GetAspectRatio() > fDefaultAspectRatio)
                    val = AdjustFOV(val, GetAspectRatio(), fDefaultAspectRatio);

                val *= fFOVFactor;

                if (val > 162.5f)
                    val = 162.5f;

                fFOVDiff = val / fFOVDiff;

                *(float*)(regs.esi + 0x34) = val;
                regs.xmm0.f32[0] = val;

                _asm { fld dword ptr[val] }

                TriggerScreenCullBias(false);
            }
        }; injector::MakeInline<CameraImpl__UpdateFOV>(pattern.get_first(0));

        // Cull
        pattern = hook::pattern("55 8B EC E8 ? ? ? ? 8B C8 E8 ? ? ? ? 80 7D 08 00");
        l3d::GetResource = (l3d::LeadOptions * (*)())injector::GetBranchDestination(pattern.get_first(3), true).as_int();
        shTriggerScreenCullBias = safetyhook::create_inline(pattern.get_first(), TriggerScreenCullBias);
    }
}

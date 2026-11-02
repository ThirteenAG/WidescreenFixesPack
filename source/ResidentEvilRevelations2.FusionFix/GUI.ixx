module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <string_view>
#include <atomic>
#include <unordered_map>

export module GUI;

import ComVars;
import SplitScreen;

namespace Rev2HudLayout
{
    inline float Number(std::string_view text)
    {
        const auto first = text.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos)
            return 0.0f;
        const auto last = text.find_last_not_of(" \t\r\n");
        text = text.substr(first, last - first + 1);
        float value = 0.0f;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        return result.ec == std::errc{} && result.ptr == text.data() + text.size() && std::isfinite(value) && value > 0.0f ? value : 0.0f;
    }

    inline float ParseAspect(std::string_view text)
    {
        const auto colon = text.find(':');
        const float width = Number(colon == std::string_view::npos ? text : text.substr(0, colon));
        const float height = colon == std::string_view::npos ? 1.0f : Number(text.substr(colon + 1));
        const float ratio = height > 0.0f ? width / height : 0.0f;
        return std::isfinite(ratio) && ratio > 0.0f ? std::clamp(ratio, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
    }

    // Clip-space displacement from the original layout edge to the selected box edge.
    // A zero constraint means Auto; ratios wider than the viewport use the viewport.
    inline float EdgeShift(float viewportAspect, float layoutAspect, float constraint)
    {
        if (viewportAspect <= 0.0f || layoutAspect <= 0.0f)
            return 0.0f;
        const float box = constraint > 0.0f
            ? std::min(viewportAspect, std::clamp(constraint, 4.0f / 3.0f, 32.0f / 9.0f))
            : viewportAspect;
        return (box - layoutAspect) / viewportAspect;
    }
}

// The HUD, menus and 2D screens. Every GUI object (uGUI...) is drawn through one function, which sets the transform
// from the object's 1280x720 layout to the screen in a vertex shader constant (scale x, scale y, offset x, offset y).
// The game scales the layout by the screen height, starting at the left edge; the fix moves it per class:
// - Most objects: the layout centered (in split screen, in the player's viewport).
// - Full-screen layers (fades, damage): stretched over the whole screen.
// - Subtitles: centered, with the text narrowed back to the layout's width.
// - Markers that follow 3D positions (commands, enemy/partner vitality, damage numbers): as the game places them;
//   the ones projected into the 1280 wide layout get the screen's width to project into.
// The 3D items of the inventory are rendered into the layout and follow it.

namespace
{
    enum class Mode
    {
        Center,     // the 16:9 layout centered (the game draws it at the left edge)
        Right,      // a right-anchored HUD group
        Stretch,    // over the whole screen
        Subtitles,  // centered, narrowed back to 16:9
        Offset,     // centered, only when drawn for the whole screen
        Cursor,     // mouse cursor in the centered menu coordinate space
        Vanilla,    // as the game draws it
        Hidden,     // not drawn
    };

    const int* pDisableDamageOverlay = nullptr;
    const int* pDisableFade = nullptr;
    const int* pDisableGUICommandFar = nullptr;
    std::atomic<float> fHudAspect{ 0.0f };

    thread_local Mode nCurrentMode = Mode::Center;

    // A horizontal shift in layout units for the draw being issued, added after the class transform.
    thread_local float fGroupShift = 0.0f;

    // The Raid vitality unit (player panel at the left, mission counter at the right) is one GUI tree under one
    // transform, and its node positions are animated every frame. Draw it in two passes, each with only the groups
    // of one side linked into the tree, and move each pass to its side of the HUD box.
    struct VitalitySides
    {
        uint8_t* group = nullptr;
        uint8_t* first = nullptr;
        std::vector<uint8_t*> nodes;      // the group's children in their native order
        std::vector<uint8_t*> next;       // their native sibling links
        float displacement = 0.0f;        // layout units from the 16:9 layout edge to the box edge

        static uint8_t*& Child(uint8_t* node) { return *reinterpret_cast<uint8_t**>(node + 96); }
        static uint8_t*& Sibling(uint8_t* node) { return *reinterpret_cast<uint8_t**>(node + 100); }

        VitalitySides(void* object, void* context)
        {
            auto root = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(object) + 244);
            group = root ? Child(root) : nullptr;
            // A child list that is about to be re-sorted is drawn as it is.
            if (!group || !context || (*reinterpret_cast<uint32_t*>(group + 84) & 0x40000))
                return;

            float viewportAspect = GetAspectRatio();
            float layoutAspect = defaultAspectRatio;
            if (IsSplitScreenActive())
            {
                auto viewport = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(context) + 4);
                if (!viewport)
                    return;
                const auto width = *reinterpret_cast<int32_t*>(viewport + 196) - *reinterpret_cast<int32_t*>(viewport + 188);
                const auto height = *reinterpret_cast<int32_t*>(viewport + 200) - *reinterpret_cast<int32_t*>(viewport + 192);
                const bool vertical = IsSplitScreenVertical();
                const bool inViewport = vertical ? (width == SplitScreen::GetCurrentResX() || width == SplitScreen::GetCurrentResX() + 1)
                                                 : (height == SplitScreen::GetCurrentResY() || height == SplitScreen::GetCurrentResY() + 1);
                if (!inViewport)
                    return;
                viewportAspect = vertical ? viewportAspect / 2.0f : viewportAspect * 2.0f;
                layoutAspect = float(SplitScreen::NativeResX) / float(SplitScreen::NativeResY);
            }
            const auto constraint = fHudAspect.load(std::memory_order_relaxed);
            const auto box = constraint > 0.0f ? std::min(viewportAspect, constraint) : viewportAspect;
            displacement = viewportAspect < layoutAspect
                ? 360.0f * layoutAspect * (box / viewportAspect - 1.0f)
                : 360.0f * (box - layoutAspect);
            if (std::abs(displacement) < 0.001f)
                return;
            first = Child(group);
            for (auto node = first; node && nodes.size() < 16; node = Sibling(node))
            {
                nodes.push_back(node);
                next.push_back(Sibling(node));
            }
        }

        bool Split() const { return nodes.size() > 1; }

        // Link only the groups of one side (left of the layout's center or not), draw, restore the native links.
        template<typename Draw>
        void Pass(bool left, Draw&& draw)
        {
            const auto center = 360.0f * defaultAspectRatio;
            uint8_t* head = nullptr;
            uint8_t* tail = nullptr;
            for (auto node : nodes)
            {
                const bool isLeft = *reinterpret_cast<float*>(node + 160) < center;
                if (isLeft != left)
                    continue;
                if (tail)
                    Sibling(tail) = node;
                else
                    head = node;
                tail = node;
            }
            if (!head)
                return;
            Sibling(tail) = nullptr;
            Child(group) = head;
            fGroupShift = left ? -displacement : displacement;
            draw();
            fGroupShift = 0.0f;
            Child(group) = first;
            for (size_t i = 0; i < nodes.size(); ++i)
                Sibling(nodes[i]) = next[i];
        }
    };

    Mode GetClassMode(void* object)
    {
        thread_local std::unordered_map<uintptr_t, Mode> known;
        auto vtable = *(uintptr_t*)object;
        auto it = known.find(vtable);
        if (it != known.end())
            return it->second;

        Mode mode = Mode::Center;
        if (auto name = GetClassName(object))
        {
            std::string_view n(name);
            if (n == "uBioGUISubtitles")
                mode = Mode::Subtitles;
            else if (n == "uGUIMouseCursor")
                mode = Mode::Cursor;
            else if (n == "uGUIEquip" || n == "uGUIEquipShortcut" || n == "uGUIEquipShortcutRaid" || n == "uGUIRaidEquip" ||
                     n == "uGUIHealNum" || n == "uGUIPurpose" || n == "uGUIFlash" ||
                     n == "uGUIMap" || n == "uGUIMapIcon" || n.starts_with("uGUIMapDetail"))
                mode = Mode::Right;
            else if (n == "uGUIFade" || n == "uGUIDamage" || n == "uGUIDamage2")
                mode = Mode::Stretch;
            else if (n == "uGUICommandBase" || n == "uGUICommandNear" || n == "uGUIIndicatorNear" || n == "uGUIVitalityPartner" ||
                     n == "uGUIVitalityEnemy" || n == "uGUIDamageNum")
                mode = Mode::Vanilla;
            else if (n == "uGUICommandFar")
                mode = Mode::Offset;
        }
        known.emplace(vtable, mode);
        return mode;
    }

    Mode GetMode(void* object)
    {
        auto mode = GetClassMode(object);
        if (mode == Mode::Stretch)
        {
            if (auto name = GetClassName(object); name && std::string_view(name) == "uGUIFade")
                return *pDisableFade ? Mode::Hidden : mode;
            return *pDisableDamageOverlay ? Mode::Hidden : mode;
        }
        if (mode == Mode::Offset && *pDisableGUICommandFar)
            return Mode::Hidden;
        return mode;
    }

    SafetyHookInline shDrawGUI = {};
    void __fastcall DrawGUI(void* object, void* edx, void* context)
    {
        auto mode = GetMode(object);
        if (mode == Mode::Hidden)
            return;
        const auto previous = nCurrentMode;
        nCurrentMode = mode;
        if (auto name = GetClassName(object); name && std::string_view(name) == "uGUIVitalityMe")
        {
            VitalitySides sides(object, context);
            if (sides.Split())
            {
                for (const bool left : { true, false })
                {
                    nCurrentMode = mode;
                    sides.Pass(left, [&]() { shDrawGUI.unsafe_fastcall(object, edx, context); });
                }
            }
            else
                shDrawGUI.unsafe_fastcall(object, edx, context);
        }
        else
            shDrawGUI.unsafe_fastcall(object, edx, context);
        nCurrentMode = previous;
    }

    // The transform the game just computed: constants c[0..3] = scale x, scale y, offset x, offset y (clip space).
    // The game's offset x places the layout's left edge at the viewport's left edge.
    void AdjustClassTransform(float* c, int32_t width, int32_t height);

    void AdjustTransform(float* c, int32_t width, int32_t height)
    {
        AdjustClassTransform(c, width, height);
        // c[0] is the clip-space size of one layout unit
        c[2] += fGroupShift * c[0];
    }

    void AdjustClassTransform(float* c, int32_t width, int32_t height)
    {
        const auto mode = nCurrentMode;
        nCurrentMode = Mode::Center;
        if (mode == Mode::Vanilla)
            return;

        if (mode == Mode::Stretch)
        {
            // The layer is laid out 1280 wide and scaled by the viewport height: widen it to the viewport's width.
            c[2] = -1.0f;
            if (!IsSplitScreenActive() && width > 0 && height > 0)
            {
                const float wider = (float(width) / float(height)) / defaultAspectRatio;
                if (wider > 1.0f)
                    c[0] *= wider;
            }
            return;
        }

        const auto screenAspect = GetAspectRatio();
        if (!IsSplitScreenActive() && screenAspect < defaultAspectRatio && width > 0 && height > 0)
        {
            if (mode == Mode::Offset)
                return;
            // Some narrow modes use a 16:9 logical viewport stretched to the backbuffer.
            // Fit its pixels to the backbuffer width, then center the 16:9 menu vertically.
            const float logicalAspect = float(width) / float(height);
            const float verticalScale = std::min(1.0f, screenAspect / logicalAspect);
            c[1] *= verticalScale;
            c[3] = 1.0f + (c[3] - 1.0f) * verticalScale;
            // The cursor's node coordinates are normalized to 720 even when the
            // native logical viewport is taller. Restore the layout's pixel scale.
            if (mode == Mode::Cursor)
                c[1] *= std::max(1.0f, defaultAspectRatio / logicalAspect);
            if (mode == Mode::Center || mode == Mode::Subtitles || mode == Mode::Cursor)
                c[3] -= 1.0f - screenAspect / defaultAspectRatio;
            return;
        }

        float diff = GetDiff();
        float shift = 1.0f - 1.0f / diff;   // centered
        float viewportAspect = GetAspectRatio();
        float layoutAspect = defaultAspectRatio;
        if (IsSplitScreenActive())
        {
            const bool vertical = IsSplitScreenVertical();
            const bool inViewport = vertical ? (width == SplitScreen::GetCurrentResX() || width == SplitScreen::GetCurrentResX() + 1)
                                             : (height == SplitScreen::GetCurrentResY() || height == SplitScreen::GetCurrentResY() + 1);
            if (inViewport)
            {
                // The game's split HUD is laid out for its 800x360 viewport
                viewportAspect = vertical ? GetAspectRatio() / 2.0f : GetAspectRatio() * 2.0f;
                layoutAspect = (float)SplitScreen::NativeResX / (float)SplitScreen::NativeResY;
                diff = viewportAspect / layoutAspect;
                if (diff < 1.0f)
                {
                    // Narrower than the layout: fitted to the width, centered vertically
                    c[0] *= diff;
                    c[1] *= diff;
                    c[2] = (c[2] + 1.0f) * diff - 1.0f;
                    c[3] = (c[3] - 1.0f) * diff + diff;
                    if (mode == Mode::Right)
                        c[2] += Rev2HudLayout::EdgeShift(viewportAspect, viewportAspect, fHudAspect.load(std::memory_order_relaxed));
                    return;
                }
                shift = 1.0f - 1.0f / diff;
            }
            else if (height == GetResY())
                shift = 1.0f - 1.0f / (GetAspectRatio() / defaultAspectRatio);
            else
                return;
        }
        else if (mode == Mode::Offset && height != GetResY())
            return;

        if (!IsSplitScreenActive())
        {
            // The game may render either a fitted logical viewport or the full
            // backbuffer. Convert its x scale to the physical viewport once.
            const auto physicalWidth = GetResX();
            if (width > 0 && physicalWidth > 0)
                c[0] *= float(width) / float(physicalWidth);
        }
        else if (mode == Mode::Subtitles)
            c[0] /= diff;
        c[2] += shift;
        if (mode == Mode::Right)
            c[2] += Rev2HudLayout::EdgeShift(viewportAspect, layoutAspect, fHudAspect.load(std::memory_order_relaxed));
    }

    // Inventory: the 3D item views, placed in the layout
    SafetyHookInline shSetItemViewPos = {};
    void __fastcall SetItemViewPos(void* view, void* edx, int32_t x, int32_t y)
    {
        if (IsSplitScreenActive())
            x += (int32_t)((720.0f * GetAspectRatio()) - (float)SplitScreen::NativeResX);
        else if (const auto aspect = GetAspectRatio(); aspect < defaultAspectRatio)
        {
            const auto render = pRender.get();
            const float width = float(*(int32_t*)(render + 0x1E0));
            const float height = float(*(int32_t*)(render + 0x1E4));
            if (width > 0.0f && height > 0.0f)
            {
                const float logicalAspect = width / height;
                const float verticalScale = std::min(1.0f, aspect / logicalAspect);
                const float layoutScale = std::min(width / 1280.0f, height / 720.0f);
                const float offset = height * (1.0f - aspect / defaultAspectRatio) / (2.0f * layoutScale);
                y = int32_t(std::lround(y * verticalScale + offset));
            }
        }
        else
            x += (int32_t)(((720.0f * GetAspectRatio()) - 1280.0f) / 2.0f);
        shSetItemViewPos.unsafe_fastcall(view, edx, x, y);
    }
}

class GUI
{
public:
    GUI()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            const auto updateHudAspect = []()
            {
                fHudAspect.store(Rev2HudLayout::ParseAspect(FixSettings().String("MAIN.HudAspectRatioConstraint")), std::memory_order_relaxed);
            };
            updateHudAspect();
            settings.OnChange("MAIN.HudAspectRatioConstraint", updateHudAspect);
            pDisableDamageOverlay = &settings.Int("MAIN.DisableDamageOverlay");
            pDisableFade = &settings.Int("MAIN.DisableFade");
            pDisableGUICommandFar = &settings.Int("MAIN.DisableGUICommandFar");

            auto pattern = hook::pattern("55 8B EC 83 E4 F8 83 EC 40 8B 45 08");
            shDrawGUI = safetyhook::create_inline(pattern.get_first(), DrawGUI);

            // The native mouse state is in the logical viewport. Move hit testing
            // into the centered 16:9 menu layout on either side of that aspect.
            pattern = hook::pattern("89 46 04 8B 8F AC 00 00 00 85 C9");
            static auto MousePositionHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                const auto render = pRender.get();
                if (!render || IsSplitScreenActive())
                    return;
                const auto width = *(int32_t*)(render + 0x1E0);
                const auto height = *(int32_t*)(render + 0x1E4);
                if (width > 0 && height > 0)
                {
                    const auto offset = std::max(0.0f, (float(height) - float(width) / defaultAspectRatio) * 0.5f);
                    const auto offsetX = std::max(0.0f, (float(width) - float(height) * defaultAspectRatio) * 0.5f);
                    auto& x = *reinterpret_cast<int32_t*>(regs.esi);
                    const auto widthScale = std::max(1.0f, (float(width) / float(height)) / defaultAspectRatio);
                    x = int32_t(std::lround((float(x) - offsetX) * widthScale));
                    regs.eax = uint32_t(int32_t(regs.eax) - int32_t(std::lround(offset)));
                }
            });

            // Right after the transform is written: edx = the constants; the viewport's size in the layout space:
            // right at esp+0x30, left at esp+0x0C, bottom at esp+0x10
            pattern = hook::pattern("83 66 4C FD 8B 45 08 F3 0F 10 44 24 38");
            static auto TransformHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                AdjustTransform((float*)regs.edx, *(int32_t*)(regs.esp + 0x30) - *(int32_t*)(regs.esp + 0x0C), *(int32_t*)(regs.esp + 0x10));
            });

            pattern = hook::pattern("A1 ? ? ? ? 53 8B D1 56 80 BA 90 00 00 00 00");
            shSetItemViewPos = safetyhook::create_inline(pattern.get_first(), SetItemViewPos);

            // Markers projected from 3D (enemy and partner vitality, damage numbers) map the camera's view to the
            // layout width (1280, esi), which the HUD scales by the screen height: on wider screens they ended up
            // squeezed into the left 16:9 part. Map the view to the whole screen width instead.
            pattern = hook::pattern("69 F6 40 06 00 00 B8 67 66 66 66 F7 EE C1 FA 09 8B F2 C1 EE 1F 03 F2 8B 45 10");
            static auto ProjectionWidthHook = safetyhook::create_mid(pattern.get_first(23), [](SafetyHookContext& regs)
            {
                if (IsSplitScreenActive())
                    return;
                const auto diff = GetAspectRatio() / defaultAspectRatio;
                if (diff > 1.0f)
                    regs.esi = uint32_t(std::lround(float(int32_t(regs.esi)) * diff));
            });
        };
    }
} GUI;

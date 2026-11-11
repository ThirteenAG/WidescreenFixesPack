module;

#include <stdafx.h>
#include <d3d9.h>
#include "HudLayout.hpp"
#include "FixSettings.hpp"
#include "FloatingPoint.hpp"
#include <array>
#include <atomic>
#include <stdexcept>

export module HudAspect;

import VertexTransform;

import Window;

export namespace GodfatherHud
{
    SafetyHookInline treeHook;
    SafetyHookMid beforeHook, afterHook;
    std::array<void*, 3> radarSurfaces{};
    std::atomic<float> constraint{};

    struct Frame
    {
        Anchor parent = Anchor::Center;
        Anchor node = Anchor::Center;
        float* position{};
        float delta{};
        float original{};
        bool model{};
    };
    thread_local std::array<Frame, 64> frames;
    thread_local unsigned depth{};

    // HUD group of each 3D model node, taken while the tree draws it; the models themselves are drawn later.
    struct ModelPlacement
    {
        const void* node{};
        Anchor anchor = Anchor::Center;
    };
    std::array<ModelPlacement, 16> modelPlacements{};
    size_t nextModelPlacement{};

    void RememberModel(const void* node, Anchor anchor)
    {
        for (auto& placement : modelPlacements)
        {
            if (placement.node == node)
            {
                placement.anchor = anchor;
                return;
            }
        }
        modelPlacements[nextModelPlacement] = { node, anchor };
        nextModelPlacement = (nextModelPlacement + 1) % modelPlacements.size();
    }

    Anchor ModelAnchor(const void* node)
    {
        for (const auto& placement : modelPlacements)
            if (placement.node == node)
                return placement.anchor;
        return Anchor::Center;
    }

    bool CenteredLayer()
    {
        return !depth || (depth <= frames.size() && frames[depth - 1].node == Anchor::Center && !frames[depth - 1].model);
    }

    class TreeScope
    {
    public:
        TreeScope()
        {
            const auto parent = depth && depth <= frames.size() ? frames[depth - 1].node : Anchor::Center;
            if (depth < frames.size())
                frames[depth] = { parent, parent };
            ++depth;
        }
        ~TreeScope() { --depth; }
    };

    void __fastcall Tree(void* node, void*, const float* position, int layer)
    {
        TreeScope scope;
        treeHook.unsafe_thiscall<void>(node, position, layer);
    }

    void Before(SafetyHookContext& context)
    {
        if (!depth || depth > frames.size())
            return;
        auto& frame = frames[depth - 1];
        frame.node = frame.parent;
        frame.position = nullptr;
        frame.delta = 0.0f;
        const auto node = reinterpret_cast<const char*>(context.esi);
        const auto name = *reinterpret_cast<const char* const*>(node + 8);
        if (name)
        {
            if (const auto anchor = NamedAnchor(std::string_view(name, strnlen_s(name, 64))))
                frame.node = *anchor;
        }
        const auto type = *reinterpret_cast<const unsigned*>(node + 4);
        frame.model = (type & (512 | 2048)) != 0;
        if (type & 512)
            RememberModel(node, frame.node);
        if (frame.model && frame.node != Anchor::Center && GodfatherWindow::height)
        {
            Godfather::FloatingPointScope floatingPoint;
            frame.position = reinterpret_cast<float*>(context.esp + 8);
            frame.original = *frame.position;
            frame.delta = Offset(float(GodfatherWindow::width), float(GodfatherWindow::height),
                constraint.load(std::memory_order_relaxed), frame.node) / (float(GodfatherWindow::height) * (4.0f / 3.0f));
            *frame.position += frame.delta;
        }
    }

    void After(SafetyHookContext&)
    {
        if (!depth || depth > frames.size())
            return;
        auto& frame = frames[depth - 1];
        if (frame.position)
            *frame.position = frame.original;
        frame.position = nullptr;
    }

    void Translate(void* vertices, size_t count, void* caller)
    {
        if (!depth || depth > frames.size() || frames[depth - 1].model ||
            std::find(radarSurfaces.begin(), radarSurfaces.end(), caller) != radarSurfaces.end())
            return;
        Godfather::FloatingPointScope floatingPoint;
        const auto offset = Offset(float(GodfatherWindow::width), float(GodfatherWindow::height),
            constraint.load(std::memory_order_relaxed), frames[depth - 1].node);
        if (!offset)
            return;
        for (size_t i = 0; i < count; ++i)
            *reinterpret_cast<float*>(static_cast<char*>(vertices) + 28 * i) += offset;
    }

    void Init()
    {
        auto tree = hook::pattern("55 8B EC 83 E4 F8 83 EC 08 56 57 8B 7D 0C 8B F1 F6 46 20 01");
        auto draw = hook::pattern("8B 06 8D 4C 24 08 51 8B CE FF 50 04 8B 4E 18 85 C9 74 ? 57");
        auto radar = hook::pattern("D9 5E 6C 8B 15 ? ? ? ? FF 52 30 83 C4 14 5F 5E C2 04 00");
        auto frame = hook::pattern("D9 5F 6C 8B 15 ? ? ? ? FF 52 30 83 C4 14 5F 5E C2 04 00");
        auto icons = hook::pattern("83 C4 0C 8B 54 24 30 8B 7C 24 4C");
        radarSurfaces = { radar.get_first(12), frame.get_first(12), icons.get_first() };
        treeHook = safetyhook::create_inline(tree.get_first(), Tree, SafetyHookInline::StartDisabled);
        beforeHook = safetyhook::create_mid(draw.get_first(), Before, SafetyHookMid::StartDisabled);
        afterHook = safetyhook::create_mid(draw.get_first(12), After, SafetyHookMid::StartDisabled);
        (void)beforeHook.enable();
        (void)afterHook.enable();
        (void)treeHook.enable();
        const auto refresh = []()
        {
            Godfather::FloatingPointScope floatingPoint;
            const auto value = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            constraint.store(value && std::isfinite(*value) && *value > 0.0f
                ? std::clamp(*value, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f, std::memory_order_relaxed);
        };
        refresh();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", refresh);
        postTransform = Translate;
    }
}

module;
#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <map>
#include <set>

export module Hud;
import NativeMenu;
import Window;
import HudMath;
import FloatingPoint;
import Cinematics;


export namespace PsiOpsHud
{
    using PsiOpsMenu::Node;
    using PsiOpsMenu::Pointer;
    inline SafetyHookInline orthoHook, drawHook;
    inline void(__thiscall* rebuild)(void*) = nullptr;
    inline std::atomic<float> constraint{ 0.0f };
    inline std::atomic<bool> scopeDrawn{ false };
    struct Anchor { int x = 0, y = 0; };
    inline std::map<Node, Anchor> anchors;
    inline std::set<Node> scopes;

    inline bool Named(Node node, const char* name)
    {
        const auto length = std::strlen(name) + 1;
        return node && (node[3] & 0x8000) && Pointer(node[1])
            && std::memcmp(Pointer(node[1]), name, length) == 0;
    }

    inline int NativeBar(Node node)
    {
        if (!node || node[0] != PsiOpsMenu::ObjectTag || (node[3] & 0x8000))
            return 0;
        const auto transform = Pointer(node[5]);
        const auto children = Pointer(node[6]);
        if (!transform || !children || children[0] != 1)
            return 0;
        const auto group = Pointer(children[1]);
        if (!group || group[0] != PsiOpsMenu::GroupTag)
            return 0;
        const auto list = Pointer(group[6]);
        if (!list || list[0] != 1)
            return 0;
        const auto polygon = Pointer(list[1]);
        if (!polygon || polygon[0] != 0x504F4C59)
            return 0;
        constexpr float vertices[]{ 0.0f, 0.0f, 0.0f, 573.0f, 0.0f, 0.0f,
            0.0f, 92.0f, 0.0f, 573.0f, 92.0f, 0.0f };
        if (std::memcmp(polygon + 4, vertices, sizeof(vertices)) != 0)
            return 0;
        for (size_t i = 16; i < 20; ++i)
            if (polygon[i] != 0xFF000000)
                return 0;
        const auto position = reinterpret_cast<const float*>(transform);
        if (position[6] == -35.0f && position[7] == -100.0f) return 1;
        if (position[6] == -30.0f && position[7] == 450.0f) return -1;
        return 0;
    }

    inline void Register(Node resource)
    {
        if (!resource || resource[0] != 0x53584258)
            return;
        const auto strings = Pointer(resource[5]);
        const bool hud = strings && strings[0] && Pointer(strings[1])
            && std::memcmp(Pointer(strings[1]), "HUD", 4) == 0;
        if (hud)
            PsiOpsCinematics::nativeBars.clear();
        size_t remaining = 1024;
        std::set<Node> visited;
        const auto walk = [&](auto&& self, Node node, size_t depth) -> void
        {
            if (depth >= 32 || !remaining || !node || !visited.insert(node).second)
                return;
            --remaining;
            anchors.erase(node);
            scopes.erase(node);
            PsiOpsCinematics::nativeBars.erase(node);
            if (hud)
                if (const auto bar = NativeBar(node))
                    PsiOpsCinematics::nativeBars[node] = bar > 0;
            if (!PsiOpsMenu::Group(node))
                return;
            if (hud && node[0] == PsiOpsMenu::ObjectTag && node[5])
            {
                if (Named(node, "HealthAndPsi") || Named(node, "Cheats")) anchors[node] = { -1, -1 };
                else if (Named(node, "weapon")) anchors[node] = { 1, 1 };
                else if (Named(node, "UserPromptObject")) anchors[node] = { -1, 1 };
                else if (Named(node, "QuickTip")) anchors[node] = { 0, -1 };
                else if (Named(node, "ScoreAndTime") || Named(node, "TimerAndText") || Named(node, "CDTimer")
                    || Named(node, "boss_0") || Named(node, "boss_1") || Named(node, "boss_2")) anchors[node] = { 1, -1 };
                else if (Named(node, "ItemFlyOut") || Named(node, "ItemFly")) anchors[node] = { 1, 0 };
                else if (Named(node, "Scope")) scopes.insert(node);
            }
            const auto list = Pointer(node[6]);
            if (!list || list[0] > 512)
                return;
            for (uint32_t i = 0; i < list[0]; ++i)
                self(self, Pointer(list[i + 1]), depth + 1);
        };
        walk(walk, Pointer(resource[3]), 0);
    }

    inline uintptr_t __fastcall Ortho(void* self, void*, float nearPlane, float farPlane, float width, float height)
    {
        const auto result = orthoHook.unsafe_thiscall<uintptr_t>(self, nearPlane, farPlane, width, height);
        PsiOps::FloatingPointScope floatingPoint;
        const auto layout = Calculate(PsiOpsWindow::width, PsiOpsWindow::height, constraint);
        const auto matrix = reinterpret_cast<float*>(static_cast<uint8_t*>(self) + 628);
        for (size_t row = 0; row < 4; ++row)
        {
            auto x = matrix + row * 4;
            auto y = matrix + row * 4 + 1;
            *x *= layout.scaleX;
            *y *= layout.scaleY;
        }
        return result;
    }

    inline uintptr_t __fastcall Draw(void* self, void*, void* color)
    {
        const auto node = *(reinterpret_cast<Node*>(static_cast<uint8_t*>(self) + 28));
        if (node && PsiOpsCinematics::Capture(self, node, color))
            return reinterpret_cast<uintptr_t>(self);
        if (!node || !(node[3] & 1))
            return drawHook.unsafe_thiscall<uintptr_t>(self, color);
        if (scopes.contains(node))
            scopeDrawn = true;
        const auto found = anchors.find(node);
        if (found == anchors.end())
            return drawHook.unsafe_thiscall<uintptr_t>(self, color);
        auto x = reinterpret_cast<float*>(static_cast<uint8_t*>(self) + 16);
        auto y = reinterpret_cast<float*>(static_cast<uint8_t*>(self) + 20);
        const auto originalX = *x, originalY = *y;
        {
            PsiOps::FloatingPointScope floatingPoint;
            const auto layout = Calculate(PsiOpsWindow::width, PsiOpsWindow::height, constraint);
            *x = originalX + found->second.x * layout.horizontal;
            *y = originalY + found->second.y * layout.vertical;
            rebuild(self);
        }
        const auto result = drawHook.unsafe_thiscall<uintptr_t>(self, color);
        {
            PsiOps::FloatingPointScope floatingPoint;
            *x = originalX;
            *y = originalY;
            rebuild(self);
        }
        return result;
    }

    inline void Mask(IDirect3DDevice9* device)
    {
        if (!scopeDrawn.exchange(false))
            return;
        IDirect3DSurface9* surface = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
            return;
        D3DSURFACE_DESC description{};
        if (SUCCEEDED(surface->GetDesc(&description)))
        {
            const auto width = LONG(description.Width), height = LONG(description.Height);
            const auto layout = Calculate(width, height, 0.0f);
            const auto horizontal = LONG(std::ceil(float(width) * (1.0f - layout.scaleX) * 0.5f));
            const auto vertical = LONG(std::ceil(float(height) * (1.0f - layout.scaleY) * 0.5f));
            const RECT rectangles[]{ { 0, 0, horizontal, height }, { width - horizontal, 0, width, height },
                { 0, 0, width, vertical }, { 0, height - vertical, width, height } };
            for (const auto& rectangle : rectangles)
                if (rectangle.right > rectangle.left && rectangle.bottom > rectangle.top)
                    device->ColorFill(surface, &rectangle, D3DCOLOR_XRGB(0, 0, 0));
        }
        surface->Release();
    }

    inline void Init()
    {
        auto ortho = hook::pattern("55 8B EC 81 EC 44 01 00 00 56 57 89 8D BC FE FF FF 6A 00 68 00 00 80 3F 68 00 00 80 BF");
        auto draw = hook::pattern("55 8B EC 83 EC 58 89 4D A8 8B 45 A8 8B 48 1C 8B 51 0C 83 E2 01 0F 86 52 01 00 00");
        auto matrix = hook::pattern("55 8B EC 83 EC 54 89 4D AC 8B 45 AC 8B 48 1C 83 79 14 00 0F 84 83 01 00 00");
        rebuild = reinterpret_cast<decltype(rebuild)>(matrix.get_first());
        orthoHook = safetyhook::create_inline(ortho.get_first(), Ortho);
        drawHook = safetyhook::create_inline(draw.get_first(), Draw);
        const auto refresh = []()
        {
            PsiOps::FloatingPointScope floatingPoint;
            const auto value = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            constraint = value && std::isfinite(*value) && *value > 0.0f ? std::clamp(*value, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
        };
        refresh();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", refresh);
        PsiOpsMenu::resourceRelocated = Register;
        const auto previous = PsiOpsWindow::frame;
        PsiOpsWindow::frame = [previous](IDirect3DDevice9* device)
        {
            Mask(device);
            if (previous) previous(device);
        };
    }
}


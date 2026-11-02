module;
#include <stdafx.h>
#include "FixSettings.hpp"

export module Hud;
import ComVars;
import Display;


// Every interface screen is a full-screen root whose panels are anchored: 1 top left, 2 bottom left, 3 top right,
// 4 bottom right, 5 centred (dialogs and screens). The game lays them out for the current resolution itself, so in
// Auto the panels sit at the screen edges. A HUD aspect ratio moves the left panels right and the right panels left,
// as the game would place them on a screen of the box's width, centred. Panels the game keeps tucked away behind a
// screen edge (empty squad slots) stay behind the real screen edge, and follow the box once they slide out.
export namespace ParadiseHud
{
    inline SafetyHookInline LayoutHook;
    inline SafetyHookMid ConstructHook, DestructHook, PlaceHook;
    inline std::atomic<float> Constraint{ 0.0f };
    inline std::unordered_map<uint8_t*, std::unordered_map<uint8_t*, int>> Applied; // interface -> panel -> offset

    struct TreeNode
    {
        uint32_t Color;
        TreeNode* Parent;
        TreeNode* Left;
        TreeNode* Right;
        uint8_t* Widget;
        int32_t Anchor;
    };

    struct ListNode
    {
        ListNode* Next;
        ListNode* Previous;
        uint8_t* Widget;
    };

    inline TreeNode* NextNode(TreeNode* node)
    {
        if (node->Right)
        {
            node = node->Right;
            while (node->Left)
                node = node->Left;
            return node;
        }
        auto parent = node->Parent;
        while (node == parent->Right)
        {
            node = parent;
            parent = parent->Parent;
        }
        return node->Right != parent ? parent : node;
    }

    inline int Offset()
    {
        const int width = ParadiseDisplay::Logical.get_ptr()[0], height = ParadiseDisplay::Logical.get_ptr()[1];
        const float ratio = Constraint.load(std::memory_order_relaxed);
        if (ratio <= 0.0f || width <= 0 || height <= 0)
            return 0;
        const int box = (std::min)(width, int(std::lround(height * ratio)));
        return (width - box) / 2;
    }

    inline void Move(uint8_t* widget, int x)
    {
        using Function = void(__thiscall*)(uint8_t*, int, int);
        (*reinterpret_cast<Function**>(widget))[23](widget, x, 0);
    }

    // layout: right after the interface's first layout pass (panels at their native places). Panels the game adds or
    // places later are put next to panels that already follow the box, so they are only recorded.
    inline void Adjust(uint8_t* root, std::unordered_map<uint8_t*, int>& applied, bool layout)
    {
        const int offset = Offset();
        const int width = ParadiseDisplay::Logical.get_ptr()[0];
        std::unordered_map<uint8_t*, int> anchors;
        const auto header = *reinterpret_cast<TreeNode**>(root + 64);
        for (auto node = header->Left; node && node != header; node = NextNode(node))
            anchors[node->Widget] = node->Anchor;
        std::vector<std::pair<uint8_t*, int32_t*>> panels;
        const auto children = *reinterpret_cast<ListNode**>(root + 60);
        for (auto node = children->Next; node && node != children; node = node->Next)
        {
            const auto rect = node->Widget ? *reinterpret_cast<int32_t**>(node->Widget + 28) : nullptr;
            if (rect)
                panels.emplace_back(node->Widget, rect);
        }
        // The weapon's item icon sits centred in its slot (the game places it from the slot's native place)
        uint8_t* item = nullptr;
        int32_t* itemRect = nullptr;
        int32_t* slotRect = nullptr;
        for (const auto& [panel, rect] : panels)
        {
            const auto name = *reinterpret_cast<const char**>(panel + 4);
            if (*reinterpret_cast<void**>(panel) == SlotItem.get_ptr())
            {
                item = panel;
                itemRect = rect;
            }
            else if (name && std::strcmp(name, "weapon_icon") == 0)
                slotRect = rect;
        }
        for (const auto& [widget, rect] : panels)
        {
            const auto anchor = anchors.find(widget);
            const int type = anchor != anchors.end() ? anchor->second : 1;
            if (type == 5 || widget == item)
                continue;
            const int sign = type == 3 || type == 4 ? -1 : 1;
            if (!layout && !applied.contains(widget))
            {
                const bool tucked = sign < 0 ? rect[6] > width : rect[4] < 0;
                applied[widget] = tucked ? 0 : offset;
                continue;
            }
            auto& current = applied[widget];
            // Where the game put it: left and right edge without the box offset
            const int left = rect[4] - sign * current;
            const bool tucked = sign < 0 ? rect[6] - sign * current > width : left < 0;
            const int wanted = tucked ? 0 : offset;
            if (wanted == current)
                continue;
            Move(widget, sign * (wanted - current));
            current = wanted;
        }
        if (item && slotRect && itemRect[5] >= slotRect[5] && itemRect[7] <= slotRect[7])
        {
            const int x = slotRect[4] + ((slotRect[6] - slotRect[4]) - (itemRect[6] - itemRect[4])) / 2;
            if (x != itemRect[4])
                Move(item, x - itemRect[4]);
        }
    }

    inline void __fastcall Layout(uint8_t* root, void*)
    {
        LayoutHook.thiscall<void>(root);
        const bool first = !Applied.contains(root);
        Adjust(root, Applied[root], first);
    }

    // On the game thread, every frame: the setting, resolution changes and panels sliding out
    inline void Frame()
    {
        for (auto& [root, applied] : Applied)
            Adjust(root, applied, false);
    }

    inline void Init()
    {
        const auto read = []()
        {
            const auto ratio = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            Constraint.store(ratio ? std::clamp(*ratio, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f);
        };
        read();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", read);
        // The interface's layout check after it is built and after mode changes (moves the anchored panels)
        auto item = hook::pattern("68 ? ? ? ? 89 75 F0 E8 ? ? ? ? 83 65 FC 00 68 88 00 00 00 C7 06");
        SlotItem.SetAddress(*item.get_first<void*>(24));
        auto layout = hook::pattern("55 8B EC 83 EC 14 53 56 57 8B F9 E8 ? ? ? ? 8B D8 2B 5F 4C");
        LayoutHook = safetyhook::create_inline(layout.get_first(), Layout);
        // The game places some panels at their native position again (the squad list after loading a game)
        auto place = hook::pattern("55 8B EC 8B 41 1C 8B 11 56 8B 75 0C 2B 70 14 56 8B 75 08 2B 70 10 56 FF 52 5C");
        PlaceHook = safetyhook::create_mid(place.get_first(), [](SafetyHookContext& context)
        {
            const auto widget = reinterpret_cast<uint8_t*>(context.ecx);
            for (auto& [root, applied] : Applied)
            {
                if (const auto panel = applied.find(widget); panel != applied.end())
                    panel->second = 0;
            }
        });
        // Interfaces come and go with the game's screens
        auto construct = hook::pattern("B8 ? ? ? ? E8 ? ? ? ? 51 56 FF 75 08 8B F1 89 75 F0 E8 ? ? ? ? 83 65 FC 00 8D 4E 40");
        ConstructHook = safetyhook::create_mid(construct.get_first(), [](SafetyHookContext& context)
        {
            Applied.erase(reinterpret_cast<uint8_t*>(context.ecx));
        });
        auto destruct = hook::pattern("B8 ? ? ? ? E8 ? ? ? ? 51 51 56 8B F1 57 89 75 F0 C7 06 ? ? ? ? 83 65 FC 00 8D 7E 40");
        DestructHook = safetyhook::create_mid(destruct.get_first(), [](SafetyHookContext& context)
        {
            Applied.erase(reinterpret_cast<uint8_t*>(context.ecx));
        });
    }
}

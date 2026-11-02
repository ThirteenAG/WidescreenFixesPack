module;

#include <stdafx.h>
#include "ModeCatalog.hpp"
#include <stdexcept>

export module ResolutionPicker;

import ComVars;
import NativeMenuABI;

export namespace GodfatherResolutions
{
    using GodfatherMenu::Value;
    SafetyHookInline countHook, resolutionHook, activeHook, selectHook, modeHook;
    void(__thiscall* prepare)(void*){};
    int(__thiscall* integer)(Value*){};
    Value*(__cdecl* number)(int){};
    Value*(__cdecl* text)(const char*){};

    struct Node
    {
        Node* left{};
        Node* parent{};
        Node* right{};
        int key{};
        int width{}, height{}, depth{};
        unsigned flags{};
        int refresh{};
        unsigned format{};
        unsigned char color{}, nil{};
        unsigned short padding{};
    };
    static_assert(sizeof(Node) == 44);
    void* owner{};
    Node* tree{};
    unsigned treeCount{};
    std::vector<Mode> modes;
    bool installed{};
    Value* populatedPage{};
    Value* populatedLabels{};
    Value* populatedIndices{};

    bool Invalidate()
    {
        owner = nullptr;
        tree = nullptr;
        treeCount = 0;
        modes.clear();
        return false;
    }

    template <typename T> bool Read(const void* source, T& value)
    {
        if (!source)
            return false;
        std::memcpy(&value, source, sizeof(T));
        return true;
    }

    bool Catalog(bool refresh = false)
    {
        void* manager = nullptr;
        void* current = nullptr;
        if (!GodfatherMenuScreens.is_initialized() || !GodfatherMenu::api.parameter || !GodfatherMenu::api.undefined ||
            !Read(GodfatherMenuScreens.get_ptr(), manager) || !manager ||
            !Read(static_cast<char*>(manager) + 8, current) || !current)
            return Invalidate();
        unsigned char ready = 0;
        if (!Read(static_cast<char*>(current) + 200 + GodfatherMenu::layoutShift, ready))
            return Invalidate();
        if (!ready)
        {
            prepare(current);
            refresh = true;
        }
        Node* head = nullptr;
        unsigned count = 0;
        if (!Read(static_cast<char*>(current) + 180 + GodfatherMenu::layoutShift, head) || !head ||
            !Read(static_cast<char*>(current) + 184 + GodfatherMenu::layoutShift, count) || count > 8192)
            return Invalidate();
        if (!refresh && owner == current && tree == head && treeCount == count)
            return !modes.empty();
        Node sentinel{};
        if (!Read(head, sentinel))
            return Invalidate();
        std::vector<Mode> found;
        auto node = sentinel.left;
        for (unsigned ordinal = 0; ordinal < count; ++ordinal)
        {
            Node value{};
            if (!node || node == head || !Read(node, value) || value.nil)
                return Invalidate();
            found.push_back({ value.key, int(ordinal), value.width, value.height, value.depth, value.flags, value.refresh });
            Node right{};
            if (!Read(value.right, right))
                return Invalidate();
            if (!right.nil)
            {
                node = value.right;
                unsigned steps = 0;
                for (;;)
                {
                    Node branch{}, left{};
                    if (++steps > count || !Read(node, branch) || !Read(branch.left, left))
                        return Invalidate();
                    if (left.nil)
                        break;
                    node = branch.left;
                }
            }
            else
            {
                auto parent = value.parent;
                unsigned steps = 0;
                for (;;)
                {
                    Node branch{};
                    if (++steps > count + 1 || !Read(parent, branch))
                        return Invalidate();
                    if (branch.nil || node != branch.right)
                        break;
                    node = parent;
                    parent = branch.parent;
                }
                node = parent;
            }
        }
        if (node != head)
            return Invalidate();
        owner = current;
        tree = head;
        treeCount = count;
        modes = Normalize(std::move(found));
        return !modes.empty();
    }

    Value* __cdecl Count()
    {
        return Catalog(true) ? number(int(modes.size())) : countHook.unsafe_ccall<Value*>();
    }
    Value* __cdecl Resolution()
    {
        if (!Catalog())
            return resolutionHook.unsafe_ccall<Value*>();
        const int index = integer(GodfatherMenu::api.parameter(0));
        if (index < 0 || size_t(index) >= modes.size())
            return text(" ");
        const auto& mode = modes[index];
        const auto label = std::to_string(mode.width) + "x" + std::to_string(mode.height);
        return text(label.c_str());
    }
    Value* __cdecl Active()
    {
        if (!Catalog())
            return activeHook.unsafe_ccall<Value*>();
        const int width = *reinterpret_cast<const int*>(static_cast<const char*>(owner) + 188 + GodfatherMenu::layoutShift);
        const int height = *reinterpret_cast<const int*>(static_cast<const char*>(owner) + 192 + GodfatherMenu::layoutShift);
        for (size_t i = 0; i < modes.size(); ++i)
            if (modes[i].width == width && modes[i].height == height)
                return number(int(i));
        return number(0);
    }
    Value* __cdecl Select()
    {
        if (!Catalog())
            return selectHook.unsafe_ccall<Value*>();
        const int index = integer(GodfatherMenu::api.parameter(0));
        if (index >= 0 && size_t(index) < modes.size())
        {
            const auto& mode = modes[index];
            *reinterpret_cast<int*>(static_cast<char*>(owner) + 188 + GodfatherMenu::layoutShift) = mode.width;
            *reinterpret_cast<int*>(static_cast<char*>(owner) + 192 + GodfatherMenu::layoutShift) = mode.height;
            *reinterpret_cast<int*>(static_cast<char*>(owner) + 196 + GodfatherMenu::layoutShift) = mode.ordinal;
        }
        return GodfatherMenu::api.undefined();
    }
    int __fastcall SelectMode(void* self, void*, int width, int height, int depth)
    {
        if (Catalog())
            for (const auto& mode : modes)
                if (mode.width == width && mode.height == height && mode.depth == depth && (mode.flags & 1))
                    return mode.native;
        return modeHook.unsafe_thiscall<int>(self, width, height, depth);
    }

    void Populate(Value* page)
    {
        if (!installed || !page || !Catalog())
            return;
        auto widget = GodfatherMenu::Get(page, "videoRes");
        auto labels = GodfatherMenu::Get(widget, "textArray");
        auto indices = GodfatherMenu::Get(widget, "modeArray");
        if (!widget || !labels || !indices)
            return;
        const auto count = int(modes.size());
        if (populatedPage == page && populatedLabels == labels && populatedIndices == indices &&
            GodfatherMenu::Number(labels, "length") == float(count) &&
            GodfatherMenu::Number(indices, "length") == float(count))
            return;
        GodfatherMenu::Set(labels, "length", 0.0f);
        GodfatherMenu::Set(indices, "length", 0.0f);
        int selected = 0;
        const int width = *reinterpret_cast<const int*>(static_cast<const char*>(owner) + 188 + GodfatherMenu::layoutShift);
        const int height = *reinterpret_cast<const int*>(static_cast<const char*>(owner) + 192 + GodfatherMenu::layoutShift);
        for (int i = 0; i < count; ++i)
        {
            const auto& mode = modes[i];
            const auto key = std::to_string(i);
            const auto label = std::to_string(mode.width) + "x" + std::to_string(mode.height);
            GodfatherMenu::Set(labels, key.c_str(), label);
            GodfatherMenu::Set(indices, key.c_str(), float(i));
            if (mode.width == width && mode.height == height)
                selected = i;
        }
        GodfatherMenu::Set(page, "numVideoModes", float(count));
        GodfatherMenu::Set(page, "currentModeIndex", float(selected));
        GodfatherMenu::Set(page, "currentResIndex", float(selected));
        GodfatherMenu::Set(widget, "hiPos", float(selected));
        const auto& mode = modes[selected];
        GodfatherMenu::Set(GodfatherMenu::Get(widget, "scrollTextSingle"), "text",
            std::to_string(mode.width) + "x" + std::to_string(mode.height));
        populatedPage = page;
        populatedLabels = labels;
        populatedIndices = indices;
    }

    void Disable()
    {
        installed = false;
        countHook = {};
        resolutionHook = {};
        activeHook = {};
        selectHook = {};
        modeHook = {};
    }
    void Init();
}

namespace GodfatherResolutions
{
    void Init()
    {
        auto prepareEntry = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 81 EC 80 00 00 00 53"
            " 55 56 57 6A 03 8B F1 33 DB BF 0F 00 00 00 68 ? ? ? ? 8D 4C 24 60");
        auto integerEntry = hook::pattern("8B 41 04 8B D0 C1 EA 04 F6 D2 F6 C2 01 74 ? 33 C0 C3 C1 F8 19 8D 50 FF 83 FA 29 77"
            " ? 0F B6 92 ? ? ? ? FF 24 95 ? ? ? ? 83 F8 01 74 ?");
        auto numberEntry = hook::pattern("A1 ? ? ? ? 85 C0 56 57 74 ? 8B 48 08 89 0D ? ? ? ? 8B 48 04 83 C9 04 89 48"
            " 04 8B 15 ? ? ? ? 8B 72 04 3B 32 8B F9 8D 4A 04 7C ? 8B 4C 24 0C 83 E7 FB 89 78"
            " 04 5F 5E 89 48 08 C3 8B 52 08 89 04 B2 8B 11 42 5F 89 11 8B 4C 24 08 5E 89 48 08 C3"
            " 8B 0D ? ? ? ? 6A 0C E8 ? ? ? ? 85 C0 74 ? 8B 48 04 81 E1 ? ? ? ? 81 C9"
            " 34 00 00 0E");
        auto textEntry = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 8B 35 ? ? ?"
            " ? 85 F6 74 ? 8B 46 0C A3 ? ? ? ? 8B 46 04 83 C8 04 89 46 04");
        auto countEntry = hook::pattern("8B 0D ? ? ? ? 56 8B 71 08 85 F6 B8 01 00 00 00 74 ? 8A 86 C8 00 00 00 84 C0 75"
            " ? 8B CE E8 ? ? ? ? 8B 86 B8 00 00 00 50 E8 ? ? ? ?");
        auto resolutionEntry = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 44 A1 ? ? ?"
            " ? 53 55 56 57 33 DB 53 89 44 24 54 E8 ? ? ? ? 83 C4 04");
        auto activeEntry = hook::pattern("8B 0D ? ? ? ? 56 8B 71 08 33 C0 85 F6 74 ? 8A 86 C8 00 00 00 84 C0 75 ? 8B CE"
            " E8 ? ? ? ? 8B 86 C4 00 00 00 50 E8 ? ? ? ? 83 C4 04");
        auto selectEntry = hook::pattern("51 53 56 6A 00 E8 ? ? ? ? 83 C4 04 8B C8 E8 ? ? ? ? 8B D8 A1 ? ? ? ? 8B"
            " 70 08 85 F6 74 ? 8A 86 C8 00 00 00 84 C0 75 ? 8B CE E8 ? ? ? ?");
        auto modeEntry = hook::pattern("83 EC 10 55 56 57 8D 44 24 10 50 E8 ? ? ? ? 8B 74 24 14 8B 3E 8B C7 33 ED 3B C6"
            " 89 44 24 0C 74 ? 53 8B 5C 24 2C 8B 4C 24 24 3B 48 10 75 ?");
        if (countEntry.empty()) // Chinese executable
        {
            GodfatherMenu::layoutShift = 4;
            countEntry = hook::pattern("8B 0D ? ? ? ? 56 8B 71 08 85 F6 B8 01 00 00 00 74 ? 8A 86 CC 00 00 00 84 C0 75 ? 8B CE E8 ? ? ? ? 8B 86 BC 00 00 00 50 E8 ? ? ? ?");
            activeEntry = hook::pattern("8B 0D ? ? ? ? 56 8B 71 08 33 C0 85 F6 74 ? 8A 86 CC 00 00 00 84 C0 75 ? 8B CE E8 ? ? ? ? 8B 86 C8 00 00 00 50 E8 ? ? ? ? 83 C4 04");
            selectEntry = hook::pattern("51 53 56 6A 00 E8 ? ? ? ? 83 C4 04 8B C8 E8 ? ? ? ? 8B D8 A1 ? ? ? ? 8B 70 08 85 F6 74 ? 8A 86 CC 00 00 00 84 C0 75 ? 8B CE E8 ? ? ? ?");
        }
        prepare = reinterpret_cast<decltype(prepare)>(prepareEntry.get_first());
        integer = reinterpret_cast<decltype(integer)>(integerEntry.get_first());
        number = reinterpret_cast<decltype(number)>(numberEntry.get_first());
        text = reinterpret_cast<decltype(text)>(textEntry.get_first());
        countHook = safetyhook::create_inline(countEntry.get_first(), Count, SafetyHookInline::StartDisabled);
        resolutionHook = safetyhook::create_inline(resolutionEntry.get_first(), Resolution, SafetyHookInline::StartDisabled);
        activeHook = safetyhook::create_inline(activeEntry.get_first(), Active, SafetyHookInline::StartDisabled);
        selectHook = safetyhook::create_inline(selectEntry.get_first(), Select, SafetyHookInline::StartDisabled);
        modeHook = safetyhook::create_inline(modeEntry.get_first(), SelectMode, SafetyHookInline::StartDisabled);
        if (!countHook || !resolutionHook || !activeHook || !selectHook || !modeHook ||
            !countHook.enable() || !resolutionHook.enable() || !activeHook.enable() || !selectHook.enable() || !modeHook.enable())
        {
            Disable();
            throw std::runtime_error("Resolution catalog hooks unavailable");
        }
        else
            installed = true;
    }
}

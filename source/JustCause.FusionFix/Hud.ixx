module;
#include <stdafx.h>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include "FixSettings.hpp"

export module Hud;


export namespace JCHud
{
    inline SafetyHookInline parseHook, layoutHook, constructHook, miniMapHook, markerHook;
    inline injector::hook_back<void(__fastcall*)(void*, void*, uint32_t, uint32_t, uint32_t, uint32_t)> nativeClip;
    inline std::mutex mutex;
    struct Root
    {
        void* type;
        int anchor;
        float applied = NAN;
    };
    inline std::unordered_map<void*, Root> roots;
    inline std::unordered_set<void*> menus;
    inline std::atomic<DWORD> menuTick{ 0 };
    inline std::atomic<float> requestedAspect{ 0.0f };
    inline std::atomic<float> screenAspect{ 4.0f / 3.0f };
    inline std::atomic<void*> miniMapType{ nullptr };
    inline thread_local float markerShift = 0.0f;
    inline bool ready = false;

    inline float Offset(float screen, float requested)
    {
        const auto box = requested > 0.0f && std::isfinite(requested) ?
            std::min(screen, std::clamp(requested, 4.0f / 3.0f, 32.0f / 9.0f)) : screen;
        return (box / (4.0f / 3.0f) - 1.0f) * 0.5f;
    }

    inline int Anchor(std::string_view name)
    {
        if (name == "CGuiPlayerHealth" || name == "CGuiEquipObject" || name == "CGuiActionMap" || name == "CGuiActionMap_PC") return -1;
        if (name == "CGuiAltitudeMeter" || name == "CGuiRelations") return 1;
        return 0;
    }

    inline void Refresh()
    {
        const auto value = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        requestedAspect.store(value && std::isfinite(*value) ? *value : 0.0f);
    }

    inline void* __fastcall Construct(void* self, void*)
    {
        {
            const std::lock_guard lock(mutex);
            roots.erase(self);
            menus.erase(self);
        }
        return constructHook.thiscall<void*>(self);
    }

    inline char __fastcall Parse(void* self, void*, void* data)
    {
        const auto result = parseHook.thiscall<char>(self, data);
        auto root = *reinterpret_cast<void**>(static_cast<uint8_t*>(self) + 32);
        if (!root) return result;
        const char* name = nullptr;
        using Name = const char** (__thiscall*)(void*, const char**);
        const auto table = *reinterpret_cast<void***>(self);
        reinterpret_cast<Name>(table[30])(self, &name);
        auto anchor = name ? Anchor(name) : 0;
        // The minimap's backdrop is a plain image anchored to the right side
        if (name && std::string_view(name) == "CGuiImageObject" && (static_cast<uint8_t*>(root)[45] & 0x0F) == 2)
            anchor = 1;
        const std::lock_guard lock(mutex);
        if (anchor) roots[root] = { *reinterpret_cast<void**>(root), anchor };
        else roots.erase(root);
        if (name && std::string_view(name) == "CGuiMenuWindow") menus.insert(root);
        else menus.erase(root);
        return result;
    }

    // The minimap reads its own layout block, which the generic parser never sees
    inline uintptr_t __fastcall MiniMap(void* self, void*, void* data)
    {
        const auto result = miniMapHook.thiscall<uintptr_t>(self, data);
        auto layout = static_cast<uint8_t*>(self) + 232;
        miniMapType.store(*reinterpret_cast<void**>(self));
        const std::lock_guard lock(mutex);
        roots[layout] = { *reinterpret_cast<void**>(layout), 1 };
        return result;
    }

    // Map tiles and markers are clipped to a rectangle taken from the minimap's unshifted position
    inline void __fastcall Marker(void* self, void*, float* position, void* marker, int a, char centered, int b)
    {
        const auto previous = markerShift;
        markerShift = *reinterpret_cast<void**>(self) == miniMapType.load() ? Offset(screenAspect.load(), requestedAspect.load()) : 0.0f;
        markerHook.thiscall<void>(self, position, marker, a, centered, b);
        markerShift = previous;
    }

    inline void __fastcall Clip(void* marker, void* edx, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        if (markerShift != 0.0f)
            x = std::bit_cast<uint32_t>(std::bit_cast<float>(x) + markerShift);
        nativeClip.fun(marker, edx, x, y, width, height);
    }

    inline void __fastcall Layout(void* self, void*)
    {
        int anchor = 0;
        {
            const std::lock_guard lock(mutex);
            if (const auto found = roots.find(self); found != roots.end() && found->second.type == *reinterpret_cast<void**>(self))
                anchor = found->second.anchor;
            if (menus.contains(self))
                menuTick.store(GetTickCount());
        }
        if (!anchor)
        {
            layoutHook.thiscall<void>(self);
            return;
        }
        auto position = reinterpret_cast<float*>(static_cast<uint8_t*>(self) + 20);
        auto flags = static_cast<uint8_t*>(self) + 44;
        const auto original = *position;
        const auto offset = anchor * Offset(screenAspect.load(), requestedAspect.load());
        const bool dirty = (*flags & 4) != 0;
        // Recompute the whole native subtree from its authored position on every pass.
        injector::WriteMemory<float>(position, original + offset, true);
        injector::WriteMemory<uint8_t>(flags, *flags | 4, true);
        layoutHook.thiscall<void>(self);
        injector::WriteMemory<float>(position, original, true);
        // Plain images test their dirty flag before laying out, so a changed offset reaches them on the next draw
        const std::lock_guard lock(mutex);
        if (const auto found = roots.find(self); found != roots.end())
        {
            if (found->second.applied != offset)
                injector::WriteMemory<uint8_t>(flags, *flags | 4, true);
            if (dirty)
                found->second.applied = offset;
        }
    }

    inline bool MenuShown()
    {
        return GetTickCount() - menuTick.load() < 250;
    }

    inline void Install()
    {
        Refresh();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", Refresh);
        auto parse = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 81 EC 04 01 00 00 89 8D 00 FF FF FF 8B 85 00 FF FF FF 83 78 20 00 75 05");
        auto layout = hook::pattern("83 EC 08 56 8B F1 F6 46 2C 02 75 09 C6 46 30 05 5E 83 C4 08 C3");
        auto miniMap = hook::pattern("55 8B EC 81 EC A4 00 00 00 89 8D 60 FF FF FF C7 45 E8 CD CC 4C 3F C7 45 EC CD CC 4C 3F C7 45 E0 CD CC 4C 3D C7 45 E4 B8 1E 85 3D");
        auto marker = hook::pattern("55 8B EC 83 EC 6C 89 4D 94 8B 45 94 05 E8 00 00 00 89 45 B4");
        auto clip = hook::pattern("52 8B 4D 0C E8 ? ? ? ? 8B 45 CC 50 8B 4D C8 51 8B 4D 0C");
        auto construct = hook::pattern("6A 5C E8 ? ? ? ? 83 C4 04 89 45 EC C6 45 FC 06 83 7D EC 00 74 0D 8B 4D EC E8 ? ? ? ? 89 45 C0");
        auto call = construct.get_first<uint8_t>(26);
        auto target = call + 5 + *reinterpret_cast<int32_t*>(call + 1);
        auto parseSite = parse.get_first();
        auto miniMapSite = miniMap.get_first();
        auto markerSite = marker.get_first();
        auto clipSite = clip.get_first(4);
        auto layoutSite = layout.get_first();
        constructHook = safetyhook::create_inline(target, Construct);
        if (constructHook) parseHook = safetyhook::create_inline(parseSite, Parse);
        if (parseHook) miniMapHook = safetyhook::create_inline(miniMapSite, MiniMap);
        if (miniMapHook) markerHook = safetyhook::create_inline(markerSite, Marker);
        if (markerHook) nativeClip.fun = injector::MakeCALL(clipSite, Clip, true).get();
        if (nativeClip.fun) layoutHook = safetyhook::create_inline(layoutSite, Layout);
        ready = constructHook && parseHook && miniMapHook && markerHook && nativeClip.fun && layoutHook;
        if (!ready)
        {
            if (nativeClip.fun) injector::MakeCALL(clipSite, nativeClip.fun, true);
            markerHook.reset();
            miniMapHook.reset();
            layoutHook.reset();
            parseHook.reset();
            constructHook.reset();
            OutputDebugStringA("Just Cause: HUD layout hooks unavailable.\n");
        }
        FixSettings().Availability("MAIN.HudAspectRatioConstraint", [](std::string_view) { return ready; });
    }
}

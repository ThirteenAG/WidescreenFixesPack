module;
#include "stdafx.h"
#include "RTTI.h"

export module HUD;
import ComVars;

namespace
{
    injector::hook_back<void(__thiscall*)(void*)> hbDraw;
    void* (__cdecl* findObject)(const char*);
    void* (__thiscall* getElement)(void*, int);
    void (__thiscall* updateBox)(void*);
    injector::hook_back<void(__thiscall*)(void*, const void*)> hbSubmitMapIcon;
    void** hudManager;
    void** currentDisplay;
    void** application;
    bool* frontendDraw;
    uintptr_t groupVtable, layerVtable, pageVtable, boxVtable, p3dVtable;
    uintptr_t objectVtable, textVtable, spriteVtable, roundedRectangleVtable;
    std::optional<float> constraint;

    struct Drawable
    {
        void* object;
        float x;
        bool box;
        bool p3d = false;
        uint8_t transformFlags = 0;
        std::array<float, 16> transform{};
    };
    std::vector<Drawable> moved;
    std::vector<Drawable> group;

    void* Find(const char* name)
    {
        return name ? findObject(name) : nullptr;
    }

    void* Element(void* manager, int id)
    {
        auto element = getElement(manager, id);
        // Resource pointers can outlive a modal page. Only visit visible HUD elements.
        return element && (Game::Field<uint8_t>(element, 0x60) & 0x10) ? element : nullptr;
    }

    void Collect(void* object, bool map = false, unsigned depth = 0)
    {
        if (!object || depth > 8) return;
        if (std::any_of(group.begin(), group.end(), [&](const auto& item) { return item.object == object; }))
            return;
        const auto vtable = Game::Field<uintptr_t>(object, 0);
        if (vtable == groupVtable || vtable == layerVtable || vtable == pageVtable)
        {
            const auto count = Game::Field<uint16_t>(object, 0x38);
            auto children = Game::Field<void**>(object, 0x30);
            for (unsigned i = 0; children && i < count; ++i)
                Collect(children[i], map, depth + 1);
        }
        else if (vtable == boxVtable) // FEBoxObject stores its position in its polygon.
            group.push_back({ object, static_cast<float>(Game::Field<int16_t>(object, 0x30)), true });
        else if (vtable == p3dVtable) // Map geometry moves with its viewport.
        {
            if (map) return;
            if (auto drawable = Game::Field<void*>(object, 0x20))
            {
                Drawable item{ object, Game::Field<float>(drawable, 0x18), false, true };
                item.transformFlags = Game::Field<uint8_t>(drawable, 0x94);
                memcpy(item.transform.data(), static_cast<uint8_t*>(drawable) + 0x48, sizeof(item.transform));
                group.push_back(item);
            }
        }
        else if (vtable == objectVtable || vtable == textVtable || vtable == spriteVtable || vtable == roundedRectangleVtable)
        {
            if (auto drawable = Game::Field<void*>(object, 0x20))
                group.push_back({ object, Game::Field<float>(drawable, 0x18), false });
        }
    }

    void Position(const Drawable& item, float x)
    {
        if (item.p3d)
        {
            auto drawable = Game::Field<void*>(item.object, 0x20);
            auto transform = item.transform;
            auto flags = item.transformFlags;
            const float offset = x - item.x;
            if (offset != 0.0f)
            {
                // 674F60 renders the matrix at +48h, not Drawable::mX.
                // 6798E0 scales these meshes by 0.6886667 after the 640x480 HUD transform.
                if (!(flags & 8))
                {
                    transform = {};
                    transform[0] = transform[5] = transform[10] = transform[15] = 1.0f;
                }
                transform[12] += offset / 0.6886667013168335f;
                flags |= 8;
            }
            memcpy(static_cast<uint8_t*>(drawable) + 0x48, transform.data(), sizeof(transform));
            Game::Field<uint8_t>(drawable, 0x94) = flags;
        }
        else if (item.box)
        {
            Game::Field<int16_t>(item.object, 0x30) = static_cast<int16_t>(std::lround(x));
            updateBox(item.object);
        }
        else
        {
            auto drawable = Game::Field<void*>(item.object, 0x20);
            Game::Field<float>(drawable, 0x18) = x;
            // Rebuild the renderer's cached geometry just as FEObject::SetPosition does.
            auto update = Game::Field<void(__thiscall*)(void*)>(Game::Field<void*>(drawable, 0), 0x20);
            update(drawable);
        }
    }

    void Shift(float distance, std::optional<float> fixedDirection = {})
    {
        if (group.empty()) return;
        float center = 0.0f;
        size_t anchors = 0;
        // Mesh vertices are authored around their own origin; their unused mX is not a screen anchor.
        for (const auto& item : group)
        {
            if (!item.p3d) { center += item.x; ++anchors; }
        }
        if (!anchors)
        {
            for (const auto& item : group) center += item.x;
            anchors = group.size();
        }
        center /= anchors;
        const float offset = distance * fixedDirection.value_or(center < 320.0f ? -1.0f : 1.0f);
        for (const auto& item : group)
        {
            if (std::none_of(moved.begin(), moved.end(), [&](const auto& old) { return old.object == item.object; }))
            {
                moved.push_back(item);
                Position(item, item.x + offset);
            }
        }
        group.clear();
    }

    float Offset(void* display)
    {
        const float width = static_cast<float>(Game::Field<int>(display, 0x1EC));
        const float height = static_cast<float>(Game::Field<int>(display, 0x1E8));
        if (width <= 0.0f || height <= 0.0f) return 0.0f;
        const float aspect = width / height;
        if (aspect <= 4.0f / 3.0f) return 0.0f;
        float target = aspect;
        if (constraint && std::isfinite(*constraint))
        {
            // Also accept the repository's pixel-width form, e.g. 1920x1080.
            target = ClampHudAspectRatio(*constraint > 32.0f / 9.0f ? *constraint / height : *constraint, aspect);
        }
        // Layer::Display (67A910) fits 640x480 and applies its native 448/480 scale.
        const float pixelsPerUnit = std::min(width / 640.0f, height / 480.0f) * (448.0f / 480.0f);
        return height * (target - 4.0f / 3.0f) * 0.5f / pixelsPerUnit;
    }

    float MapOffset(void* map, void* display)
    {
        // UpdateCamera clamps the map width to [0,640]; the shipped minimap is 90 units.
        const float width = Game::Field<int16_t>(map, 0x8C);
        return Offset(display) * std::clamp((640.0f - width) / 550.0f, 0.0f, 1.0f);
    }

    void __fastcall SubmitMapIcon(void* sprite, void*, const void* instance)
    {
        // Waypoint sprites have per-instance coordinates, independent of FEObject positions.
        std::array<uint32_t, 4> data;
        memcpy(data.data(), instance, sizeof(data));
        auto manager = *hudManager;
        auto display = *currentDisplay;
        if (manager && display)
        {
            if (auto map = Element(manager, 27))
                Game::Field<float>(data.data(), 0) -= MapOffset(map, display);
        }
        hbSubmitMapIcon.fun(sprite, data.data());
    }

    void __fastcall Draw(void* handler, void*)
    {
        auto manager = *hudManager;
        auto display = *currentDisplay;
        auto app = *application;
        if (!manager || !display || !app || !*frontendDraw)
            return hbDraw.fun(handler);
        const float offset = Offset(display);
        if (offset == 0.0f) return hbDraw.fun(handler);

        moved.clear();
        group.clear();
        // HUDMeterWithIcon::RelinkResources: select the meter's own layer, never the shared HUD page.
        for (int id : { 1, 2, 3, 4 }) // Health, balls, gang heat, cop heat.
        {
            if (auto element = Element(manager, id))
            {
                Collect(Game::Field<void*>(element, 0x70));
                Collect(Game::Field<void*>(element, 0x78));
                Collect(Find(Game::Field<const char*>(element, 0x6C)));
                Shift(offset);
            }
        }
        if (auto weapon = Element(manager, 14))
        {
            for (size_t member : { 0x80, 0x84, 0x88, 0x8C })
                Collect(Game::Field<void*>(weapon, member));
            Shift(offset);
        }
        for (int id : { 15, 16, 17 }) // HUDTrackable: dirty cash, clean cash, product.
        {
            if (auto element = Element(manager, id))
            {
                // Each trackable page also owns a gradient sprite, alongside its text and icon.
                // MoneyLayer, CleanMoneyLayer and ProductLayer are separate pages in ingame.prj.
                Collect(Game::Field<void*>(element, 0x44));
                Shift(offset);
            }
        }

        const float mapLeft = Game::Field<float>(app, 4);
        const float mapRight = Game::Field<float>(app, 8);
        if (auto map = Element(manager, 27))
        {
            const float mapOffset = MapOffset(map, display);
            // The map border is owned by VisibilityHUD, not MapHUD2. ingame.prj places
            // SC_visibilityMeterFrame and map_border together under this separate page.
            if (auto visibility = Element(manager, 35))
            {
                Collect(Game::Field<void*>(visibility, 0x44));
                Shift(mapOffset, -1.0f);
            }
            // These are the resources owned by MapHUD2::RelinkResources, not its shared page.
            for (const auto name : { "HUDMapBacking", "TonyDirection", "DL_NorthIcon", "MapDescriptorTxt",
                                    "DL_MapDescriptorBack", "DL_map circle" })
                Collect(Find(name), true);
            Shift(mapOffset, -1.0f);
            // 67A910 converts these normalized project coordinates into the 3D map viewport.
            Game::Field<float>(app, 4) -= mapOffset / 640.0f;
            Game::Field<float>(app, 8) -= mapOffset / 640.0f;
        }

        hbDraw.fun(handler);
        Game::Field<float>(app, 4) = mapLeft;
        Game::Field<float>(app, 8) = mapRight;
        for (auto it = moved.rbegin(); it != moved.rend(); ++it)
            Position(*it, it->x);
        // All logical positions are restored before the next update, animation or map clamp.
    }
}

class HUD
{
public:
    HUD()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            auto szHudAspectRatioConstraint = iniReader.ReadString("MAIN", "HudAspectRatioConstraint", "Auto");

            constraint = ParseWidescreenHudOffset(szHudAspectRatioConstraint);
            moved.reserve(256);
            group.reserve(128);

            auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 8D 4C 24 ? 8B F8 ? ? 51");
            findObject = reinterpret_cast<decltype(findObject)>(injector::GetBranchDestination(pattern.get_first()).as_int());

            pattern = hook::pattern("E8 ? ? ? ? 8B C8 E8 ? ? ? ? 84 C0 74 ? E8 ? ? ? ? 84 C0 74 ? F6 86");
            getElement = reinterpret_cast<decltype(getElement)>(injector::GetBranchDestination(pattern.get_first()).as_int());

            pattern = hook::pattern("E8 ? ? ? ? 80 7E ? ? 74 ? 8B 4E ? ? ? FF 50 ? 5E C2 ? ? E8");
            updateBox = reinterpret_cast<decltype(updateBox)>(injector::GetBranchDestination(pattern.get_first()).as_int());

            pattern = hook::pattern("8B 0D ? ? ? ? 6A ? E8 ? ? ? ? 8B C8 E8 ? ? ? ? 84 C0 74 ? E8 ? ? ? ? 84 C0 74 ? F6 86");
            hudManager = *pattern.get_first<void**>(2);

            pattern = hook::pattern("8B 15 ? ? ? ? 8B B2");
            currentDisplay = *pattern.get_first<void**>(2);

            pattern = hook::pattern("8B 35 ? ? ? ? 85 F6 74 ? 56");
            application = *pattern.get_first<void**>(2);

            pattern = hook::pattern("C6 05 ? ? ? ? ? E8 ? ? ? ? 6A ? 6A ? E8");
            frontendDraw = *pattern.get_first<bool*>(2);

            // RTTI
            groupVtable = ScarfaceRTTI::FindVtable(".?AVFEGroupObject@@");
            layerVtable = ScarfaceRTTI::FindVtable(".?AVFELayerObject@@");
            pageVtable = ScarfaceRTTI::FindVtable(".?AVFEPageObject@@");
            boxVtable = ScarfaceRTTI::FindVtable(".?AVFEBoxObject@@");
            p3dVtable = ScarfaceRTTI::FindVtable(".?AVFEP3DObject@@");
            objectVtable = ScarfaceRTTI::FindVtable(".?AVFEObject@@");
            textVtable = ScarfaceRTTI::FindVtable(".?AVFETextObject@@");
            spriteVtable = ScarfaceRTTI::FindVtable(".?AVFESpriteObject@@");
            roundedRectangleVtable = ScarfaceRTTI::FindVtable(".?AVFERoundedRectangleObject@@");

            const auto drawHandlerVtable = ScarfaceRTTI::FindVtable(".?AVDrawHandler@FrontEndManager@@");

            if (!drawHandlerVtable || !groupVtable || !layerVtable || !pageVtable || !boxVtable || !p3dVtable || !objectVtable || !textVtable || !spriteVtable || !roundedRectangleVtable)
                return;

            pattern = hook::pattern("8B 48 ? E8 ? ? ? ? 5F 5E 83 C4");
            hbSubmitMapIcon.fun = injector::MakeCALL(pattern.get_first(3), SubmitMapIcon, true).get();

            pattern = hook::pattern("E8 ? ? ? ? 80 7E ? ? 0F 84 ? ? ? ? 8B 7C 24");
            hbSubmitMapIcon.fun = injector::MakeCALL(pattern.get_first(), SubmitMapIcon, true).get();

            pattern = hook::pattern("E8 ? ? ? ? 80 7E ? ? 74 ? 8B 4E ? ? ? 55");
            hbSubmitMapIcon.fun = injector::MakeCALL(pattern.get_first(), SubmitMapIcon, true).get();

            hbDraw.fun = injector::ReadMemory<decltype(hbDraw.fun)>(drawHandlerVtable, true);
            injector::WriteMemory(drawHandlerVtable, &Draw, true);
        };
    }
} HUD;

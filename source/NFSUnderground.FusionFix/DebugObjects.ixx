module;

#include <stdafx.h>
#include <usercall.hpp>
#include <unordered_set>

export module DebugObjects;

import ComVars;

// The menus' development tags: red blocks with labels (back, next, quit...) at the sides of the screens, outside the
// 4:3 area, so they show on wider screens. Their labels have the default name of a text object, the font's name,
// which real texts of a few screens have too; on those screens only the blocks are hidden.
namespace DevelopmentTags
{
    // The front end's name hash, case-insensitive
    constexpr uint32_t NameHash(std::string_view name)
    {
        uint32_t hash = 0xFFFFFFFF;
        for (char c : name)
            hash = hash * 33 + static_cast<uint8_t>(c >= 'a' && c <= 'z' ? c - 32 : c);
        return hash;
    }

    constexpr std::string_view Blocks[] = {
        "QUIT_TAG", "QUITTAG", "BACK_TAG", "BACKTAG", "NEXT_TAG", "NEXTTAG", "DISPLAYMENUBAR", "DISPLAY_MENUBAR_",
        "DeleteProfile_tag", "CUSTOMIZE_TAG", "CUSTOMIZETAG", "STOCK_TAG", "STOCKTAG", "changecolor_tag", "changecolortag",
        "DecalColor_tag", "DecalColortag", "performance_tag", "performancetag", "defaultcontsettings_tag",
    };

    // Labels: the font's name, and the copies the editor numbered
    constexpr std::string_view Label = "ConduitMdITC_TT21i";
    constexpr int NumberedLabels[] = { 88, 90, 91, 92, 93, 94, 103, 107, 108, 109, 110, 117, 118 };

    // Screens where texts with the label's name are real texts
    constexpr std::string_view RealLabelScreens[] = { "PC_Loading.fng", "LS_Splash_PC.fng", "MU_Online_NetworkLogin.fng", "MU_QuickRaceOptions_PC.fng" };

    constexpr uintptr_t PackageObjects = 0x40;  // FEPackage: first object
    constexpr uintptr_t GroupObjects = 0x60;    // FEGroup: first child
    constexpr uint32_t HiddenFlag = 1;

    std::unordered_set<uint32_t> TagHashes;
    uint32_t LabelHash = 0;
    void (__cdecl* HideObject)(FEObject* object) = nullptr;

    void Init()
    {
        for (auto name : Blocks)
            TagHashes.insert(NameHash(name));
        for (int number : NumberedLabels)
            TagHashes.insert(NameHash(std::string(Label) + "(" + std::to_string(number) + ")"));
        LabelHash = NameHash(Label);
        // The front end's hide of an object (and a group's children)
        auto hide = hook::pattern("8B 4C 24 04 85 C9 56 74 ? 8B 51 1C 8B 41 18 81 CA 01 00 40 00");
        HideObject = reinterpret_cast<decltype(HideObject)>(hide.get_first());
    }

    void HideAll(FEObject* object, bool labels)
    {
        for (; object; object = static_cast<FEObject*>(object->next))
        {
            bool tag = TagHashes.contains(object->NameHash) || (labels && object->NameHash == LabelHash);
            if (tag)
            {
                if (!(object->Flags & HiddenFlag))
                    HideObject(object);
            }
            else if (object->Type == FE_Group)
                HideAll(*reinterpret_cast<FEObject**>(reinterpret_cast<uintptr_t>(object) + GroupObjects), labels);
        }
    }

    void Hide(const char* screen)
    {
        if (!screen)
            return;
        auto package = usercall::CallAndRet<uintptr_t, usercall::reg::eax>(FEPkgMgr_FindPackage, screen);
        if (!package)
            return;
        bool labels = std::none_of(std::begin(RealLabelScreens), std::end(RealLabelScreens), [&](std::string_view name) { return _stricmp(name.data(), screen) == 0; });
        HideAll(*reinterpret_cast<FEObject**>(package + PackageObjects), labels);
    }
}

class DebugObjects
{
public:
    DebugObjects()
    {
        WFP::onInitEvent() += []()
        {
            DevelopmentTags::Init();

            // Every message to a screen, so the tags stay hidden when a screen's script shows them again
            onMenuScreenBaseNotify() += [](MenuScreen* menu)
            {
                DevelopmentTags::Hide(menu->PackageFilename);
            };
        };
    }
} DebugObjects;

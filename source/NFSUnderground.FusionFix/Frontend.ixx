module;

#include <stdafx.h>
#include <usercall.hpp>
#include "FixSettings.hpp"
#include <d3d9.h>

export module Frontend;

import ComVars;
import Resolution;

// The front end and the HUD are authored for a 640x480 canvas. The canvas is scaled to the screen
// height and centred, HUD elements are moved to the screen edges using the layout table, and full
// screen movies are shown whole at their original aspect ratio with black bars, in the game's 4:3 area
// (Original) or across the whole screen (Fill, MAIN.FMVWidescreenMode).

namespace
{
    std::optional<float> fHudAspectRatioConstraint;
    float fWidescreenHudOffset = 120.0f;
    std::atomic<bool> HudLayoutChanged{ false };

    union HudPos
    {
        uint32_t dwPos;
        float fPos;
        HudPos(uint32_t pos) : dwPos(pos) {}
        HudPos(float pos) : fPos(pos) {}
    };

    struct HudLayoutEntry
    {
        uint32_t posX;
        uint32_t posY;
        float offsetX;
        float offsetY;
    };

    std::vector<HudLayoutEntry> HudLayout;

    // The screen size the HUD offset was made for; the offset follows a change of the constraint at once
    float fHudScreenWidth = 640.0f;
    float fHudScreenHeight = 480.0f;

    void UpdateWidescreenHudOffset()
    {
        float width = fHudScreenWidth;
        float height = fHudScreenHeight;
        fWidescreenHudOffset = -CalculateWidescreenOffset(width, height, 640.0f, 480.0f);
        if (fHudAspectRatioConstraint.has_value() && std::isfinite(*fHudAspectRatioConstraint))
        {
            float value = std::min(std::clamp(*fHudAspectRatioConstraint, 4.0f / 3.0f, 32.0f / 9.0f), width / height);
            fWidescreenHudOffset = -CalculateWidescreenOffset(height * value, height, 640.0f, 480.0f);
        }
    }

    // Layout table: original element position (as float bits) and how far to move it for 16:9.
    // Embedded from textures/NFS/NFSU/HudLayout.dat.
    void LoadHudLayout()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&LoadHudLayout), &module);
        HRSRC resource = FindResourceA(module, "HUDLAYOUT", MAKEINTRESOURCEA(10));
        HGLOBAL handle = resource ? LoadResource(module, resource) : nullptr;
        auto text = handle ? static_cast<const char*>(LockResource(handle)) : nullptr;
        if (!text)
            return;

        std::istringstream stream(std::string(text, SizeofResource(module, resource)));
        std::string line;
        while (std::getline(stream, line))
        {
            if (line.empty() || line[0] == '#' || line[0] == '\r')
                continue;

            HudLayoutEntry entry = {};
            if (sscanf_s(line.c_str(), "%*s %x %x %f %f", &entry.posX, &entry.posY, &entry.offsetX, &entry.offsetY) == 4)
                HudLayout.push_back(entry);
        }
    }

    std::vector<ProtectedGameRef<float>> HudPosXRefs;
    std::vector<FEObject*> ScreenBackdropImages;

    void ExtendGradientImages(FEObject* group, float extra, bool quickRaceOptions)
    {
        for (auto child = *reinterpret_cast<FEObject**>(reinterpret_cast<uintptr_t>(group) + 0x60); child;
            child = reinterpret_cast<FEObject*>(child->next))
        {
            if (child->Type == FE_Group)
                ExtendGradientImages(child, extra, quickRaceOptions);
            else if (child->Type == FE_Image && child->pData && child->DataSize >= 64 &&
                (child->NameHash == 0x30935DFF || child->NameHash == 0xD408C101))
            {
                float originalWidth = quickRaceOptions && (child->GUID == 0xA766 || child->GUID == 0xA767) ? 789.0f : 640.0f;
                float width = originalWidth + extra + (extra > 0.0f ? 128.0f : 0.0f);
                auto sizeX = reinterpret_cast<float*>(child->pData + 56);
                if (*sizeX != width)
                {
                    *sizeX = width;
                    child->Flags |= 0x400000;
                }
                ScreenBackdropImages.push_back(child);
            }
        }
    }

    void UpdateScreenBackdrops()
    {
        ScreenBackdropImages.clear();
        struct Backdrop
        {
            const char* package;
            uint32_t name;
            float width;
        };
        static constexpr Backdrop backdrops[] =
        {
            { "MU_PauseConfirm.fng", 0x88C7F0CE, 721.333313f },   // Backing
            { "MU_PauseConfirm.fng", 0xD6B42B95, 721.333313f },   // Backing(114)
            { "MU_PostRaceConfirm.fng", 0x59C734F0, 737.333252f }, // ScreenBack
            { "MU_PostRaceConfirm.fng", 0x0D2D5518, 737.333252f }, // ScreenBack(115)
            { "PostRaceStats.fng", 0x8B66200D, 746.666748f },     // BlackScreenBack
            { "RewardPopup.fng", 0x8B66200D, 746.666687f },
            { "StyleAnimator.fng", 0x8B66200D, 746.666687f },
            { "MU_UG_InGameDialogBox.fng", 0x88C7F0CE, 640.0f },
            { "MU_ONLINE_WAITING.fng", 0x88C7F0CE, 721.333313f },
        };
        // Solid white-texture backdrops cover the screen, independently of the HUD box.
        // Preserve each backdrop's authored overscan on the 640x480 canvas.
        float extra = std::max(0.0f, fHudScreenWidth / fHudScreenHeight * 480.0f - 640.0f);
        for (auto& backdrop : backdrops)
        {
            auto package = usercall::CallAndRet<uintptr_t, usercall::reg::eax>(FEPkgMgr_FindPackage, backdrop.package);
            if (!package)
                continue;
            auto object = reinterpret_cast<FEObject*>(usercall::CallAndRet<uintptr_t, usercall::reg::edx, usercall::reg::ecx>(FEngFindObject, package, backdrop.name));
            if (object && object->Type == FE_Image && object->pData && object->DataSize >= 64)
            {
                ScreenBackdropImages.push_back(object);
                auto sizeX = reinterpret_cast<float*>(object->pData + 56);
                float width = backdrop.width + extra;
                if (*sizeX != width)
                {
                    *sizeX = width;
                    object->Flags |= 0x400000;
                }
            }
        }
        // Full-screen gradients are image children of these groups. Group scale is not applied by the
        // front-end renderer, so extend the images themselves and keep the dialog controls unchanged.
        for (auto name : { "GenericDialog.fng", "GenericOKDialog.fng", "GenericOKDialogMedium.fng",
            "MU_UG_Explained.fng", "MU_GarageVinylPrompt.fng", "MU_QuickRaceOptions.fng",
            "MU_QuickRaceOptions_PC.fng", "MU_QRTransmissionSelect.fng", "MU_QuickRaceLapPopup.fng",
            "MU_QuickRaceLapPopup_PC.fng", "MU_MultiTrackConfirm.fng", "MU_UG_TradeCarConfirm.fng",
            "MU_DemoCustomBlurb.fng", "MU_OnlineGamesRoomDialog.fng", "MU_Online_WarningDialog.fng",
            "MU_Online4PlayerGameDialogueBox.fng", "MU_Online_OK_Dialog.fng", "PC_customize_graphics.fng",
            "DiscErrorPC.fng", "HELP_Generic.fng" })
        {
            auto package = usercall::CallAndRet<uintptr_t, usercall::reg::eax>(FEPkgMgr_FindPackage, name);
            if (!package)
                continue;
            auto object = reinterpret_cast<FEObject*>(usercall::CallAndRet<uintptr_t, usercall::reg::edx, usercall::reg::ecx>(FEngFindObject, package, 0x853AB49E));
            if (object && object->Type == FE_Group)
                ExtendGradientImages(object, extra, !_stricmp(name, "MU_QuickRaceOptions.fng"));
        }
    }
}

export bool IsScreenBackdropImage(const FEObject* object)
{
    return std::find(ScreenBackdropImages.begin(), ScreenBackdropImages.end(), object) != ScreenBackdropImages.end();
}

// An object the fix adds to an in-game menu, such as the pause menu's "Plugins" row: it's moved for the
// screen's aspect ratio like the object it was copied from. The layout table matches objects by their position.
export void AddHudLayoutCopy(float fromX, float fromY, float toX, float toY)
{
    auto bits = [](float value) { uint32_t result; memcpy(&result, &value, sizeof(result)); return result; };
    auto it = std::find_if(HudLayout.begin(), HudLayout.end(), [&](const HudLayoutEntry& e) { return e.posX == bits(fromX) && e.posY == bits(fromY); });
    if (it == HudLayout.end())
        return;
    auto copy = *it;
    copy.posX = bits(toX);
    copy.posY = bits(toY);
    if (std::none_of(HudLayout.begin(), HudLayout.end(), [&](const HudLayoutEntry& e) { return e.posX == copy.posX && e.posY == copy.posY; }))
        HudLayout.push_back(copy);
}

namespace
{
    float fMinimapPosX = 320.0f;
    float fMinimapPosY = 240.0f;

    struct MinimapPosition
    {
        uint32_t originalX, originalY;
        uint32_t writtenX, writtenY;
    };
    std::unordered_map<uintptr_t, MinimapPosition> MinimapPositions;

    void WidescreenHud(HudPos& x, HudPos& y, bool minimap = false)
    {
        if (IsFrontEnd())
            return;

        auto it = std::find_if(HudLayout.begin(), HudLayout.end(), [&](const HudLayoutEntry& e)
        {
            return e.posX == x.dwPos && e.posY == y.dwPos;
        });

        if (it == HudLayout.end())
            return;

        constexpr auto fBaseOffset = (480.0f * (16.0f / 9.0f) - 640.0f) / 2.0f;
        if (it->offsetX >= 0.0f)
            x.fPos += fWidescreenHudOffset + (it->offsetX - fBaseOffset);
        else
            x.fPos -= fWidescreenHudOffset - (it->offsetX + fBaseOffset);

        y.fPos += it->offsetY;

        if (minimap)
        {
            x.fPos -= fMinimapPosX;
            x.fPos += HudPosXRefs.front();
        }
    }

    void WidescreenMinimap(HudPos& x, HudPos& y, uintptr_t position)
    {
        auto previous = MinimapPositions.find(position);
        if (previous != MinimapPositions.end() && x.dwPos == previous->second.writtenX && y.dwPos == previous->second.writtenY)
        {
            x.dwPos = previous->second.originalX;
            y.dwPos = previous->second.originalY;
        }
        MinimapPosition saved{ x.dwPos, y.dwPos, 0, 0 };
        WidescreenHud(x, y, true);
        saved.writtenX = x.dwPos;
        saved.writtenY = y.dwPos;
        MinimapPositions[position] = saved;
    }

    void AdjustLapsY(SafetyHookContext& regs)
    {
        HudPos x = *(uint32_t*)(regs.ebx + 0x1C);
        HudPos y = *(uint32_t*)(regs.ebx + 0x20);
        float originalY = y.fPos;
        WidescreenHud(x, y);
        float offsetY = y.fPos - originalY;
        _asm fadd dword ptr[offsetY]
    }
}

// Movies
namespace
{
    bool IsPowerOfTwo(int value)
    {
        return value > 0 && (value & (value - 1)) == 0;
    }

    // 0 original aspect, 1 fill the screen

    struct MovieQuad
    {
        void* renderObject;
        float x0, y0, x1, y1, z;
        float u0, v0, u1, v1;
    };

    void FitMovie(MovieQuad& quad, int frameWidth, int frameHeight)
    {
        // Object transform in canvas units, copied to the render object by the game
        auto matrix = reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(quad.renderObject) + 0x20);
        float objectWidth = std::fabs(matrix[0]);
        float objectHeight = std::fabs(matrix[5]);
        if (objectWidth < 1.0f || objectHeight < 1.0f || frameWidth <= 0 || frameHeight <= 0)
            return;

        // The game's own movies are anamorphic: power-of-two frames made for a 4:3 screen. The 2:1 frames
        // contain a 16:9 picture with black bars, which are cropped.
        // Replacement movies with other sizes are shown with square pixels.
        bool fullScreen = objectWidth >= 600.0f && objectHeight >= 450.0f;

        float contentAspect;
        float crop = 0.0f;
        if (IsPowerOfTwo(frameWidth) && IsPowerOfTwo(frameHeight))
        {
            contentAspect = fullScreen ? 4.0f / 3.0f : objectWidth / objectHeight;
            if (frameWidth == frameHeight * 2)
            {
                crop = 0.125f;
                contentAspect /= 1.0f - 2.0f * crop;
            }
        }
        else
            contentAspect = float(frameWidth) / float(frameHeight);

        // Full screen movies use the whole screen, others keep their box
        float boxWidth = fullScreen ? 480.0f * GetAspectRatio() : objectWidth;
        float boxHeight = fullScreen ? 480.0f : objectHeight;

        // Original keeps full screen movies in the game's centred 4:3 area, Fill uses the whole screen
        static const auto& movieMode = FixSettings().Int("MAIN.FMVWidescreenMode");
        if (fullScreen && movieMode == 0)
        {
            boxWidth = std::min(boxWidth, boxHeight * 4.0f / 3.0f);
            boxHeight = std::min(boxHeight, boxWidth * 3.0f / 4.0f);
        }

        float width = boxWidth;
        float height = boxWidth / contentAspect;
        if (height > boxHeight)
        {
            height = boxHeight;
            width = boxHeight * contentAspect;
        }

        float halfX = 0.5f * width / objectWidth;
        float halfY = 0.5f * height / objectHeight;
        quad.x0 = -halfX;
        quad.y0 = -halfY;
        quad.x1 = halfX;
        quad.y1 = halfY;

        float v = quad.v1 - quad.v0;
        quad.v0 += v * crop;
        quad.v1 -= v * crop;

        // Black bars around a playing full screen movie. Menus also contain movie objects that aren't playing.
        if (fullScreen && MovieIsPlaying())
        {
            if (auto device = Direct3DDevice.get())
                device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        }
    }
}

class Frontend
{
public:
    Frontend()
    {
        WFP::onInitEvent() += []()
        {
            fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));

            LoadHudLayout();

            WFP::onGameProcessEvent() += UpdateScreenBackdrops;
            WFP::onGameProcessEvent() += []()
            {
                if (IsFrontEnd())
                    MinimapPositions.clear();
                if (HudLayoutChanged.exchange(false) && cFEng::pInstance && *cFEng::pInstance)
                    cFEng::MakeLoadedPackagesDirty(*cFEng::pInstance);
            };

            static ProtectedGameRef<float> fHudScaleX;
            fHudScaleX.SetAddress(*hook::pattern("D8 0D ? ? ? ? 57 0F B6 B8").count(1).get(0).get<float*>(2));

            auto addRefs = [](hook::pattern pattern, ptrdiff_t offset)
            {
                if (pattern.empty())
                {
                    (void)hook::pattern(pattern).get_first();
                    std::abort();
                }
                pattern.for_each_result([&](hook::pattern_match match)
                {
                    ProtectedGameRef<float> ref;
                    ref.SetAddress(match.get<float>(offset));
                    HudPosXRefs.push_back(std::move(ref));
                });
            };

            // Canvas centre X (320.0) used by the HUD
            addRefs(hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"), 7);
            auto russianHud = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43");
            if (!russianHud.empty()) // Russian 1.4
                addRefs(russianHud, 4);
            else // US 1.4
            {
                auto us = hook::pattern("E8 ? ? ? ? 83 C4 04 85 C0 76 21 8D 43 60");
                if (us.empty())
                {
                    us.get_first();
                    std::abort();
                }
            }
            addRefs(hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43"), 4);
            {
                ProtectedGameRef<float> ref;
                ref.SetAddress(*hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 0F D9 45 FC").count(1).get(0).get<float*>(2));
                HudPosXRefs.push_back(std::move(ref));
            }

            static ProtectedGameRef<float> fRearviewMirrorLeftX1;
            static ProtectedGameRef<float> fRearviewMirrorLeftX2;
            static ProtectedGameRef<float> fRearviewMirrorRightX1;
            static ProtectedGameRef<float> fRearviewMirrorRightX2;

            auto pattern = hook::pattern("C7 44 24 ? 00 00 DC 43 C7 44 24 ? 00 00 70 41");
            if (!pattern.empty()) // US 1.4
            {
                fRearviewMirrorLeftX1.SetAddress(pattern.get_first<float>(20));
                fRearviewMirrorLeftX2.SetAddress(pattern.get_first<float>(42));
                fRearviewMirrorRightX1.SetAddress(pattern.get_first<float>(4));
                fRearviewMirrorRightX2.SetAddress(pattern.get_first<float>(64));
            }
            else // Russian 1.4
            {
                pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? FF 91");
                fRearviewMirrorLeftX1.SetAddress(pattern.get_first<float>(26));
                fRearviewMirrorLeftX2.SetAddress(pattern.get_first<float>(48));
                fRearviewMirrorRightX1.SetAddress(pattern.get_first<float>(4));
                fRearviewMirrorRightX2.SetAddress(pattern.get_first<float>(70));
            }

            // Front end mouse cursor: canvas left edge (-320) in screen to canvas conversions
            static std::vector<ProtectedGameRef<int32_t>> CursorOffsetRefs;
            auto addCursorRefs = [](hook::pattern pattern, ptrdiff_t offset)
            {
                if (pattern.empty())
                {
                    (void)hook::pattern(pattern).get_first();
                    std::abort();
                }
                pattern.for_each_result([&](hook::pattern_match match)
                {
                    ProtectedGameRef<int32_t> ref;
                    ref.SetAddress(match.get<int32_t>(offset));
                    CursorOffsetRefs.push_back(std::move(ref));
                });
            };
            addCursorRefs(hook::pattern("81 C2 C0 FE FF FF"), 2);
            addCursorRefs(hook::pattern("05 C0 FE FF FF"), 1);
            addCursorRefs(hook::pattern("8D 88 C0 FE FF FF"), 2);

            pattern = hook::pattern("8B 06 8B 4C 24 08");
            struct ClientRectHook1
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.eax = *(uint32_t*)(regs.esi + 0x00);
                    regs.ecx = static_cast<uint32_t>(HudPosXRefs.front() + HudPosXRefs.front());
                }
            }; injector::MakeInline<ClientRectHook1>(pattern.get_first(0), pattern.get_first(6));

            pattern = hook::pattern("8B 07 8B 4C 24 0C");
            struct ClientRectHook2
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.eax = *(uint32_t*)(regs.edi + 0x00);
                    regs.ecx = 480;
                }
            }; injector::MakeInline<ClientRectHook2>(pattern.get_first(0), pattern.get_first(6));

            onResChange() += [](int Width, int Height)
            {
                float width = static_cast<float>(Width);
                float height = static_cast<float>(Height);

                fHudScaleX = (1.0f / width * (height / 480.0f)) * 2.0f;
                float fHudPosX = 640.0f / (640.0f * fHudScaleX);

                for (auto& pos : HudPosXRefs)
                    pos = fHudPosX;

                for (auto& pos : CursorOffsetRefs)
                    pos = static_cast<int32_t>(-fHudPosX);

                fRearviewMirrorLeftX1 = (fHudPosX - 320.0f) + 200.0f;
                fRearviewMirrorLeftX2 = (fHudPosX - 320.0f) + 200.0f;
                fRearviewMirrorRightX1 = (fHudPosX - 320.0f) + 440.0f;
                fRearviewMirrorRightX2 = (fHudPosX - 320.0f) + 440.0f;

                fHudScreenWidth = width;
                fHudScreenHeight = height;
                UpdateWidescreenHudOffset();
            };

            FixSettings().OnChange("MAIN.HudAspectRatioConstraint", []()
            {
                fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
                UpdateWidescreenHudOffset();
                HudLayoutChanged = true;
            });

            pattern = hook::pattern("8B 47 1C 8B 4F 20 89 84 24 10 01 00 00 8B 47 24 89 84 24 18 01 00 00 39 5C 24 04");
            {
                struct HudHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.eax = *(uint32_t*)(regs.edi + 0x1C);
                        regs.ecx = *(uint32_t*)(regs.edi + 0x20);
                        HudPos x = regs.eax;
                        HudPos y = regs.ecx;
                        WidescreenHud(x, y);
                        regs.eax = x.dwPos;
                        regs.ecx = y.dwPos;
                    }
                }; injector::MakeInline<HudHook>(pattern.get_first(0), pattern.get_first(6));

                pattern = hook::pattern("D8 05 ? ? ? ? D9 5B 68 D8 05 ? ? ? ? D9 5B 6C D9 40 3C");
                injector::WriteMemory(pattern.get_first(2), &fMinimapPosX, true);
                injector::WriteMemory(pattern.get_first(11), &fMinimapPosY, true);

                pattern = hook::pattern("8B 4D 08 D9 41 68 53 57 8B 5D 0C 8B F8");
                struct MinimapHook1
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.ecx = *(uint32_t*)(regs.ebp + 0x08);
                        HudPos x = *(uint32_t*)(regs.ecx + 0x68);
                        HudPos y = *(uint32_t*)(regs.ecx + 0x6C);
                        WidescreenMinimap(x, y, regs.ecx + 0x68);
                        *(uint32_t*)(regs.ecx + 0x68) = x.dwPos;
                        *(uint32_t*)(regs.ecx + 0x6C) = y.dwPos;
                        _asm fld dword ptr[x.fPos]
                    }
                }; injector::MakeInline<MinimapHook1>(pattern.get_first(0), pattern.get_first(6));

                pattern = hook::pattern("D9 40 68 8B 4D 0C D8 60 70 53 56 8D 91 83 00 00 00 D9 54 24 34");
                struct MinimapHook2
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.ecx = *(uint32_t*)(regs.ebp + 0x0C);
                        HudPos x = *(uint32_t*)(regs.eax + 0x68);
                        HudPos y = *(uint32_t*)(regs.eax + 0x6C);
                        WidescreenMinimap(x, y, regs.eax + 0x68);
                        *(uint32_t*)(regs.eax + 0x68) = x.dwPos;
                        *(uint32_t*)(regs.eax + 0x6C) = y.dwPos;
                        _asm fld dword ptr[x.fPos]
                    }
                }; injector::MakeInline<MinimapHook2>(pattern.get_first(0), pattern.get_first(6));

                pattern = hook::pattern("D9 40 1C 8B 4D 08 D9 40 20");
                struct MinimapHook3
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        HudPos x = *(uint32_t*)(regs.eax + 0x1C);
                        HudPos y = *(uint32_t*)(regs.eax + 0x20);
                        WidescreenHud(x, y, true);
                        // The cached minimap gets this position; the shared FEng state stays authored.
                        regs.ecx = *(uint32_t*)(regs.ebp + 0x8);
                        _asm fld dword ptr[x.fPos]
                        _asm fld dword ptr[y.fPos]
                    }
                }; injector::MakeInline<MinimapHook3>(pattern.get_first(0), pattern.get_first(9));

                pattern = hook::pattern("D8 43 1C 8B 54 24 18 8B 44 24 10 D9 1A 8B 4E 68 50 51 57");
                struct LapsHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.edx = *(uint32_t*)(regs.esp + 0x18);
                        HudPos x = *(uint32_t*)(regs.ebx + 0x1C);
                        HudPos y = *(uint32_t*)(regs.ebx + 0x20);
                        WidescreenHud(x, y);
                        _asm fadd dword ptr[x.fPos]
                    }
                };
                auto yPosition = hook::range_pattern(reinterpret_cast<uintptr_t>(pattern.get_first()), reinterpret_cast<uintptr_t>(pattern.get_first()) + 0x30, "D8 43 20 5B 5F").get_first();
                injector::MakeInline<LapsHook>(pattern.get_first(0), pattern.get_first(7));
                static auto LapsYHook = safetyhook::create_mid(yPosition, AdjustLapsY);
            }

            // cFEngRender::RenderMovie: the movie quad's corners and texture coordinates are pushed for
            // the render object setup call, the movie's texture info is in ebx
            pattern = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 50 E8");
            static auto RenderMovieHook = safetyhook::create_mid(pattern.get_first(26), [](SafetyHookContext& regs)
            {
                auto quad = reinterpret_cast<MovieQuad*>(regs.esp);
                auto texture = regs.ebx;
                if (texture)
                    FitMovie(*quad, *reinterpret_cast<int16_t*>(texture + 0x44), *reinterpret_cast<int16_t*>(texture + 0x46));
            });
        };
    }
} Frontend;

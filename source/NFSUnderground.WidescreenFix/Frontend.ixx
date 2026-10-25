module;

#include <stdafx.h>
#include "../Shared/PatternPreflight.hpp"
#include "FixSettings.hpp"
#include <d3d9.h>

export module Frontend;

import ComVars;
import Resolution;

// The front end and the HUD are authored for a 640x480 canvas. The canvas is scaled to the screen
// height and centred, HUD elements are moved to the screen edges using the layout table, and full
// screen movies are shown at their original aspect ratio with black bars, fill the screen or are
// stretched to it (MAIN.FMVWidescreenMode).

namespace
{
    std::optional<float> fHudAspectRatioConstraint;
    float fWidescreenHudOffset = 120.0f;

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
        if (fHudAspectRatioConstraint.has_value())
        {
            float value = fHudAspectRatioConstraint.value();
            if (value < 0.0f || value > (32.0f / 9.0f))
                fWidescreenHudOffset = value;
            else
            {
                value = ClampHudAspectRatio(value, width / height);
                fWidescreenHudOffset = -CalculateWidescreenOffset(height * value, height, 640.0f, 480.0f);
            }
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
}

// Movies
namespace
{
    bool IsPowerOfTwo(int value)
    {
        return value > 0 && (value & (value - 1)) == 0;
    }

    // 0 original aspect, 1 fill the screen, 2 stretch
    const int* pFMVWidescreenMode = nullptr;

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
        int mode = fullScreen ? *pFMVWidescreenMode : 0;

        float width = boxWidth;
        float height = boxWidth / contentAspect;
        if (mode == 2)
            height = boxHeight;
        else if (mode == 1 ? height < boxHeight : height > boxHeight)
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
            pFMVWidescreenMode = &FixSettings().Int("MAIN.FMVWidescreenMode");

            LoadHudLayout();

            static ProtectedGameRef<float> fHudScaleX;
            fHudScaleX.SetAddress(*hook::pattern("D8 0D ? ? ? ? 57 0F B6 B8").count(1).get(0).get<float*>(2));

            auto addRefs = [](hook::pattern pattern, ptrdiff_t offset)
            {
                pattern.for_each_result([&](hook::pattern_match match)
                {
                    ProtectedGameRef<float> ref;
                    ref.SetAddress(match.get<float>(offset));
                    HudPosXRefs.push_back(std::move(ref));
                });
            };

            // Canvas centre X (320.0) used by the HUD
            addRefs(hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"), 7);
            addRefs(hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"), 4);
            addRefs(hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43"), 4);
            {
                [&]()
                {
                    if (!PatternPreflight::Check("NFSUnderground Frontend", {
                        { "D8 1D ? ? ? ? DF E0 F6 C4 41 75 0F D9 45 FC", 1, true },
                    }))
                        return;

                    ProtectedGameRef<float> ref;
                    ref.SetAddress(*hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 0F D9 45 FC").count(1).get(0).get<float*>(2));
                    HudPosXRefs.push_back(std::move(ref));
                }();

            }

            static ProtectedGameRef<float> fRearviewMirrorLeftX1;
            static ProtectedGameRef<float> fRearviewMirrorLeftX2;
            static ProtectedGameRef<float> fRearviewMirrorRightX1;
            static ProtectedGameRef<float> fRearviewMirrorRightX2;

            auto pattern = hook::pattern("C7 44 24 ? 00 00 DC 43 C7 44 24 ? 00 00 70 41");
            if (!pattern.empty())
            {
                fRearviewMirrorLeftX1.SetAddress(pattern.get_first<float>(20));
                fRearviewMirrorLeftX2.SetAddress(pattern.get_first<float>(42));
                fRearviewMirrorRightX1.SetAddress(pattern.get_first<float>(4));
                fRearviewMirrorRightX2.SetAddress(pattern.get_first<float>(64));
            }
            else
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
            });

            pattern = hook::pattern("8B 47 1C 8B 4F 20 89 84 24 10 01 00 00 8B 47 24 89 84 24 18 01 00 00 39 5C 24 04");
            if (!pattern.empty())
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
                        WidescreenHud(x, y, true);
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
                        WidescreenHud(x, y, true);
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
                        *(uint32_t*)(regs.eax + 0x1C) = x.dwPos;
                        *(uint32_t*)(regs.eax + 0x20) = y.dwPos;
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
                        *(uint32_t*)(regs.ebx + 0x1C) = x.dwPos;
                        *(uint32_t*)(regs.ebx + 0x20) = y.dwPos;
                        _asm fadd dword ptr[x.fPos]
                    }
                }; injector::MakeInline<LapsHook>(pattern.get_first(0), pattern.get_first(7));
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

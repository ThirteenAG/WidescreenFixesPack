module;

#include <stdafx.h>
#include <d3d9.h>
#include <array>
#include "FixSettings.hpp"

export module Hud;

import ComVars;
import Camera;
import Movie;
import Cinema;
import Window;
import Intro;
import Focus;
import Frame;
import Frontend;

class Hud
{
public:
    Hud()
    {
        WFP::onInitEvent() += []()
        {
            WindowedModeWrapper::EnableDpiAwareness();
            (void)FixSettings();
            CoCWindow::Init();
            CoCIntro::Init();
            CoCFocus::Init();
            asi_settings::LiveLink::Start();

            auto pattern = hook::pattern("8B 4C 24 18 BF 01 00 00 00 C7"); //0x406344

            struct ResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = *(int32_t*)(regs.esp + 0x18);
                    regs.edi = 1;
                    int width = int(regs.ebx), height = int(regs.ecx);
                    CoCWindow::Resolution(width, height);
                    regs.ebx = width;
                    regs.ecx = height;
                    *reinterpret_cast<int32_t*>(regs.esp + 0x18) = height;

                    Screen.Width = regs.ebx;
                    Screen.Height = regs.ecx;
                    Screen.fWidth = static_cast<float>(Screen.Width);
                    Screen.fHeight = static_cast<float>(Screen.Height);
                    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
                    Screen.fHudScale = ((1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));
                    Screen.fHudOffset = ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
                    Screen.fTextOffset = -(Screen.fHudOffset / Screen.fHudScale);
                    Screen.fFMVOffset = ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
                    // Room for quads a third beyond the screen's edges (the inventory places some there)
                    Screen.fQuadUnits = std::min(10.0f, 21000.0f / std::max(Screen.fWidth, Screen.fHeight));
                    CoCCinema::width = Screen.fWidth;
                    CoCCinema::height = Screen.fHeight;
                    CoCCinema::aspect = Screen.fAspectRatio;

                    static auto aspectValues = []()
                    {
                        std::vector<float*> values;
                        auto first = hook::pattern("68 A5 AC A9 3F").get_first<float>(1);
                        values.push_back(first);
                        hook::pattern("68 A5 AC A9 3F").for_each_result([&](hook::pattern_match match)
                        {
                            auto value = match.get<float>(1);
                            if (value != first)
                                values.push_back(value);
                        }); //4352C4 435511 5D7EF6
                        return values;
                    }();
                    for (auto value : aspectValues)
                    {
                        injector::WriteMemory<float>(value, Screen.fAspectRatio, true);
                    }

                    // InventoryMenu - BlackBox (covers top-right line paper)
                    static auto blackBoxPosition = hook::pattern("68 D7 A3 30 3F 6A 00").get_first<uint8_t>(); // 422B1A
                    static auto blackBoxSize = hook::pattern("68 E1 7A 14 3F ? ? ? ? ? ? ? 68 52 B8 9E 3E").get_first<uint8_t>(); // 422B26
                    const uint32_t codePushZeroPush = 0x68006A; // 6A 00 = push 0x00; 68 = push (next value)
                    const float blackBoxWidth = (4.0f / 3.0f) * Screen.fHudOffset / Screen.Width;
                    injector::WriteMemory<uint32_t>(reinterpret_cast<uint32_t*>(blackBoxPosition), codePushZeroPush, true); // y = 0
                    injector::WriteMemory<float>(reinterpret_cast<float*>(blackBoxPosition + 3), 1.0f, true); // x = 1
                    injector::WriteMemory<float>(reinterpret_cast<float*>(blackBoxSize + 1), blackBoxWidth, true); // w
                    injector::WriteMemory<float>(reinterpret_cast<float*>(blackBoxSize + 13), 1.0f, true); // h = 1

                    // InventoryMenu - HeartMonitorLine (positioned on line paper)
                    // NOTE: HeartMonitorLine ended at 103% ScreenWidth (i.e. 3% off-screen) in OG game.
                    static auto heartLineEnd = hook::pattern("D8 0D ? ? ? ? 85 C9 DB").get_first<float*>(2); // 424FD8; [????] = 0.53
                    Screen.fInvMenuHeartLineEnd = Screen.fHudScale * 0.5f; // 50% instead of 53% so line doesn't go past screen edge
                    injector::WriteMemory<float*>(heartLineEnd, &Screen.fInvMenuHeartLineEnd, true);

                    static auto heartLineWidth = hook::pattern("D8 0D ? ? ? ? 8B 44 24 ? 85 C0").get_first<float*>(2); // 425DC8; [????] = 0.000775
                    Screen.fInvMenuHeartLineWidth = Screen.fHudScale * 0.000715f; // Subtracted 0.00006 (3% + 3%)
                    injector::WriteMemory<float*>(heartLineWidth, &Screen.fInvMenuHeartLineWidth, true);

                    // HealMenu - Fix Cursor Boxes
                    // NOTE: Cursor was off by one small grid square in the OG game (fixed)
                    // The constants are shared with other code: the heal menu's instructions read the fix's own copies
                    static auto healCode = hook::pattern("F3 0F 10 05 ? ? ? ? 0F 2F C1 0F 86 ? ? ? ? F3 0F 10 44 24 ? 0F 2F 05 ? ? ? ? 0F 82 ? ? ? ? F3 0F 10 15 ? ? ? ? 0F 2F D0 0F 86 ? ? ? ? 0F 2F 0D ? ? ? ? 72 ? D9 44 24 ? 8B CF D8 25 ? ? ? ? D9 7C 24 ? 0F B7 44 24 ? 80 CC 0C D8 0D").get_first<uint8_t>(); // 42475F
                    static float heal[5]; // box width, cursor x min, y max, y min, x max
                    static const std::array<float, 5> originalHealValues = []()
                    {
                        auto code = healCode;
                        return std::array<float, 5>{ **reinterpret_cast<float**>(code + 0x58), **reinterpret_cast<float**>(code + 0x38),
                            **reinterpret_cast<float**>(code + 0x28), **reinterpret_cast<float**>(code + 0x1A), **reinterpret_cast<float**>(code + 0x04) };
                    }();
                    static const bool redirected = []()
                    {
                        auto code = healCode;
                        injector::WriteMemory<float*>(reinterpret_cast<float**>(code + 0x58), &heal[0], true);
                        injector::WriteMemory<float*>(reinterpret_cast<float**>(code + 0x38), &heal[1], true);
                        injector::WriteMemory<float*>(reinterpret_cast<float**>(code + 0x46), &heal[1], true);
                        injector::WriteMemory<float*>(reinterpret_cast<float**>(code + 0x28), &heal[2], true);
                        injector::WriteMemory<float*>(reinterpret_cast<float**>(code + 0x1A), &heal[3], true);
                        injector::WriteMemory<float*>(reinterpret_cast<float**>(code + 0x04), &heal[4], true);
                        return true;
                    }();
                    (void)redirected;
                    const float HealMenuBoxWidth = originalHealValues[0]; // 26.087f
                    const float HealMenuCursorXMin = originalHealValues[1]; // 0.35f
                    const float HealMenuCursorYMax = originalHealValues[2]; // 0.87f
                    const float HealMenuCursorYMin = originalHealValues[3]; // 0.635f
                    const float HealMenuCursorXMax = originalHealValues[4]; // 0.58f
                    const float TinyBoxWidth = (HealMenuCursorXMax - HealMenuCursorXMin) / 24.0f; // 24 tiny boxes wide
                    heal[0] = HealMenuBoxWidth / Screen.fHudScale;
                    heal[1] = ((HealMenuCursorXMin + TinyBoxWidth - 0.5f) * Screen.fHudScale) + 0.5f;
                    heal[2] = HealMenuCursorYMax - TinyBoxWidth;
                    heal[3] = HealMenuCursorYMin - TinyBoxWidth;
                    heal[4] = ((HealMenuCursorXMax + TinyBoxWidth - 0.5f) * Screen.fHudScale) + 0.5f;
                }
            };
            injector::MakeInline<ResHook>(pattern.get_first<uintptr_t>(0), pattern.get_first<uintptr_t>(9));

            // The quad's arguments, kept at its entry: the routine reuses their stack slots before the hook below
            static struct
            {
                uint8_t* renderer;
                float rect[4];
                uint32_t texture, colour;
            } Quad{};
            // Quad corners are stored as 16-bit numbers of tenths of a pixel, too few past 3276 pixels: the quads use fewer
            // steps per pixel on wider screens (and the hook below scales by the same)
            static float& quadUnits = Screen.fQuadUnits;
            quadUnits = 10.0f;
            auto quadEntry = hook::pattern("83 EC 1C 53 8B 5C 24 28 85 DB 55 75");
            static SafetyHookMid QuadEntryHook;
            QuadEntryHook = safetyhook::create_mid(quadEntry.get_first(), [](SafetyHookContext& regs)
            {
                auto args = reinterpret_cast<uint32_t*>(regs.esp);
                Quad.renderer = reinterpret_cast<uint8_t*>(args[1]);
                auto rect = reinterpret_cast<const float*>(args[2]);
                for (int i = 0; i < 4; ++i)
                    Quad.rect[i] = rect ? rect[i] : 0.0f;
                Quad.texture = args[4];
                Quad.colour = args[5];
                if (Quad.renderer)
                    CoCFrame::Attach(*reinterpret_cast<IDirect3DDevice9**>(Quad.renderer + 104));
            });
            // The three reads of the step size: movss at +8Ah, fld at +228h and +24Ch
            auto code = quadEntry.get_first<uint8_t>();
            injector::WriteMemory<float*>(reinterpret_cast<float**>(code + 0x8A + 4), &quadUnits, true);
            injector::WriteMemory<float*>(reinterpret_cast<float**>(code + 0x228 + 2), &quadUnits, true);
            injector::WriteMemory<float*>(reinterpret_cast<float**>(code + 0x24C + 2), &quadUnits, true);

            pattern = hook::pattern("D9 05 ? ? ? ? D8 F1 D9 5C 24 38 DD D8 DD D8"); //0x4E1486
            struct HudHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    float x = (float)(*(int32_t*)(regs.esp + 0x24));
                    float y = (float)(*(int32_t*)(regs.esp + 0x28));
                    const float units = quadUnits;
                    auto renderer = QuadEntryHook ? Quad.renderer : nullptr;
                    const float* rect = QuadEntryHook ? Quad.rect : nullptr;
                    const auto texture = Quad.texture;
                    const auto colour = Quad.colour;

                    // The 2D layout is 4:3, centred. Plain and see-through layers across the whole width (fades, dimming
                    // behind menus, subtitle and tip panels) cover the whole screen; full screen artwork keeps its shape
                    // with black at its sides.
                    const bool fullWidth = rect && rect[0] <= 0.0f && rect[2] >= x;
                    if (fullWidth && (!texture || (colour >> 24) < 0xF0))
                    {
                        *(float*)(regs.esp + 0x2C) = 1.0f / (units * x);
                        *(float*)(regs.esp + 0x34) = -0.5f / x;
                    }
                    else
                    {
                        *(float*)(regs.esp + 0x2C) = Screen.fHudScale / (units * x);
                        *(float*)(regs.esp + 0x34) = Screen.fHudOffset / x;
                        if (fullWidth && rect[1] <= 0.0f && rect[3] >= y && Screen.fHudOffset >= 1.0f && renderer)
                        {
                            auto device = *reinterpret_cast<IDirect3DDevice9**>(renderer + 104);
                            const LONG side = LONG(Screen.fHudOffset);
                            const D3DRECT sides[] = { { 0, 0, side, LONG(y) }, { LONG(x) - side, 0, LONG(x), LONG(y) } };
                            if (device)
                                device->Clear(2, sides, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
                        }
                    }

                    float fneg05 = -0.5f;
                    _asm fld ds : fneg05
                }
            };
            injector::MakeInline<HudHook>(pattern.get_first<uintptr_t>(0), pattern.get_first<uintptr_t>(6));

            pattern = hook::pattern("89 4C 24 08 8B 48 08 89 54 24 0C 8B 50 0C 8D 44 24 04"); //0x4239B1
            struct ClickableAreaHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(float*)(regs.esp + 8) = (*(float*)&regs.ecx * Screen.fHudScale) + Screen.fHudOffset;
                    *(float*)&regs.ecx = (*(float*)(regs.eax + 0x8) * Screen.fHudScale);
                }
            };
            injector::MakeInline<ClickableAreaHook>(pattern.get_first<uintptr_t>(0), pattern.get_first<uintptr_t>(7));

            pattern = hook::pattern("D8 25 ? ? ? ? D8 F2 D9 5C 24 2C D9"); //0x4E0BC4
            injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fTextOffset, true);

            pattern = hook::pattern("D8 F2 D9 5C 24 2C D9 44 24 44 D8 25"); //0x4E0BCA
            struct TextHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    float temp = 0.0f;
                    _asm {fdiv    st, st(2)}
                    _asm {fstp    dword ptr[temp]}
                    *(float*)(regs.esp + 0x2C) = temp;

                    *(float*)(regs.esp + 0x24) *= Screen.fHudScale;
                    *(float*)(regs.esp + 0x2C) *= Screen.fHudScale;
                }
            };
            injector::MakeInline<TextHook>(pattern.get_first<uintptr_t>(0), pattern.get_first<uintptr_t>(6));

            CoCMovie::Install();
            CoCCamera::Install();

            CoCCinema::Init(&FixSettings().Int("MAIN.CutsceneBorders"));
            CoCFrame::Init();
            CthulhuMenu::Init();
        };
    }
} Hud;

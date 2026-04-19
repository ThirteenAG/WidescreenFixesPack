module;

#include <stdafx.h>
#include "usercall.hpp"
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

export module Controller;

import ComVars;

SafetyHookInline shsub_50B460 = {};
void __cdecl sub_50B460(int a1, int a2)
{
    if (!bHideCursorForMouseLook)
        return shsub_50B460.unsafe_ccall(a1, a2);
}

bool bLastInputWasPad = false;

class Controller
{
public:
    Controller()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            static int nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
            static float fLeftStickDeadzone = std::clamp(iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f), 0.0f, 100.0f);
            static float fRightStickDeadzone = std::clamp(iniReader.ReadFloat("MISC", "RightStickDeadzone", 10.0f), 0.0f, 100.0f);

            if (nImproveGamepadSupport)
            {
                struct PadState
                {
                    int32_t LSAxis1;
                    int32_t LSAxis2;
                    int32_t LTRT;
                    int32_t RSAxis1;
                    int32_t RSAxis2;
                    uint8_t unk[28];
                    int8_t A;
                    int8_t B;
                    int8_t X;
                    int8_t Y;
                    int8_t LB;
                    int8_t RB;
                    int8_t Select;
                    int8_t Start;
                    int8_t LSClick;
                    int8_t RSClick;
                };

                static bool Zstate, Pstate, Tstate, Dstate, Qstate, Cstate, LBRACKETstate, RBRACKETstate;
                static int32_t* nGameState = (int32_t*)*hook::pattern("83 3D ? ? ? ? 06 ? ? A1").count(1).get(0).get<uint32_t*>(2);
                auto pattern = hook::pattern("B9 ? ? ? ? BE ? ? ? ? F3 A5 5F 5E 5B C3");
                struct CatchPad
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.ecx = 0x88;
                        PadState* PadKeyPresses = (PadState*)(regs.edi);

                        if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1 && *nGameState == 3)
                        {
                            if (PadKeyPresses->LSClick && PadKeyPresses->RSClick)
                            {
                                if (!Qstate)
                                {
                                    keybd_event(BYTE(VkKeyScan('Q')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                                    keybd_event(BYTE(VkKeyScan('Q')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                }
                                Qstate = true;
                            }
                            else
                                Qstate = false;

                            /*if (PadKeyPresses->Y)
                            {
                                if (!Zstate)
                                {
                                    keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                                    keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                }
                                Zstate = true;
                            }
                            else
                                Zstate = false;
                                */
                        }
                    }
                }; injector::MakeInline<CatchPad>(pattern.get_first(0));

                if (nImproveGamepadSupport < 4)
                {
                    const char* MenuTexts[] = { "Quit Game", "Continue", "Back", "Reset Keys To Default", "Activate GPS", "Deactivate GPS", "Install Package", "Install Part", "Install Paint", "Install Decal", "Reset To Default", "Delete Tuned Car", "Logoff", "Cancel Changes", "Customize", "Host LAN Server", "Read Message", "Delete", "Test N2O Purge", "Accept", "Reset to default", "Select", "Open/Close Doors", "Open/Close Hood" }; // "Tip",
                    const char* MenuTextsPC[] = { "[Q] Quit Game", "[Enter] Continue", "[Esc] Back", "[P] Reset Keys To Default", "[C] Activate GPS", "[C] Deactivate GPS", "[Enter] Install Package", "[Enter] Install Part", "[Enter] Install Paint", "[Enter] Install Decal", "[C] Reset To Default", "[C] Delete Tuned Car", "[Esc] Logoff", "[C] Cancel Changes", "[C] Customize", "[C] Host LAN Server", "[Enter] Read Message", "[C] Delete", "[C] Test N2O Purge", "[Enter] Accept", "[C] Reset to default", "[Enter] Select", "[C] Open/Close Doors", "[C] Open/Close Hood" };
                    const char* MenuTextsXBOX[] = { "(LS+RS) Quit Game", "(A) Continue", "(B) Back", "(Y) Reset Keys To Default", "(X) Activate GPS", "(X) Deactivate GPS", "(A) Install Package", "(A) Install Part", "(A) Install Paint", "(A) Install Decal", "(X) Reset To Default", "(X) Delete Tuned Car", "(B) Logoff", "(X) Cancel Changes", "(X) Customize", "(X) Host LAN Server", "(A) Read Message", "(X) Delete", "(X) Test N2O Purge", "(A) Accept", "(X) Reset to default", "(A) Select", "(X) Open/Close Doors", "(X) Open/Close Hood" };
                    const char* MenuTextsPS[] = { "(L3+R3) Quit Game", "(Cross) Continue", "(Circle) Back", "(Triangle) Reset Keys To Default", "(Square) Activate GPS", "(Square) Deactivate GPS", "(Cross) Install Package", "(Cross) Install Part", "(Cross) Install Paint", "(Cross) Install Decal", "(Square) Reset To Default", "(Square) Delete Tuned Car", "(Circle) Logoff", "(Square) Cancel Changes", "(Square) Customize", "(Square) Host LAN Server", "(Cross) Read Message", "(Square) Delete", "(Square) Test N2O Purge", "(Cross) Accept", "(Square) Reset to default", "(Cross) Select", "(Square) Open/Close Doors", "(Square) Open/Close Hood" };

                    static std::vector<std::string> vMenuStrings(MenuTexts, std::end(MenuTexts));
                    static std::vector<std::string> vMenuStringsPC(MenuTextsPC, std::end(MenuTextsPC));
                    static std::vector<std::string> vMenuStringsXBOX(MenuTextsXBOX, std::end(MenuTextsXBOX));
                    static std::vector<std::string> vMenuStringsPS(MenuTextsPS, std::end(MenuTextsPS));

                    pattern = hook::pattern("8B 0D ? ? ? ? 85 C9 50 74 12 8B 44"); //0x5124DD (v1.1)
                    static auto dword_8383DC = *pattern.count(1).get(0).get<uint32_t>(2);
                    struct MenuText
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            regs.ecx = dword_8383DC;

                            auto pszStr = (char*)regs.eax;
                            auto it = std::find(vMenuStrings.begin(), vMenuStrings.end(), pszStr);
                            auto i = std::distance(vMenuStrings.begin(), it);

                            if (it != vMenuStrings.end())
                            {
                                if (bLastInputWasPad)
                                {
                                    if (nImproveGamepadSupport != 2)
                                        regs.eax = (uint32_t)vMenuStringsXBOX[i].c_str();
                                    else
                                        regs.eax = (uint32_t)vMenuStringsPS[i].c_str();
                                }
                                else
                                {
                                    if (nImproveGamepadSupport == 3)
                                        regs.eax = (uint32_t)vMenuStringsPC[i].c_str();
                                }
                            }
                        }
                    }; injector::MakeInline<MenuText>(pattern.get_first(0), pattern.get_first(6));
                }

                // Start menu text
                //uint32_t* dword_4A91E7 = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 50 55 E8").count(3).get(1).get<uint32_t>(1);
                //if (nImproveGamepadSupport != 2)
                //    injector::WriteMemory(dword_4A91E7, 0xD18D4C4C, true); //"Please press START to begin" (Xbox)
                //else
                //    injector::WriteMemory(dword_4A91E7, 0xDC64C04C, true); //"Press START Button" (PlayStation)

                // FrontEnd button remap (through game code, not key emulation)
                pattern = hook::pattern("89 4E ? 89 4E ? 8B E8"); // 005C1A07
                struct FrontEndRemap
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(uintptr_t*)(regs.esi + 0x08) = regs.ecx;
                        *(uintptr_t*)(regs.esi + 0x0C) = regs.ecx;

                        int Y_Button = (regs.eax + 0x5064);
                        *(uintptr_t*)Y_Button = 0x2E; // FE Action "P"
                        int Start = (regs.eax + 0x5074);
                        *(uintptr_t*)Start = 0x24; // FE Action "Comma"
                        int LB = (regs.eax + 0x5068);
                        *(uintptr_t*)LB = 0x26; // FE Action "Left Bracket"
                        int RB = (regs.eax + 0x506C);
                        *(uintptr_t*)RB = 0x27; // FE Action "Right Bracket"
                    }
                };
                injector::MakeInline<FrontEndRemap>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
            }

            const wchar_t* ControlsTexts[] = { L" 2", L" 3", L" 4", L" 5", L" 6", L" 7", L" 8", L" 9", L" 10", L" 1", L" Up", L" Down", L" Left", L" Right", L"X Rotation", L"Y Rotation", L"X Axis", L"Y Axis", L"Z Axis", L"Hat Switch" };
            const wchar_t* ControlsTextsXBOX[] = { L"B", L"X", L"Y", L"LB", L"RB", L"View (Select)", L"Menu (Start)", L"Left stick", L"Right stick", L"A", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"LT / RT", L"D-pad" };
            const wchar_t* ControlsTextsPS[] = { L"Circle", L"Square", L"Triangle", L"L1", L"R1", L"Select", L"Start", L"L3", L"R3", L"Cross", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"L2 / R2", L"D-pad" };

            static std::vector<std::wstring> Texts(ControlsTexts, std::end(ControlsTexts));
            static std::vector<std::wstring> TextsXBOX(ControlsTextsXBOX, std::end(ControlsTextsXBOX));
            static std::vector<std::wstring> TextsPS(ControlsTextsPS, std::end(ControlsTextsPS));

            auto pattern = hook::pattern("66 83 7C 24 08 00 5F 74 13 8D 4C 24 04 51"); //0x4ACF01
            injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0x5F, true); //pop     edi
            static auto dword_4ACF1D = pattern.count(1).get(0).get<uint32_t>(28);
            struct Buttons
            {
                void operator()(injector::reg_pack& regs)
                {
                    auto pszStr = (wchar_t*)(regs.esp + 0x4);
                    if (wcslen(pszStr))
                    {
                        if (nImproveGamepadSupport)
                        {
                            auto it = std::find_if(Texts.begin(), Texts.end(), [&](const std::wstring& str) { std::wstring s(pszStr); return s.find(str) != std::wstring::npos; });
                            auto i = std::distance(Texts.begin(), it);

                            if (it != Texts.end())
                            {
                                const std::wstring& text = (nImproveGamepadSupport != 2) ? TextsXBOX[i] : TextsPS[i];
                                wcscpy_s(pszStr, text.length() + 1, text.c_str());
                            }
                        }
                    }
                    else
                        *(uintptr_t*)(regs.esp - 4) = (uintptr_t)dword_4ACF1D;
                }
            }; injector::MakeInline<Buttons>(pattern.get_first(1), pattern.get_first(9));

            {
                static auto dword_86FFC0 = *hook::get_pattern<int32_t*>("8D 95 ? ? ? ? 8D 3C 28 8B", 2);

                auto pattern = hook::pattern("8B F0 85 F6 7D ? 53 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? 81 FE");
                static auto JoyStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    DIJOYSTATE* state = (DIJOYSTATE*)(&dword_86FFC0[regs.ebp]);

                    // [ -10000 | 10000 ]

                    int32_t nLeftStickDeadzone = static_cast<int32_t>(fLeftStickDeadzone * 100.0f);
                    int32_t nRightStickDeadzone = static_cast<int32_t>(fRightStickDeadzone * 100.0f);

                    state->lX = (std::abs(state->lX) <= nLeftStickDeadzone) ? 0 : state->lX;
                    state->lY = (std::abs(state->lY) <= nLeftStickDeadzone) ? 0 : state->lY;

                    state->lRx = (std::abs(state->lRx) <= nRightStickDeadzone) ? 0 : state->lRx;
                    state->lRy = (std::abs(state->lRy) <= nRightStickDeadzone) ? 0 : state->lRy;

                    g_RightStick.RawX = state->lRx;
                    g_RightStick.RawY = state->lRy;

                    double rx = (double)state->lRx / 10000.0;
                    g_RightStick.X = (float)rx;

                    double ry = (double)state->lRy / 10000.0;
                    g_RightStick.Y = (float)ry;

                    bool hasInput = false;

                    // Any button pressed?
                    for (int i = 0; i < 32; ++i)
                    {
                        if (state->rgbButtons[i] & 0x80)   // button down
                        {
                            hasInput = true;
                            break;
                        }
                    }

                    // Any axis moved outside deadzone?
                    if (!hasInput)
                    {
                        if (std::abs(state->lX) > nLeftStickDeadzone ||
                            std::abs(state->lY) > nLeftStickDeadzone ||
                            std::abs(state->lRx) > nRightStickDeadzone ||
                            std::abs(state->lRy) > nRightStickDeadzone ||
                            state->lZ != 0 || state->lRz != 0)   // triggers / other axes
                        {
                            hasInput = true;
                        }
                    }

                    if (hasInput)
                        bLastInputWasPad = true;
                });

                pattern = hook::pattern("E8 ? ? ? ? 8B 54 24 ? 8A 4E");
                static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (hWnd != GetForegroundWindow())
                        return;

                    MouseData* state = (MouseData*)(regs.esp + 0x8);
                    g_Mouse.DeltaX = state->DeltaX;
                    g_Mouse.DeltaY = state->DeltaY;
                    g_Mouse.Wheel = state->Wheel;

                    if (state->DeltaX != 0 || state->DeltaY != 0 || state->Wheel != 0)
                        bLastInputWasPad = false;
                });

                pattern = hook::pattern("85 C0 7D ? 3D ? ? ? ? 74 ? 3D ? ? ? ? 0F 85 ? ? ? ? A1 ? ? ? ? ? ? 50 FF 52 ? 5F 5E 5D 5B 8B E5 5D C3 A0");
                static auto KeyboardStateHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
                {
                    int keyMaskLow = *(int*)(regs.esi + 8);
                    int keyMaskHigh = *(int*)(regs.esi + 12);

                    if (keyMaskLow != -1 || keyMaskHigh != -1)
                    {
                        bLastInputWasPad = false;
                    }
                });

                pattern = hook::pattern("55 8B EC 83 E4 ? 81 EC ? ? ? ? A1 ? ? ? ? 53 8B 5D ? 56 57");
                shsub_50B460 = safetyhook::create_inline(pattern.get_first(), sub_50B460);
            }
        };
    }
} Controller;
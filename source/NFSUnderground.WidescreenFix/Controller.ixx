module;

#include <stdafx.h>
#include "usercall.hpp"
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

export module Controller;

import ComVars;

using namespace usercall;

SafetyHookInline shsub_4F3030 = {};
void __cdecl sub_4F3030(int a1, int a2)
{
    if (!bHideCursorForMouseLook)
        return shsub_4F3030.unsafe_ccall(a1, a2);
}

bool bLastInputWasPad = false;

struct ButtonMapping
{
    const char* hotkeyName;
    const char* pcTexture;
    const char* xboxTexture;
    const char* psTexture;
};

std::vector<ButtonMapping> AllButtonMappings =
{
    {"DELETEPROFILE_HOTKEY", "PC_DELETE",      "BUTTON_RS",   "BUTTON_R3"       },
    {"DELETEPERSONA_HOTKEY", "PC_DELETE",      "BUTTON_RS",   "BUTTON_R3"       },
    {"DEFAULT_HOTKEY",       "PC_CREATEROOM",  "BUTTON_Y",    "BUTTON_TRIANGLE" },
    {"STOCK_HOTKEY",         "PC_CREATEROOM",  "BUTTON_Y",    "BUTTON_TRIANGLE" },
    {"PERFORMANCE_HOTKEY",   "PC_PERFORMANCE", "BUTTON_X",    "BUTTON_SQUARE"   },
    {"CHANGECOLOR_HOTKEY",   "PC_X",           "BUTTON_A",    "BUTTON_CROSS"    },
    {"DECALCOLOR_HOTKEY",    "PC_CREATEROOM",  "BUTTON_Y",    "BUTTON_TRIANGLE" },
    {"QUIT_HOTKEY",          "PC_QUIT",        "BUTTON_QUIT", "BUTTON_QUIT"     },
    {"CUSTOMIZE_HOTKEY",     "PC_CUSTOM",      "BUTTON_VIEW", "BUTTON_SHARE"    },
    {"NEXT_HOTKEY",          "PC_X",           "BUTTON_A",    "BUTTON_CROSS"    },
    {"BACK_HOTKEY",          "PC_BACK",        "BUTTON_B",    "BUTTON_CIRCLE"   },
    {"PC_TUTORIAL_01",       "PC_TUTORIAL",    "BUTTON_LS",   "BUTTON_L3"       },
    {"PC_TUTORIAL_02",       "PC_TUTORIAL",    "BUTTON_LS",   "BUTTON_L3"       },
};

void LoadResourceFile(const char* filename, int32_t type, int32_t flags = 0, void* callback = nullptr, void* callback_param = nullptr, int32_t file_offset = 0, int32_t file_size = 0)
{
    auto r = CreateResourceFile(filename, type, flags, file_offset, file_size);
    usercall::Call<usercall::reg::eax, usercall::reg::ecx, usercall::reg::edx>(ResourceFileBeginLoading, callback, callback_param, r);
}

void SetTextureHash(FEImage* image, unsigned int texture_hash)
{
    if (image)
    {
        if (image->Handle != texture_hash)
        {
            auto Flags = image->Flags;
            image->Handle = texture_hash;
            image->Flags = Flags | 0x2400000;
        }
    }
}

void* __stdcall FindObject(const char* pkg, int hash)
{
    void* pkgname = (void*)usercall::CallAndRet<uintptr_t, reg::eax>(FEPkgMgr_FindPackage, pkg);
    if (!pkgname)
        return nullptr;
    else
        return (void*)usercall::CallAndRet<uintptr_t, reg::edx, reg::ecx>(FEngFindObject, pkgname, hash);
}

FEImage* FEngFindImage(const char* pkg_name, unsigned int name_hash)
{
    auto result = (FEImage*)FindObject(pkg_name, name_hash);
    if (!result || result->Type != FE_Image)
        return nullptr;
    return result;
}

class Controller
{
public:
    Controller()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            static int nImproveGamepadSupport = std::clamp(iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0), 0, 2);
            static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
            static float fRightStickDeadzone = std::clamp(iniReader.ReadFloat("MISC", "RightStickDeadzone", 10.0f), 0.0f, 100.0f);

            if (nImproveGamepadSupport)
            {
                onMenuScreenBaseNotify() += [](MenuScreen* menu)
                {
                    static bool bOnce = false;
                    if (!bOnce)
                    {
                        auto TPKPath = GetThisModulePath<std::string>().substr(GetExeModulePath<std::string>().length()) + GetThisModuleName().replace_extension("tpk").string().c_str();
                        LoadResourceFile(TPKPath.c_str(), 0);
                        ServiceResourceLoading();
                        bOnce = true;
                    }

                    for (const auto& mapping : AllButtonMappings)
                    {
                        auto FEImage = FEngFindImage(menu->PackageFilename, bStringHash(mapping.hotkeyName));
                        if (FEImage)
                        {
                            auto mode = 0;
                            if (bLastInputWasPad)
                                mode = nImproveGamepadSupport;

                            const char* textureName = nullptr;
                            switch (mode)
                            {
                                case 0:  textureName = mapping.pcTexture;   break;
                                case 1:  textureName = mapping.xboxTexture; break;
                                case 2:  textureName = mapping.psTexture;   break;
                                default: textureName = mapping.pcTexture;   break;
                            }

                            if (textureName)
                                SetTextureHash(FEImage, bStringHash(textureName));
                        }
                    }
                };

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

                static bool Zstate, Pstate, Tstate, Dstate, Qstate, Cstate;
                auto pattern = hook::pattern("7C ? 5F 5D 5E 33 C0 5B C2 08 00"); //0x41989E
                injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(8 + 5), 0x900008C2, true);
                //static uintptr_t ButtonsState = (uintptr_t)*hook::pattern("").count(1).get(0).get<uint32_t*>(8); //0x
                struct CatchPad
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        PadState* PadKeyPresses = *(PadState**)(regs.esp + 0x4);
                        //Keyboard
                        //006F9358 backspace
                        //006F931C enter
                        if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1 && nGameState == 3)
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

                            if (PadKeyPresses->Y)
                            {
                                if (!Zstate)
                                {
                                    keybd_event(BYTE(VkKeyScan('Z')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                                    keybd_event(BYTE(VkKeyScan('Z')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                }
                                Zstate = true;
                            }
                            else
                                Zstate = false;

                            if (PadKeyPresses->X)
                            {
                                if (!Pstate)
                                {
                                    keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                                    keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                }
                                Pstate = true;
                            }
                            else
                                Pstate = false;

                            if (PadKeyPresses->LSClick && !PadKeyPresses->RSClick)
                            {
                                if (!Tstate)
                                {
                                    keybd_event(BYTE(VkKeyScan('T')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                                    keybd_event(BYTE(VkKeyScan('T')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                }
                                Tstate = true;
                            }
                            else
                                Tstate = false;

                            if (PadKeyPresses->RSClick && !PadKeyPresses->LSClick)
                            {
                                if (!Dstate)
                                {
                                    keybd_event(BYTE(VkKeyScan('D')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                                    keybd_event(BYTE(VkKeyScan('D')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                }
                                Dstate = true;
                            }
                            else
                                Dstate = false;

                            if (PadKeyPresses->Select)
                            {
                                if (!Cstate)
                                {
                                    keybd_event(BYTE(VkKeyScan('C')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                                    keybd_event(BYTE(VkKeyScan('C')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                }
                                Cstate = true;
                            }
                            else
                                Cstate = false;
                        }
                    }
                }; injector::MakeInline<CatchPad>(pattern.get_first(8));

                const wchar_t* ControlsTexts[] = { L" 0", L" 1", L" 2", L" 3", L" 4", L" 5", L" 6", L" 7", L" 8", L" 9", L" Up", L" Down", L" Left", L" Right", L"X Rotation", L"Y Rotation", L"X Axis", L"Y Axis", L"Z Axis", L"Hat Switch" };
                const wchar_t* ControlsTextsXBOX[] = { L"A", L"B", L"X", L"Y", L"LB", L"RB", L"View (Select)", L"Menu (Start)", L"Left stick", L"Right stick", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"LT / RT", L"D-pad" };
                const wchar_t* ControlsTextsPS[] = { L"Cross", L"Circle", L"Square", L"Triangle", L"L1", L"R1", L"Select", L"Start", L"L3", L"R3", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"L2 / R2", L"D-pad" };

                static std::vector<std::wstring> Texts(ControlsTexts, std::end(ControlsTexts));
                static std::vector<std::wstring> TextsXBOX(ControlsTextsXBOX, std::end(ControlsTextsXBOX));
                static std::vector<std::wstring> TextsPS(ControlsTextsPS, std::end(ControlsTextsPS));

                pattern = hook::pattern("8D 43 60 8D 74 24 10 E8 ? ? ? ? 8B"); //0x4148F6
                struct Buttons
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.eax = regs.ebx + 0x60;
                        regs.esi = regs.esp + 0x10;

                        auto pszStr = (wchar_t*)(regs.esp + 0x10);
                        auto it = std::find_if(Texts.begin(), Texts.end(), [&](const std::wstring& str) { std::wstring s(pszStr); return s.find(str) != std::wstring::npos; });
                        auto i = std::distance(Texts.begin(), it);

                        if (it != Texts.end())
                        {
                            const std::wstring& text = (nImproveGamepadSupport != 2) ? TextsXBOX[i] : TextsPS[i];
                            wcscpy_s(pszStr, text.length() + 1, text.c_str());
                        }
                    }
                }; if (pattern.size() > 0) { injector::MakeInline<Buttons>(pattern.get_first(0), pattern.get_first(7)); }

                // FrontEnd button remap (through game code, not key emulation)
                pattern = hook::pattern("8B 86 ? ? ? ? 8B 0C ? ? ? ? ? 85 C9 0F"); // 004071C5
                struct FrontEndRemap
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.eax = *(uintptr_t*)(regs.esi + 0x0130);

                        int LB = (regs.esi + 0x0130) - (0x88);
                        *(uintptr_t*)LB = 0x1A; // FE Action "Comma"
                        int RB = (regs.esi + 0x0130) - (0x84);
                        *(uintptr_t*)RB = 0x19; // FE Action "Period"
                    }
                };
                injector::MakeInline<FrontEndRemap>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
            }

            {
                static auto dword_71D8A8 = *hook::get_pattern<int32_t*>("8D 95 ? ? ? ? 8B F2", 2);

                auto pattern = hook::pattern("8B F0 85 F6 7D ? 8B 7C 24");
                static auto JoyStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    DIJOYSTATE* state = (DIJOYSTATE*)(&dword_71D8A8[regs.ebp]);

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

                pattern = hook::pattern("8B 44 24 ? 8B 4C 24 ? 8B 53");
                static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (hWnd != GetForegroundWindow())
                        return;

                    MouseData* state = (MouseData*)(regs.esp + 0x4);
                    g_Mouse.DeltaX = state->DeltaX;
                    g_Mouse.DeltaY = state->DeltaY;
                    g_Mouse.Wheel = state->Wheel;

                    if (state->DeltaX != 0 || state->DeltaY != 0 || state->Wheel != 0)
                        bLastInputWasPad = false;
                });

                pattern = hook::pattern("85 C0 7D ? 3D ? ? ? ? 74 ? 3D ? ? ? ? 0F 85 ? ? ? ? A1 ? ? ? ? ? ? 50 FF 52 ? 5F 5B");
                static auto KeyboardStateHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
                {
                    int keyMaskLow = *(int*)(regs.esi + 8);
                    int keyMaskHigh = *(int*)(regs.esi + 12);

                    if (keyMaskLow != -1 || keyMaskHigh != -1)
                    {
                        bLastInputWasPad = false;
                    }
                });

                pattern = hook::pattern("55 8B EC 83 E4 ? 81 EC ? ? ? ? A1 ? ? ? ? 53 8B 5D");
                shsub_4F3030 = safetyhook::create_inline(pattern.get_first(), sub_4F3030);
            }
        };
    }
} Controller;
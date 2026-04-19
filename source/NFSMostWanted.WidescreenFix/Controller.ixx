module;

#include <stdafx.h>
#include "usercall.hpp"
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

export module Controller;

import ComVars;

SafetyHookInline shsub_56FDB0 = {};
void __cdecl sub_56FDB0(int a1, int a2)
{
    if (!bHideCursorForMouseLook)
        return shsub_56FDB0.unsafe_ccall(a1, a2);
}

bool bLastInputWasPad = false;

struct ButtonMapping
{
    const char* pcTexture;
    const char* xboxTexture;
    const char* psTexture;
};

struct ButtonHash
{
    unsigned int pcTexture;
    unsigned int xboxTexture;
    unsigned int psTexture;
};

std::vector<ButtonMapping> AllButtonMappings =
{
    { "L1",                      "XBOX_L1",                   "PS_L1"                   },
    { "PC_KEY_Q",                "XBOX_KEY_Q",                "PS_KEY_Q"                },
    { "PC_GREATERTHAN_LESSTHAN", "XBOX_GREATERTHAN_LESSTHAN", "PS_GREATERTHAN_LESSTHAN" },
    { "R1",                      "XBOX_R1",                   "PS_R1"                   },
    { "PC_KEY_ENTER",            "XBOX_KEY_ENTER",            "PS_KEY_ENTER"            },
    { "PC_KEY_ESC_FRENCH",       "XBOX_KEY_ESC_FRENCH",       "PS_KEY_ESC_FRENCH"       },
    { "PC_KEY_ESC",              "XBOX_KEY_ESC",              "PS_KEY_ESC"              },
    { "PC_KEY_1",                "XBOX_KEY_1",                "PS_KEY_1"                },
    { "PC_KEY_2",                "XBOX_KEY_2",                "PS_KEY_2"                },
    { "PC_KEY_3",                "XBOX_KEY_3",                "PS_KEY_3"                },
    { "PC_KEY_4",                "XBOX_KEY_4",                "PS_KEY_4"                },
    { "PC_ARROW_KEYS_UP_DOWN",   "XBOX_ARROW_KEYS_UP_DOWN",   "PS_ARROW_KEYS_UP_DOWN"   },
    { "PC_LOAD",                 "XBOX_LOAD",                 "PS_LOAD"                 },
    { "PC_ARROW_KEYS",           "XBOX_ARROW_KEYS",           "PS_ARROW_KEYS"           },
};

std::vector<ButtonHash> AllButtonHashes;

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

int nImproveGamepadSupport = 0;
char __stdcall cb(FEObject* pObj)
{
    static bool bOnce = false;
    if (!bOnce)
    {
        auto TPKPath = GetThisModulePath<std::string>().substr(GetExeModulePath<std::string>().length()) + GetThisModuleName().replace_extension("tpk").string().c_str();
        auto r = CreateResourceFile(TPKPath.c_str(), 0, 0, 0, 0);
        ResourceFileBeginLoading(r, 0, nullptr, nullptr);
        ServiceResourceLoading();
        bOnce = true;
    }

    if (pObj->Type == FE_Image)
    {
        for (const auto& mapping : AllButtonHashes)
        {
            if (pObj->Handle == mapping.pcTexture || pObj->Handle == mapping.xboxTexture || pObj->Handle == mapping.psTexture)
            {
                auto mode = 0;
                if (bLastInputWasPad)
                    mode = nImproveGamepadSupport;

                unsigned int textureHash = 0;
                switch (mode)
                {
                    case 0:  textureHash = mapping.pcTexture;   break;
                    case 1:  textureHash = mapping.xboxTexture; break;
                    case 2:  textureHash = mapping.psTexture;   break;
                    default: textureHash = mapping.pcTexture;   break;
                }

                if (textureHash)
                    SetTextureHash((FEImage*)pObj, textureHash);
            }
        }
    }

    return 1;
}

namespace cFEng
{
    SafetyHookInline shService = {};
    void __fastcall Service(void* cFEng, void* edx, unsigned int tDeltaTicks, unsigned int lock)
    {
        struct FEObjectCallbackStruct
        {
            void* Destructor = nullptr;
            void* Function = nullptr;
        };

        FEObjectCallbackStruct callback = { nullptr, &cb };
        void* cbpointer = &callback;

        uintptr_t current = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(cFEng) + 0xE0);

        while (current)
        {
            FEPackage::ForAllObjects(reinterpret_cast<void*>(current), 0, &cbpointer);
            uintptr_t next = *reinterpret_cast<uintptr_t*>(current + 4);
            if (next == current)
                break;
            current = next;
        }

        return shService.unsafe_fastcall(cFEng, edx, tDeltaTicks, lock);
    }
}

class Controller
{
public:
    Controller()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
            static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
            static float fRightStickDeadzone = iniReader.ReadFloat("MISC", "RightStickDeadzone", 10.0f);

            if (nImproveGamepadSupport)
            {
                std::vector<ButtonMapping> result;
                for (const auto& mapping : AllButtonMappings)
                {
                    AllButtonHashes.emplace_back(ButtonHash{
                        .pcTexture = mapping.pcTexture ? bStringHash(mapping.pcTexture) : 0,
                        .xboxTexture = mapping.xboxTexture ? bStringHash(mapping.xboxTexture) : 0,
                        .psTexture = mapping.psTexture ? bStringHash(mapping.psTexture) : 0,
                    });
                }

                auto pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? ? ? 85 C0 74");
                cFEng::shService = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(0)).as_int(), cFEng::Service);

                struct PadState
                {
                    int32_t LSAxis1;
                    int32_t LSAxis2;
                    int32_t LTRT;
                    int32_t A;
                    int32_t B;
                    int32_t X;
                    int32_t Y;
                    int32_t LB;
                    int32_t RB;
                    int32_t Select;
                    int32_t Start;
                    int32_t LSClick;
                    int32_t RSClick;
                };

                pattern = hook::pattern("C7 45 E0 01 00 00 00 89 5D E4"); //0x7F2AE2
                static uintptr_t ButtonsState = (uintptr_t)*hook::pattern("0F B6 54 24 04 33 C0 B9").count(1).get(0).get<uint32_t*>(8); //0x514B90
                static int32_t* nGameState = (int32_t*)*hook::pattern("83 3D ? ? ? ? 06 ? ? A1").count(1).get(0).get<uint32_t*>(2); //0x925E90
                struct CatchPad
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(uintptr_t*)(regs.ebp - 0x20) = 1; //mov     dword ptr [ebp-20h], 1

                        PadState* PadKeyPresses = (PadState*)(regs.esi + 0x234); //dpad is PadKeyPresses+0x220

                        //Keyboard 
                        //008F3584 2
                        //008F3514 3 
                        //008F34F8 4 
                        if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1)
                        {
                            if (PadKeyPresses->Select) //3
                            {
                                *(int32_t*)(ButtonsState + 0xCC) = 1;
                            }
                            else
                            {
                                *(int32_t*)(ButtonsState + 0xCC) = 0;
                            }

                            if (PadKeyPresses->X) //2
                            {
                                *(int32_t*)(ButtonsState + 0x13C) = 1;
                            }
                            else
                            {
                                *(int32_t*)(ButtonsState + 0x13C) = 0;
                            }
                            if (PadKeyPresses->Start) //4
                            {
                                *(int32_t*)(ButtonsState + 0xB0) = 1;
                            }
                            else
                            {
                                *(int32_t*)(ButtonsState + 0xB0) = 0;
                            }
                            if (PadKeyPresses->LSClick && PadKeyPresses->RSClick)
                            {
                                if (*nGameState == 3)
                                {
                                    keybd_event(BYTE(VkKeyScan('Q')), 0, 0, 0);
                                    keybd_event(BYTE(VkKeyScan('Q')), 0, KEYEVENTF_KEYUP, 0);
                                }
                            }
                        }
                    }
                }; injector::MakeInline<CatchPad>(pattern.get_first(0), pattern.get_first(7));

                const char* ControlsTexts[] = { " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9", " 10", " 1", " Up", " Down", " Left", " Right", "X Rotation", "Y Rotation", "X Axis", "Y Axis", "Z Axis", "Hat Switch" };
                const char* ControlsTextsXBOX[] = { "B", "X", "Y", "LB", "RB", "View (Select)", "Menu (Start)", "Left stick", "Right stick", "A", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right", "Right stick Left/Right", "Right stick Up/Down", "Left stick Left/Right", "Left stick Up/Down", "LT / RT", "D-pad" };
                const char* ControlsTextsPS[] = { "Circle", "Square", "Triangle", "L1", "R1", "Select", "Start", "L3", "R3", "Cross", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right", "Right stick Left/Right", "Right stick Up/Down", "Left stick Left/Right", "Left stick Up/Down", "L2 / R2", "D-pad" };

                static std::vector<std::string> Texts(ControlsTexts, std::end(ControlsTexts));
                static std::vector<std::string> TextsXBOX(ControlsTextsXBOX, std::end(ControlsTextsXBOX));
                static std::vector<std::string> TextsPS(ControlsTextsPS, std::end(ControlsTextsPS));

                pattern = hook::pattern("8B 44 24 1C 50 E8 ? ? ? ? 83 C4 10 5F 5E 5B C3"); //0x6282D3
                injector::WriteMemory<uint8_t>(pattern.get_first(16 + 5), 0xC3, true);
                struct Buttons
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        auto pszStr = *(char**)(regs.esp + 4);
                        auto it = std::find_if(Texts.begin(), Texts.end(), [&](const std::string& str) { std::string s(pszStr); return s.find(str) != std::string::npos; });
                        auto i = std::distance(Texts.begin(), it);

                        if (it != Texts.end())
                        {
                            const std::string& text = (nImproveGamepadSupport != 2) ? TextsXBOX[i] : TextsPS[i];
                            strcpy_s(pszStr, text.length() + 1, text.c_str());
                        }
                    }
                }; injector::MakeInline<Buttons>(pattern.get_first(16));

                pattern = hook::pattern("5E D8 0D ? ? ? ? 59 C2 08 00"); //0x628A51
                static auto unk_91F7F4 = *hook::get_pattern<void**>("3D ? ? ? ? 89 7C 24 14 0F 8C ? ? ? ? 5F", 1);
                struct MenuRemap
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        static const auto f0078125 = 0.0078125f;
                        _asm {fmul dword ptr[f0078125]}

                        auto dword_91FABC = &unk_91F7F4[178];
                        auto dword_91FAF0 = &unk_91F7F4[191];
                        auto dword_91FC90 = &unk_91F7F4[295];

                        *(uint32_t*)(dword_91FABC) = 0; // "Enter"; changed B to A
                        *(uint32_t*)(dword_91FAF0) = 1; // "ESC"; changed X to B
                        *(uint32_t*)(dword_91FC90) = 3; // "1"; changed A to Y
                    }
                }; injector::MakeInline<MenuRemap>(pattern.get_first(1), pattern.get_first(7));

                // Start menu text
                uint32_t* dword_5A313D = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 51 E8 ? ? ? ? 83 C4 ? 3B FB").count(1).get(0).get<uint32_t>(1);
                if (nImproveGamepadSupport != 2)
                    injector::WriteMemory(dword_5A313D, 0xD18D4C4C, true); //"Press START to begin" (Xbox)
                else
                    injector::WriteMemory(dword_5A313D, 0xDC64C04C, true); //"Press START button" (PlayStation)
            }

            if (fLeftStickDeadzone)
            {
                // DInput [ 0 32767 | 32768 65535 ] 
                fLeftStickDeadzone /= 200.0f;

                auto pattern = hook::pattern("89 86 34 02 00 00 8D 45 D4"); //0x7F29B2
                struct DeadzoneHookX
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        double dStickState = (double)regs.eax / 65535.0;
                        dStickState -= 0.5;
                        if (std::abs(dStickState) <= fLeftStickDeadzone)
                            dStickState = 0.0;
                        dStickState += 0.5;
                        *(uint32_t*)(regs.esi + 0x234) = (uint32_t)(dStickState * 65535.0);
                    }
                }; injector::MakeInline<DeadzoneHookX>(pattern.get_first(0), pattern.get_first(6));

                struct DeadzoneHookY
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        double dStickState = (double)regs.edx / 65535.0;
                        dStickState -= 0.5;
                        if (std::abs(dStickState) <= fLeftStickDeadzone)
                            dStickState = 0.0;
                        dStickState += 0.5;
                        *(uint32_t*)(regs.esi + 0x238) = (uint32_t)(dStickState * 65535.0);
                    }
                }; injector::MakeInline<DeadzoneHookY>(pattern.get_first(18 + 0), pattern.get_first(18 + 6));
            }

            {
                auto pattern = hook::pattern("8B 85 ? ? ? ? 3B 86");
                static auto JoyStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (hWnd != GetForegroundWindow())
                        return;

                    DIJOYSTATE* state = (DIJOYSTATE*)(regs.ebp - 0x13C);

                    g_RightStick.RawX = state->lRx;
                    g_RightStick.RawY = state->lRy;

                    float dz = (fRightStickDeadzone > 0.0f) ? fRightStickDeadzone / 200.0f : 0.0f;

                    double rx = ((double)state->lRx - 32767.5) / 32767.5;
                    if (std::abs(rx) <= dz) rx = 0.0;
                    g_RightStick.X = (float)rx;

                    double ry = ((double)state->lRy - 32767.5) / 32767.5;
                    if (std::abs(ry) <= dz) ry = 0.0;
                    g_RightStick.Y = (float)ry;

                    bool hasPadInput = false;

                    for (int i = 0; i < 32; i++)
                    {
                        if (state->rgbButtons[i] & 0x80)
                        {
                            hasPadInput = true;
                            break;
                        }
                    }

                    if (!hasPadInput)
                    {
                        if (std::abs(rx) > 0.01 || std::abs(ry) > 0.01)
                            hasPadInput = true;
                    }

                    if (!hasPadInput)
                    {
                        double lx = ((double)state->lX - 32767.5) / 32767.5;
                        double ly = ((double)state->lY - 32767.5) / 32767.5;

                        if (std::abs(lx) > 0.01 || std::abs(ly) > 0.01)
                            hasPadInput = true;
                    }

                    if (hasPadInput)
                        bLastInputWasPad = true;
                });

                pattern = hook::pattern("8B 45 ? 3B C7 75 ? 3B D7");
                static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (hWnd != GetForegroundWindow())
                        return;

                    MouseData* state = (MouseData*)(regs.ebp - 0x14);
                    g_Mouse.DeltaX = state->DeltaX;
                    g_Mouse.DeltaY = state->DeltaY;
                    g_Mouse.Wheel = state->Wheel;

                    if (state->DeltaX != 0 || state->DeltaY != 0 || state->Wheel != 0)
                        bLastInputWasPad = false;
                });

                pattern = hook::pattern("89 45 ? 0F 84 ? ? ? ? 8D 4D ? E8");
                static auto KeyboardStateHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
                {
                    uint8_t* keyboardState = (uint8_t*)(regs.ebp - 0x130);

                    bool anyKeyPressed = false;

                    for (int i = 0; i < 256; ++i)
                    {
                        if (keyboardState[i] & 0x80)
                        {
                            anyKeyPressed = true;
                            break;
                        }
                    }

                    if (anyKeyPressed)
                        bLastInputWasPad = false;
                });

                pattern = hook::pattern("55 8B EC 83 E4 ? 81 EC ? ? ? ? A0 ? ? ? ? 84 C0 53 56 57");
                shsub_56FDB0 = safetyhook::create_inline(pattern.get_first(), sub_56FDB0);
            }
        };
    }
} Controller;
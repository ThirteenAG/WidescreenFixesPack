module;

#include <stdafx.h>
#include "usercall.hpp"
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

export module Controller;

import ComVars;

SafetyHookInline shsub_585290 = {};
void __cdecl sub_585290(int a1, int a2)
{
    if (!bHideCursorForMouseLook)
        return shsub_585290.unsafe_ccall(a1, a2);
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
    { "PC_KEY_ENTER",       "XBOX_KEY_ENTER",       "PS_KEY_ENTER"       },
    { "PC_KEY_ESC",         "XBOX_KEY_ESC",         "PS_KEY_ESC"         },
    { "PC_KEY_ESC_FRENCH",  "XBOX_KEY_ESC_FRENCH",  "PS_KEY_ESC_FRENCH"  },
    { "PC_KEY_Q",           "XBOX_KEY_Q",           "PS_KEY_Q"           },
    { "L1",                 "XBOX_L1",              "PS_L1"              },
    { "PC_KEY_9",           "XBOX_KEY_9",           "PS_KEY_9"           },
    { "PC_KEY_SPC",         "XBOX_KEY_SPC",         "PS_KEY_SPC"         },
    { "PC_KEY_SPC_CZECH",   "XBOX_KEY_SPC_CZECH",   "PS_KEY_SPC_CZECH"   },
    { "PC_KEY_SPC_DANISH",  "XBOX_KEY_SPC_DANISH",  "PS_KEY_SPC_DANISH"  },
    { "PC_KEY_SPC_DUTCH",   "XBOX_KEY_SPC_DUTCH",   "PS_KEY_SPC_DUTCH"   },
    { "PC_KEY_SPC_FRENCH",  "XBOX_KEY_SPC_FRENCH",  "PS_KEY_SPC_FRENCH"  },
    { "PC_KEY_SPC_GERMAN",  "XBOX_KEY_SPC_GERMAN",  "PS_KEY_SPC_GERMAN"  },
    { "PC_KEY_SPC_ITALIAN", "XBOX_KEY_SPC_ITALIAN", "PS_KEY_SPC_ITALIAN" },
    { "PC_KEY_SPC_SPANISH", "XBOX_KEY_SPC_SPANISH", "PS_KEY_SPC_SPANISH" },
    { "PC_X",               "XBOX_X",               "PS_X"               },
    { "PC_KEY_0",           "XBOX_KEY_0",           "PS_KEY_0"           },
    { "R1",                 "XBOX_R1",              "PS_R1"              },
    { "PC_KEY_3",           "XBOX_KEY_3",           "PS_KEY_3"           },
    { "PC_KEY_2",           "XBOX_KEY_2",           "PS_KEY_2"           },
    { "PC_KEY_1",           "XBOX_KEY_1",           "PS_KEY_1"           },
    { "PC_CLICKDRAG_ICON",  "XBOX_CLICKDRAG_ICON",  "PS_CLICKDRAG_ICON"  },
    { "PC_LOAD",            "XBOX_LOAD",            "PS_LOAD"            },
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
            bool bExpandControllerOptions = iniReader.ReadInteger("MISC", "ExpandControllerOptions", 0) != 0;

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

                auto pattern = hook::pattern("E8 ? ? ? ? 57 8B CE E8 ? ? ? ? 8B F0");
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

                pattern = hook::pattern("C7 45 E0 01 00 00 00 89 5D E4"); //0x892BFD
                static uintptr_t ButtonsState = (uintptr_t)*hook::pattern("0F B6 54 24 04 33 C0 B9").get(0).get<uint32_t*>(8); //0xA5E058
                static int32_t* nGameState = (int32_t*)*hook::pattern("83 3D ? ? ? ? 06 ? ? A1").get(0).get<uint32_t*>(2); //0xA99BBC
                struct CatchPad
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(uintptr_t*)(regs.ebp - 0x20) = 1; //mov     dword ptr [ebp-20h], 1

                        PadState* PadKeyPresses = (PadState*)(regs.esi + 0x234); //dpad is PadKeyPresses+0x220

                        //Keyboard 
                        //00A5E15C T
                        //00A5E124 M
                        //00A5E1B0 1
                        //00A5E194 2
                        //00A5E140 3 
                        //00A5E178 4 
                        //00A5E1CC 9 
                        //00A5E1E8 0 
                        //00A5E108 Space
                        //00A5E0D0 ESC
                        //00A5E0B4 Enter
                        if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1)
                        {
                            if (PadKeyPresses->Select)
                            {
                                *(int32_t*)(ButtonsState + 0xE8) = 1;
                            }
                            else
                            {
                                *(int32_t*)(ButtonsState + 0xE8) = 0;
                            }

                            if (PadKeyPresses->X)
                            {
                                *(int32_t*)(ButtonsState + 0x13C) = 1;
                            }
                            else
                            {
                                *(int32_t*)(ButtonsState + 0x13C) = 0;
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

                pattern = hook::pattern("68 04 01 00 00 51 E8 ? ? ? ? 83 C4 10 5F 5E C3"); //0x679B53
                injector::WriteMemory<uint8_t>(pattern.get(0).get<uint32_t>(16 + 5), 0xC3, true);
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

                pattern = hook::pattern("8B 0F 8B 54 0E 08 DB 44 90 0C"); //0x67A466
                static auto unk_A987EC = *hook::get_pattern<void**>("81 FA ? ? ? ? 89 74 24 18 ? ? ? ? ? ? 8B 44 24 14", 2);
                struct MenuRemap
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.ecx = *(uint32_t*)regs.edi;
                        regs.edx = *(uint32_t*)(regs.esi + regs.ecx + 0x08);

                        auto dword_A9882C = &unk_A987EC[16];
                        auto dword_A98834 = &unk_A987EC[18];
                        auto dword_A9883C = &unk_A987EC[20];
                        auto dword_A98874 = &unk_A987EC[34];

                        *(uint32_t*)(*(uint32_t*)dword_A9882C + 0x20) = 0; // "Enter"; changed B to A
                        *(uint32_t*)(*(uint32_t*)dword_A98834 + 0x20) = 1; // "ESC"; changed X to B
                        *(uint32_t*)(*(uint32_t*)dword_A9883C + 0x20) = 7; // "SPC"; changed RS to Start
                        *(uint32_t*)(*(uint32_t*)dword_A98874 + 0x20) = 3; // "1"; changed A to Y
                    }
                }; injector::MakeInline<MenuRemap>(pattern.get_first(0), pattern.get_first(6));

                // Start menu text
                uint32_t* dword_8577AC = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 51 E8 ? ? ? ? 83 C4 14 E8").count(1).get(0).get<uint32_t>(1);
                if (nImproveGamepadSupport != 2)
                    injector::WriteMemory(dword_8577AC, 0x186AAECC, true); //"Press START to begin" (Xbox)
                else
                    injector::WriteMemory(dword_8577AC, 0x703A92CC, true); //"Press START button" (PlayStation)
            }

            if (bExpandControllerOptions)
            {
                uint32_t* dword_692539 = hook::pattern("B8 14 00 00 00 C2 08 00 B8").count(1).get(0).get<uint32_t>(1);
                injector::WriteMemory((uint32_t)dword_692539, 0x1D, true); //lists all 29 options in the controller config menu
            }

            if (fLeftStickDeadzone)
            {
                // DInput [ 0 32767 | 32768 65535 ] 
                fLeftStickDeadzone /= 200.0f;

                auto pattern = hook::pattern("89 86 34 02 00 00 8D 45 D4"); //0x892ACD 0x892ADF
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

                pattern = hook::pattern("8B 45 ? 3B C7 75");
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

                pattern = hook::pattern("8B 83 ? ? ? ? 8B 80");
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

                pattern = hook::pattern("55 8B EC 83 E4 ? 81 EC ? ? ? ? A0 ? ? ? ? 84 C0 53 56 57 C6 44 24");
                shsub_585290 = safetyhook::create_inline(pattern.get_first(), sub_585290);
            }
        };
    }
} Controller;
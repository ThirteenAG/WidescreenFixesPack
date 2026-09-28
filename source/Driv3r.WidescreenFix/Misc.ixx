module;

#include <stdafx.h>
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

export module Misc;

import ComVars;

using namespace Keymap;

constexpr Binding Bind(uint32_t key, uint32_t code = None) { return { key, 0, code, false }; }
constexpr Binding BindAxis(uint32_t key, Axis axis, bool positive) { return { key, 1, axis, positive }; }

// Xbox controller as DirectInput reports it: the triggers share the Z axis (LT positive, RT negative),
// the right stick is RX/RY and the D-pad is the POV hat
constexpr uint32_t PadA = JoyButton(1);
constexpr uint32_t PadB = JoyButton(2);
constexpr uint32_t PadX = JoyButton(3);
constexpr uint32_t PadY = JoyButton(4);
constexpr uint32_t PadLB = JoyButton(5);
constexpr uint32_t PadRB = JoyButton(6);
constexpr uint32_t PadBack = JoyButton(7);
constexpr uint32_t PadStart = JoyButton(8);
constexpr uint32_t PadLS = JoyButton(9);
constexpr uint32_t PadRS = JoyButton(10);

struct DefaultBinding
{
    Action action;
    Binding negative; // the only binding of one-way actions
    Binding positive;
};

// Actions that are not listed keep the game's defaults
constexpr DefaultBinding DefaultBindings[] =
{
    { Pause,                   Bind(DIK_ESCAPE, PadStart) },
    { ActionButton,            Bind(DIK_E, PadLB) },
    { CameraChange,            Bind(DIK_V, PadBack) },
    { PauseZoomIn,             BindAxis(DIK_NUMPAD7, AxisRY, false) },
    { PauseZoomOut,            BindAxis(DIK_NUMPAD1, AxisRY, true) },

    { DirectorSecondSpeed,     Bind(DIK_LSHIFT) },
    { DirectorMoveUpDown,      BindAxis(DIK_PRIOR, AxisZ, false), BindAxis(DIK_NEXT, AxisZ, true) },
    { DirectorRotateLeftRight, BindAxis(DIK_NUMPAD4, AxisRX, false), BindAxis(DIK_NUMPAD6, AxisRX, true) },

    { Accelerate,              BindAxis(DIK_W, AxisZ, false) },
    { Reverse,                 BindAxis(DIK_S, AxisZ, true) },
    { DigitalSteerLeft,        Bind(DIK_A, PovLeft) },
    { DigitalSteerRight,       Bind(DIK_D, PovRight) },
    { DigitalLeanForward,      Bind(DIK_Q, PovUp) },
    { DigitalLeanBack,         Bind(DIK_Z, PovDown) },
    { HandBrake,               Bind(DIK_SPACE, PadA) },
    { BurnOut,                 Bind(DIK_LSHIFT, PadX) },
    { GetOutVehicle,           Bind(DIK_F, PadY) },
    { Horn,                    Bind(DIK_H, PadLS) },
    { ThrillCam,               Bind(DIK_R, PadB) },

    { GetInVehicle,            Bind(DIK_F, PadY) },
    { FootLookLeftRight,       BindAxis(None, AxisRX, false), BindAxis(None, AxisRX, true) },
    { FootLookUpDown,          BindAxis(None, AxisRY, false), BindAxis(None, AxisRY, true) },
    { Shoot,                   BindAxis(MouseButton(1), AxisZ, false) },
    { DrawHolsterGun,          Bind(MouseButton(2), PadRB) },
    { WeaponToggle,            Bind(DIK_Q, PovRight) },
    { Reload,                  Bind(DIK_R, PadX) },
    { Jump,                    Bind(DIK_SPACE, PadA) },
    { CrouchRoll,              Bind(DIK_LCONTROL, PadB) },
};

// The orbit camera takes the right stick while driving, only look back keeps a key and a button
constexpr DefaultBinding OrbitCameraLookBindings[] =
{
    { DriveLookLeftRight,      Bind(None), Bind(None) },
    { DriveLookBackForward,    Bind(DIK_C, PadRS), Bind(None) },
};

constexpr DefaultBinding LookBindings[] =
{
    { DriveLookLeftRight,      BindAxis(None, AxisRX, false), BindAxis(None, AxisRX, true) },
    { DriveLookBackForward,    BindAxis(DIK_C, AxisRY, true), BindAxis(None, AxisRY, false) },
};

bool bOrbitCamera = true;

void ApplyDefaultBindings()
{
    for (auto [table, count] : { std::pair{ Live(), EntryCount }, std::pair{ Defaults(), EntryCount }, std::pair{ Fixed(), FixedEntryCount } })
    {
        auto apply = [&](const auto& bindings)
        {
            for (const auto& binding : bindings)
            {
                if (auto entry = Find(table, count, binding.action))
                {
                    entry->binding[0] = binding.negative;
                    entry->binding[1] = entry->twoWay ? binding.positive : binding.negative;
                }
            }
        };

        apply(DefaultBindings);
        if (bOrbitCamera)
            apply(OrbitCameraLookBindings);
        else
            apply(LookBindings);
    }
}

// The controls menu has no name for mouse buttons in the key column
int (__fastcall* GetSecondaryInputName)(void* _this, void* edx, uint32_t code, wchar_t* buffer, int size) = nullptr;
SafetyHookInline shGetKeyName = {};
int __fastcall GetKeyName(void* _this, void* edx, uint32_t code, wchar_t* buffer, int size)
{
    if ((code & 0xF000) == 0x2000)
        return GetSecondaryInputName(_this, edx, code, buffer, size);
    return shGetKeyName.unsafe_thiscall<int>(_this, code, buffer, size);
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            auto bModernControls = iniReader.ReadInteger("MAIN", "ModernControls", 1) != 0;
            bOrbitCamera = iniReader.ReadInteger("CAMERA", "Enable", 1) != 0;

            //Modern default controls, for new profiles and for Reset in the controls menu
            if (bModernControls)
            {
                //the binding tables are filled by static initializers that run before WinMain, override them once each is done
                static std::vector<SafetyHookMid> KeymapInitHooks;
                auto onKeymapInit = [](SafetyHookContext& regs)
                {
                    ApplyDefaultBindings();
                };

                hook::pattern("89 0D ? ? ? ? 5B 83 C4 50 C3").count(2).for_each_result([&](hook::pattern_match match) //live and reset tables
                {
                    KeymapInitHooks.push_back(safetyhook::create_mid(match.get<void>(6), onKeymapInit));
                });

                auto pattern = hook::pattern("89 15 ? ? ? ? 5D 83 C4 50 C3"); //fixed table
                KeymapInitHooks.push_back(safetyhook::create_mid(pattern.get_first(6), onKeymapInit));

                pattern = hook::pattern("83 EC 18 53 56 8B 35 ? ? ? ? 57");
                GetSecondaryInputName = (decltype(GetSecondaryInputName))pattern.get_first();

                pattern = hook::pattern("8B 44 24 04 8B 15 ? ? ? ? 8B C8");
                shGetKeyName = safetyhook::create_inline(pattern.get_first(), GetKeyName);
            }
        };

        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
            static float fDrawDistanceFactor = iniReader.ReadFloat("MAIN", "DrawDistanceFactor", 1.0f);

            if (bSkipIntro)
            {
                auto pattern = hook::pattern("77 7B FF 24 85 ? ? ? ? 8B 06");
                injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); //jmp
            }

            //Draw distance adjuster
            if (fDrawDistanceFactor)
            {
                auto pattern = hook::pattern("89 46 7C 89 4E 08 C3"); //0x4DDD68
                struct DrawDistHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(uint32_t*)(regs.esi + 0x7C) = regs.eax;
                        *(uint32_t*)(regs.esi + 0x08) = regs.ecx;

                        //*(float*)(regs.esi + 0x88) *= fDrawDistanceFactor;
                        //*(float*)(regs.esi + 0x84) *= fDrawDistanceFactor;
                        //*(float*)(regs.esi + 0x90) *= fDrawDistanceFactor;
                        //*(float*)(regs.esi + 0x94) *= fDrawDistanceFactor;
                        //*(float*)(regs.esi + 0xA0) *= fDrawDistanceFactor;
                        //*(float*)(regs.esi + 0x9C) *= fDrawDistanceFactor;
                        //*(float*)(regs.esi + 0xA8) *= fDrawDistanceFactor;
                        //*(float*)(regs.esi + 0xAC) *= fDrawDistanceFactor;
                        //*(float*)(regs.esi + 0xB4) *= fDrawDistanceFactor;
                        //*(float*)(regs.esi + 0xB8) *= fDrawDistanceFactor;
                        //*(float*)(regs.esi + 0xC0) *= fDrawDistanceFactor;

                        *(float*)(regs.esi + 0x44) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x48) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x4C) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x40) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x50) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x54) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x58) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x5C) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x60) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x64) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x68) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x6C) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x70) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x74) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x78) *= fDrawDistanceFactor;
                        *(float*)(regs.esi + 0x7C) *= fDrawDistanceFactor;
                    }
                };
                injector::MakeInline<DrawDistHook>(pattern.count(3).get(0).get<void*>(0), pattern.count(3).get(0).get<void*>(6));
                injector::MakeInline<DrawDistHook>(pattern.count(3).get(1).get<void*>(0), pattern.count(3).get(1).get<void*>(6));
                injector::MakeInline<DrawDistHook>(pattern.count(3).get(2).get<void*>(0), pattern.count(3).get(2).get<void*>(6));
            }
        };
    }
} Misc;

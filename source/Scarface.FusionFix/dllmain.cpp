#include "stdafx.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class OptionManager
{
public:
    static inline HWND* hWnd = nullptr;
    static inline bool* bIsInvertX = nullptr;
    static inline bool* bIsInvertY = nullptr;
    static inline int* nMouseLookSensitivity = nullptr;
    static inline bool bGamepadUsed = false;
    static inline bool (*pScarfaceHook_GetMenuActive)() = nullptr;
    
    static inline bool IsInvertX()
    {
        return *bIsInvertX;
    }
    static inline bool IsInvertY()
    {
        return *bIsInvertY;
    }
    static inline float GetMouseLookSensitivity()
    {
        return *nMouseLookSensitivity / 10.0f;
    }
    static inline bool ScarfaceHook_GetMenuActive()
    {
        if (pScarfaceHook_GetMenuActive)
            return pScarfaceHook_GetMenuActive();
        else
            return false;
    }
};

void SetMouseInput(float* dest_x, float* dest_y, float value_x = 0.0f, float value_y = 0.0f)
{
    if (OptionManager::bGamepadUsed)
        return;
    
    if ((*OptionManager::hWnd == GetForegroundWindow()) && !OptionManager::ScarfaceHook_GetMenuActive())
    {
        RECT windowRect;
        POINT MousePos;
        GetCursorPos(&MousePos);
        GetWindowRect(*OptionManager::hWnd, &windowRect);
        int CenterX = ((windowRect.right - windowRect.left) / 2) + windowRect.left;
        int CenterY = ((windowRect.bottom - windowRect.top) / 2) + windowRect.top;

        float sign_x = 1.0f;
        float sign_y = 1.0f;
        if (OptionManager::IsInvertX())
            sign_x = -1.0f;
        if (OptionManager::IsInvertY())
            sign_y = -1.0f;

        if (value_x)
            *dest_x = value_x + ((float)((MousePos.x - CenterX) / 100.0f) * OptionManager::GetMouseLookSensitivity()) * sign_x;
        else
            *dest_x += ((float)((MousePos.x - CenterX) / 100.0f) * OptionManager::GetMouseLookSensitivity()) * sign_x;
        
        if (value_y)
            *dest_y = value_y + -((float)((MousePos.y - CenterY) / 100.0f) * OptionManager::GetMouseLookSensitivity()) * sign_y;
        else
            *dest_y += -((float)((MousePos.y - CenterY) / 100.0f) * OptionManager::GetMouseLookSensitivity()) * sign_y;

        SetCursorPos(CenterX, CenterY);
    }
}

injector::hook_back<char(__fastcall*)(float* mInputs, void* edx, float* angleH, float* angleV, float deltaTime)> hb_GenericVehicleCamera_ApplyRightStickMotion;
char __fastcall GenericVehicleCamera_ApplyRightStickMotion(float* mInputs, void* edx, float* angleH, float* angleV, float deltaTime)
{
    auto H = *angleH;
    auto V = *angleV;
    
    auto ret = hb_GenericVehicleCamera_ApplyRightStickMotion.fun(mInputs, edx, angleH, angleV, deltaTime);

    SetMouseInput(angleH, angleV, H, V);

    if ((GetAsyncKeyState(VK_RBUTTON)))
        return 1;
    else
        return ret;
}

void Init()
{
    CIniReader iniReader("");
    auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    auto bScrollWeaponsWithMouseWheel = iniReader.ReadInteger("MAIN", "ScrollWeaponsWithMouseWheel", 1) != 0;
    static auto fForceAlphaRef = iniReader.ReadFloat("MAIN", "ForceAlphaRef", 0.0f);
    
    if (bSkipIntro)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 20 5F 5E C7 45");
        injector::MakeNOP(pattern.get_first(), 5); //movies
        pattern = hook::pattern("75 22 E8 ? ? ? ? 8B C8");
        injector::MakeNOP(pattern.get_first(), 2); //press to start
    }
    
    auto pattern = hook::pattern("A2 ? ? ? ? A2 ? ? ? ? A2 ? ? ? ? A2 ? ? ? ? A2 ? ? ? ? A2 ? ? ? ? A3");
    OptionManager::bIsInvertX = *pattern.get_first<bool*>(1);
    OptionManager::bIsInvertY = *pattern.get_first<bool*>(6);
    pattern = hook::pattern("E8 ? ? ? ? 56 68 ? ? ? ? E8 ? ? ? ? 83 C4 10");
    OptionManager::nMouseLookSensitivity = *(int**)(injector::GetBranchDestination(pattern.get_first()).as_int() + 5);
    
    pattern = hook::pattern("A3 ? ? ? ? 5B 74 0F");
    OptionManager::hWnd = *pattern.get_first<HWND*>(1);

    pattern = hook::pattern("D9 58 10 83 C4 5C C2 0C 00");
    struct SetOutput_Update_FreeLook
    {
        void operator()(injector::reg_pack& regs)
        {
            SetMouseInput((float*)(regs.eax + 0x0C), (float*)(regs.eax + 0x10));
        }
    }; injector::MakeInline<SetOutput_Update_FreeLook>(pattern.get_first(6));
    injector::MakeRET(pattern.get_first(6+5), 0xC);

    //camera blend disable
    pattern = hook::pattern("83 EC 10 57 8B F9 80 BF");
    injector::MakeRET(pattern.get_first(0), 8);

    //vehicle camera
    pattern = hook::pattern("E8 ? ? ? ? 84 C0 74 0C C6 85");
    hb_GenericVehicleCamera_ApplyRightStickMotion.fun = injector::MakeCALL(pattern.get_first(0), GenericVehicleCamera_ApplyRightStickMotion, true).get();

    //melee camera
    pattern = hook::pattern("D9 59 10 88 41 19");
    struct SetOutput_Update_MeleeTarget
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)(regs.ecx + 0x19) = *(uint8_t*)&regs.eax;
            *(uint8_t*)(regs.ecx + 0x18) = 0;

            SetMouseInput((float*)(regs.ecx + 0x0C), (float*)(regs.ecx + 0x10));
        }
    }; injector::MakeInline<SetOutput_Update_MeleeTarget>(pattern.get_first(3), pattern.get_first(10));

    pattern = hook::pattern("B9 ? ? ? ? 21 4C 24 10 D9 44 24 10 89 54 24 14");
    struct GamepadCheck
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = 0x7FFFFFFF;
            OptionManager::bGamepadUsed = (*(float*)(regs.esp + 0x1C) + *(float*)(regs.esp + 0x18)) != 0.0f;
        }
    }; injector::MakeInline<GamepadCheck>(pattern.get_first(0));

    if (bScrollWeaponsWithMouseWheel)
    {
        static int nMouseWheelValue = 0;
        pattern = hook::pattern("8B B7 ? ? ? ? 85 F6 74 21 90");
        struct GetDeviceDataHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.esi = *(uint32_t*)(regs.edi + 0x264);
                auto controller = std::string_view((char*)regs.edi + 0x40);

                if (controller.starts_with("Mouse"))
                {
                    auto rgdod = (LPDIDEVICEOBJECTDATA)(regs.esp + 0x10);
                    if (rgdod)
                    {
                        nMouseWheelValue = 0;
                        switch (rgdod->dwOfs)
                        {
                        case DIMOFS_Z:
                            nMouseWheelValue = rgdod->dwData;
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }; injector::MakeInline<GetDeviceDataHook>(pattern.get_first(0), pattern.get_first(6));

        pattern = hook::pattern("0F BF 54 24 08 ? ? ? ? 03");
        if (pattern.empty()) pattern = hook::pattern("0F BF 54 24 ? E9");
        struct MouseScroll
        {
            void operator()(injector::reg_pack& regs)
            {
                *(int32_t*)&regs.edx = *(int16_t*)(regs.esp + 0x8);

                if (OptionManager::bGamepadUsed || OptionManager::ScarfaceHook_GetMenuActive())
                    return;
                
                enum
                {
                    weaponleft = 0x9,
                    weaponright = 0xA
                };

                static auto counter = 0;
                if (nMouseWheelValue >= WHEEL_DELTA)
                    counter++;
                else if (nMouseWheelValue <= -WHEEL_DELTA)
                    counter--;
                else
                    counter = 0;

                static constexpr auto duration = 30;
                if (counter > 0 && counter < duration)
                    *(int16_t*)(regs.ecx + weaponleft * 2) = 1; // 1
                else if (counter < 0 && counter > -duration)
                    *(int16_t*)(regs.ecx + weaponright * 2) = 1; // 3

                if (counter < -duration || counter > duration)
                {
                    counter = 0;
                    nMouseWheelValue = 0;
                }
            }
        }; injector::MakeInline<MouseScroll>(pattern.get_first(0));
    }

    if (fForceAlphaRef)
    {
        pattern = hook::pattern("D9 05 ? ? ? ? 8B 4E 14 D9 7C 24 12");
        injector::WriteMemory(pattern.get_first(2), &fForceAlphaRef, true);
    }
}

void InitXidi()
{
    CIniReader iniReader("");
    auto bModernControlScheme = iniReader.ReadInteger("MAIN", "ModernControlScheme", 1) != 0;
    
    if (bModernControlScheme)
    {
        auto SetScarfaceData = (void (*)(bool(*menu)(), void** player, uint32_t struct_offset))GetProcAddress(GetModuleHandleW(L"dinput8.dll"), "SetScarfaceData");
        if (SetScarfaceData)
        {
            auto fnMenuCheck = (bool(*)())injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 84 C0 75 A4")).as_int();
            auto CharacterObject = *hook::get_pattern<void**>("A1 ? ? ? ? 85 C0 74 50", 1);
            SetScarfaceData(fnMenuCheck, CharacterObject, 0x2E8);
        }
    }
}

void InitScarfaceHook()
{
    OptionManager::pScarfaceHook_GetMenuActive = (bool (*)())GetProcAddress(GetModuleHandleW(L"ScarfaceHook.asi"), "ScarfaceHook_GetMenuActive");
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 5E C3"));
        CallbackHandler::RegisterCallback(L"dinput8.dll", InitXidi);
        CallbackHandler::RegisterCallback(L"ScarfaceHook.asi", InitScarfaceHook);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
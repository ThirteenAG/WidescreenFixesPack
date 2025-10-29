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

void CenterMouse()
{
    if (OptionManager::bGamepadUsed)
        return;

    if ((*OptionManager::hWnd == GetForegroundWindow()) && !OptionManager::ScarfaceHook_GetMenuActive())
    {
        RECT windowRect;
        GetWindowRect(*OptionManager::hWnd, &windowRect);
        auto CenterX = ((windowRect.right - windowRect.left) / 2) + windowRect.left;
        auto CenterY = ((windowRect.bottom - windowRect.top) / 2) + windowRect.top;
        SetCursorPos(CenterX, CenterY);
    }
}

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
        auto CenterX = ((windowRect.right - windowRect.left) / 2) + windowRect.left;
        auto CenterY = ((windowRect.bottom - windowRect.top) / 2) + windowRect.top;

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

float __cdecl math__ClampX(float* value, float* min, float* max)
{
    float dummyY = 0.0f;
    SetMouseInput(value, &dummyY, 0.0f, 0.0f);
    return *value;
}

float __cdecl math__ClampY(float* value, float* min, float* max)
{
    float dummyX = 0.0f;
    SetMouseInput(&dummyX, value, 0.0f, 0.0f);
    return *value;
}

void Init()
{
    CIniReader iniReader("");
    auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    auto bScrollWeaponsWithMouseWheel = iniReader.ReadInteger("MAIN", "ScrollWeaponsWithMouseWheel", 1) != 0;
    static auto fForceAlphaRef = iniReader.ReadFloat("TEST", "ForceAlphaRef", 0.0f);
    
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

    //map camera
    pattern = hook::pattern("E8 ? ? ? ? D9 5C 24 2C 83 C4 0C D9 05");
    injector::MakeCALL(pattern.get_first(0), math__ClampX, true); //mousePanXID
    pattern = hook::pattern("E8 ? ? ? ? D9 5C 24 24 D9 05 ? ? ? ? 83");
    injector::MakeCALL(pattern.get_first(0), math__ClampY, true); //mousePanYID
    pattern = hook::pattern("7A 0E D9 44 24 1C B1 01");
    injector::MakeNOP(pattern.get_first(0), 2);
    pattern = hook::pattern("75 2B 8B 44 24 20 89 44 24 10");
    injector::MakeNOP(pattern.get_first(0), 2);
    pattern = hook::pattern("75 2B 8B 44 24 18 89 44 24 10 21 54 24 10");
    injector::MakeNOP(pattern.get_first(0), 2);
    pattern = hook::pattern("7A 0E D9 44 24 1C D8 4C 24 18");
    injector::MakeNOP(pattern.get_first(0), 2);
    pattern = hook::pattern("7A 08 DD D8 D9 05 ? ? ? ? D9 44 24 20 8D BE ? ? ? ? D8 C9");
    injector::MakeNOP(pattern.get_first(0), 2);

    pattern = hook::pattern("B9 ? ? ? ? 21 4C 24 10 D9 44 24 10 89 54 24 14");
    struct GamepadCheck
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = 0x7FFFFFFF;
            OptionManager::bGamepadUsed = (*(float*)(regs.esp + 0x1C) + *(float*)(regs.esp + 0x18)) != 0.0f;
        }
    }; injector::MakeInline<GamepadCheck>(pattern.get_first(0));

    pattern = hook::pattern("B8 ? ? ? ? C6 05 ? ? ? ? ? A3 ? ? ? ? A3 ? ? ? ? E8");
    struct CursorReset1
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = 0x18;
            CenterMouse();
        }
    }; injector::MakeInline<CursorReset1>(pattern.get_first(0));

    pattern = hook::pattern("83 C4 0C 88 9E ? ? ? ? 5E 5B C2 08 00");
    struct CursorReset2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)(regs.esi + 0x177) = regs.ebx & 0xFF;
            if ((regs.ebx & 0xFF) == 0)
                CenterMouse();
        }
    }; injector::MakeInline<CursorReset2>(pattern.get_first(3), pattern.get_first(9));
    
    pattern = hook::pattern("8B 47 0C 50 8D 4C 24 38 51 8B CE E8 ? ? ? ? 80 BE");
    struct CursorReset3
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.edi + 0xC);
            regs.ecx = regs.esp + 0x38 - 4;
            CenterMouse();
        }
    }; injector::MakeInline<CursorReset3>(pattern.get_first(0), pattern.get_first(8));
    injector::WriteMemory<uint8_t>(pattern.get_first(5), 0x50, true);

    pattern = hook::pattern("8B 97 ? ? ? ? 52 8B CE E8 ? ? ? ? 5F");
    struct CursorReset4
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = *(uint32_t*)(regs.edi + 0x84);
            CenterMouse();
        }
    }; injector::MakeInline<CursorReset4>(pattern.get_first(0), pattern.get_first(6));

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
                    *(int16_t*)(regs.ecx + weaponright * 2) = 1; // 2

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
        typedef bool (*RegisterProfileCallbackFunc)(const wchar_t* (*callback)());
        auto xidiModule = GetModuleHandleW(L"Xidi.32.dll");

        if (xidiModule)
        {
            auto RegisterProfileCallback = (RegisterProfileCallbackFunc)GetProcAddress(xidiModule, "RegisterProfileCallback");
            if (RegisterProfileCallback)
            {
                static auto fnMenuCheck = (bool(*)())injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 84 C0 75 A4")).as_int();
                static auto CharacterObject = *hook::get_pattern<void**>("A1 ? ? ? ? 85 C0 74 50", 1);
                static auto PilotStateOffset = 0x2E8;

                RegisterProfileCallback([]() -> const wchar_t*
                {
                    if (fnMenuCheck && !fnMenuCheck())
                    {
                        auto player = *CharacterObject;
                        if (player && *(uint32_t*)(*(uint32_t*)CharacterObject + PilotStateOffset) > 0)
                        {
                            return L"InCar";
                        }
                        else
                        {
                            return L"OnFoot";
                        }
                    }
                    return nullptr;
                });
            }
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
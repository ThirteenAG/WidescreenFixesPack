#include "stdafx.h"

class OptionManager
{
public:
    static inline HWND* hWnd = nullptr;
    static inline bool* bIsInvertX = nullptr;
    static inline bool* bIsInvertY = nullptr;
    static inline int* nMouseLookSensitivity = nullptr;
    static inline bool bGamepadUsed = false;
    
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
        static bool (*pScarfaceHook_GetMenuActive)() = nullptr;
        static std::once_flag flag;
        std::call_once(flag, []()
        {
            auto GetScarfaceHook = []() -> HMODULE {
                constexpr auto dll = L"ScarfaceHook.asi";
                auto hm = GetModuleHandleW(dll);
                return (hm ? hm : LoadLibraryW(dll));
            };
            pScarfaceHook_GetMenuActive = (bool (*)())GetProcAddress(GetScarfaceHook(), "ScarfaceHook_GetMenuActive");
        });
        
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
    //skipintro
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
}

void InitXidi()
{
    auto SetScarfaceData = (void (*)(uint32_t* menu, uint8_t* state))GetProcAddress(GetModuleHandleW(L"dinput8.dll"), "SetScarfaceData");
    if (SetScarfaceData)
    {
        auto nMenuCheck = *(uint32_t**)(injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 88 43 60")).as_int() + 1);
        auto bStateCheck = *hook::get_pattern<uint8_t*>("B9 ? ? ? ? E8 ? ? ? ? 5E C3", 1);
        SetScarfaceData(nMenuCheck, bStateCheck);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 5E C3"));
        CallbackHandler::RegisterCallback(L"dinput8.dll", InitXidi);
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
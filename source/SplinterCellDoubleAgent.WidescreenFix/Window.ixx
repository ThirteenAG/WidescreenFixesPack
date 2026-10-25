module;

#include "stdafx.h"

export module Window;

export void InitWindow()
{
    //icon fix
    using RegisterClassW = ATOM(WINAPI*)(WNDCLASSEXW*);
    auto RegisterClassExWProxy = [](WNDCLASSEXW* lpwcx) -> ATOM
    {
        lpwcx->hIcon = CreateIconFromResourceICO(IDR_SCDAICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
        lpwcx->hIconSm = CreateIconFromResourceICO(IDR_SCDAICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
        return RegisterClassExW(lpwcx);
    };

    using RegisterClassA = ATOM(WINAPI*)(WNDCLASSEXA*);
    auto RegisterClassExAProxy = [](WNDCLASSEXA* lpwcx) -> ATOM
    {
        lpwcx->hIcon = CreateIconFromResourceICO(IDR_SCDAICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
        lpwcx->hIconSm = CreateIconFromResourceICO(IDR_SCDAICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
        return RegisterClassExA(lpwcx);
    };

    auto pattern = hook::module_pattern(GetModuleHandle(L"Window"), "FF 15 ? ? ? ? 66 85 C0 0F 85 ? ? ? ? 68 F5 02 00 00");
    ProtectedGameRef<RegisterClassW>(*pattern.get_first<RegisterClassW*>(2)) = static_cast<RegisterClassW>(RegisterClassExWProxy);
    pattern = hook::module_pattern(GetModuleHandle(L"Window"), "FF 15 ? ? ? ? 66 85 C0 75 18 68 03 03 00 00");
    ProtectedGameRef<RegisterClassA>(*pattern.get_first<RegisterClassA*>(2)) = static_cast<RegisterClassA>(RegisterClassExAProxy);
}

module;

#include <stdafx.h>
#include <initguid.h>

export module Eax;

DEFINE_GUID(CLSID_DirectSound, 0x47d4d946, 0x62e8, 0x11cf, 0x93, 0xbc, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(CLSID_DirectSound8, 0x3901cc3f, 0x84b5, 0x4fa4, 0xba, 0x35, 0xaa, 0x81, 0x72, 0xb8, 0xa0, 0x9b);
HRESULT WINAPI CustomCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv)
{
    HRESULT hr = REGDB_E_KEYMISSING;
    HMODULE hDll = NULL;

    if (rclsid == CLSID_DirectSound || rclsid == CLSID_DirectSound8)
        hDll = ::LoadLibrary(L"dsound.dll");

    if (hDll == NULL || GetProcAddress(hDll, "IsUltimateASILoader") != NULL)
        return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);

    typedef HRESULT(__stdcall* pDllGetClassObject)(IN REFCLSID rclsid, IN REFIID riid, OUT LPVOID FAR* ppv);

    pDllGetClassObject GetClassObject = (pDllGetClassObject)::GetProcAddress(hDll, "DllGetClassObject");
    if (GetClassObject == NULL)
    {
        ::FreeLibrary(hDll);
        return hr;
    }

    IClassFactory* pIFactory;

    hr = GetClassObject(rclsid, IID_IClassFactory, (LPVOID*)&pIFactory);

    if (!SUCCEEDED(hr))
        return hr;

    hr = pIFactory->CreateInstance(pUnkOuter, riid, ppv);
    pIFactory->Release();

    return hr;
}

export void InitEAX()
{
    auto pattern = hook::pattern("24 ? 0B C2 8B 0D ? ? ? ? ? ? 89 42");
    injector::WriteMemory<uint8_t>(pattern.get_first(1), 0xFF, true);

    IATHook::Replace(GetModuleHandle(L"eax"), "OLE32.DLL",
        std::forward_as_tuple("CoCreateInstance", CustomCoCreateInstance)
    );
}
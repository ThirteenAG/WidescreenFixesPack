module;

#include <stdafx.h>
#include "Shared/DirectSoundActivation.hpp"
#include <initguid.h>

export module Eax;

DEFINE_GUID(CLSID_DirectSound, 0x47d4d946, 0x62e8, 0x11cf, 0x93, 0xbc, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(CLSID_DirectSound8, 0x3901cc3f, 0x84b5, 0x4fa4, 0xba, 0x35, 0xaa, 0x81, 0x72, 0xb8, 0xa0, 0x9b);
HRESULT WINAPI CustomCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv)
{
    if (rclsid == CLSID_DirectSound || rclsid == CLSID_DirectSound8)
        return DirectSoundActivation::Create(rclsid, pUnkOuter, dwClsContext, riid, ppv);
    return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}

export void InitEAX()
{
    IATHook::Replace(GetModuleHandle(L"eax"), "OLE32.DLL",
        std::forward_as_tuple("CoCreateInstance", CustomCoCreateInstance)
    );
}
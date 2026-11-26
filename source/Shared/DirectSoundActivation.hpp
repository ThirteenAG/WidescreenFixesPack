#pragma once

#include <atomic>
#include <objbase.h>

namespace DirectSoundActivation
{
    inline HRESULT Create(REFCLSID clsid, IUnknown* outer, DWORD context, REFIID iid, void** object)
    {
        if (!object)
            return E_POINTER;
        *object = nullptr;
        HMODULE module = LoadLibraryW(L"dsound.dll");
        if (!module)
            return CoCreateInstance(clsid, outer, context, iid, object);
        if (GetProcAddress(module, "IsUltimateASILoader"))
        {
            FreeLibrary(module);
            return CoCreateInstance(clsid, outer, context, iid, object);
        }

        using GetFactory = HRESULT (WINAPI*)(REFCLSID, REFIID, void**);
        auto getFactory = reinterpret_cast<GetFactory>(GetProcAddress(module, "DllGetClassObject"));
        HRESULT result = REGDB_E_KEYMISSING;
        if (getFactory)
        {
            IClassFactory* factory = nullptr;
            result = getFactory(clsid, IID_PPV_ARGS(&factory));
            if (SUCCEEDED(result))
            {
                result = factory->CreateInstance(outer, iid, object);
                factory->Release();
                if (SUCCEEDED(result))
                {
                    // Directly instantiated objects can outlive this call.
                    static std::atomic<HMODULE> retained{ nullptr };
                    HMODULE expected = nullptr;
                    if (retained.compare_exchange_strong(expected, module))
                        return result;
                }
            }
        }
        FreeLibrary(module);
        return result;
    }
}

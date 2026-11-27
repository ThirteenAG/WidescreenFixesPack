module;

#include <stdafx.h>
#include <locale>
#include <memory>
#include <ostream>
#include "FixSettings.hpp"
#include <dolphin/dolphin.h>
#include <LEDEffects.h>

export module LightSync;

import Game;

namespace
{
    constexpr wchar_t lightingSdk[] = L"sdk_legacy_led_x64.dll";
    decltype(&LoadLibraryW) loadLibrary;
    decltype(&std::locale::classic) classicLocale;
    // The MSVC x64 member call places the returned locale in a caller-provided buffer.
    std::locale* (*imbueStream)(std::ios*, void*, const std::locale*);

    template<class Number>
    std::ostream& (*writeNumber)(std::ostream*, Number);

    template<class Number>
    std::ostream& WriteProtocolNumber(std::ostream* stream, Number number)
    {
        // The SDK serializes its local port and hexadecimal integration ID with integer stream operators.
        // Give those SDK streams the classic locale so Dolphin's digit grouping stays out of the protocol.
        alignas(std::locale) std::byte previous[sizeof(std::locale)];
        std::destroy_at(imbueStream(static_cast<std::ios*>(stream), previous, &classicLocale()));
        return writeNumber<Number>(stream, number);
    }

    HMODULE WINAPI LoadLightingSdk(LPCWSTR path)
    {
        const auto library = loadLibrary(path);
        if (!library || library != GetModuleHandleW(lightingSdk))
            return library;

        const auto runtime = GetModuleHandleW(L"MSVCP140.dll");
        if (!runtime)
            return library;

        constexpr char shortOutput[] = "??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@G@Z";
        constexpr char longOutput[] = "??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@_K@Z";
        classicLocale = reinterpret_cast<decltype(classicLocale)>(GetProcAddress(runtime, "?classic@locale@std@@SAAEBV12@XZ"));
        imbueStream = reinterpret_cast<decltype(imbueStream)>(GetProcAddress(runtime,
            "?imbue@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z"));
        writeNumber<unsigned short> = reinterpret_cast<decltype(writeNumber<unsigned short>)>(GetProcAddress(runtime, shortOutput));
        writeNumber<uint64_t> = reinterpret_cast<decltype(writeNumber<uint64_t>)>(GetProcAddress(runtime, longOutput));
        if (classicLocale && imbueStream && writeNumber<unsigned short> && writeNumber<uint64_t>)
            IATHook::Replace(library, "MSVCP140.dll",
                std::forward_as_tuple(shortOutput, reinterpret_cast<void*>(&WriteProtocolNumber<unsigned short>)),
                std::forward_as_tuple(longOutput, reinterpret_cast<void*>(&WriteProtocolNumber<uint64_t>)));
        return library;
    }
}

// Logitech LIGHTSYNC (MAIN.LightSyncRGB): the player's health as the keyboard colour and a cardiogram, purple when
// poisoned.

export namespace LightSync
{
    void Init()
    {
        if (!FixSettings().Bool("MAIN.LightSyncRGB"))
            return;

        // The signed SDK wrapper loads G HUB's library on its first initialization.
        const auto module = module_path_detail::ThisModule();
        loadLibrary = &LoadLibraryW;
        if (module)
            IATHook::Replace(module, "KERNEL32.dll", std::forward_as_tuple("LoadLibraryW", reinterpret_cast<void*>(&LoadLightingSdk)));

        LEDEffects::Inject([]()
        {
             [&]()
             {
                 __try
                 {
                     using namespace powerpc;

                     std::this_thread::sleep_for(std::chrono::milliseconds(100));
                     
                     auto gameVersion = Game::Detect(true);
                     
                     if (gameVersion)
                     {
                         if (Dolphin::MemoryAddrValid(Game::sPlayerHealth))
                         {
                             int16_t health1 = (int16_t)swap16(*Game::sPlayerHealth);
                             auto poisoned = *(int8_t*)((uintptr_t)Game::sPlayerHealth + 0xC2);
                     
                             if (gameVersion == RE3)
                                 poisoned = *(int8_t*)((uintptr_t)Game::sPlayerHealth + 0x6) % 4 != 0;
                     
                             if (health1 > 1)
                             {
                                 if (health1 <= 20) {
                                     if (!poisoned)
                                         LEDEffects::SetLighting(26, 4, 4, true, false); //red
                                     else
                                         LEDEffects::SetLighting(51, 4, 53, true, false); //purple
                                     LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                                 }
                                 else if (health1 <= 40) {
                                     if (!poisoned)
                                         LEDEffects::SetLighting(100, 39, 13, true, false); //orange
                                     else
                                         LEDEffects::SetLighting(51, 4, 53, true, false); //purple
                                     LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                                 }
                                 else if (health1 <= 100) {
                                     if (!poisoned)
                                         LEDEffects::SetLighting(52, 48, 0, true, false); //orange
                                     else
                                         LEDEffects::SetLighting(51, 4, 53, true, false); //purple
                                     LEDEffects::DrawCardiogram(67, 0, 0, 0, 0, 0); //orange
                                 }
                                 else {
                                     if (!poisoned)
                                         LEDEffects::SetLighting(0, 48, 0, true, false);  //green
                                     else
                                         LEDEffects::SetLighting(51, 4, 53, true, false); //purple
                                     LEDEffects::DrawCardiogram(0, 100, 0, 0, 0, 0); //green
                                 }
                             }
                             else
                             {
                                 LEDEffects::SetLighting(26, 4, 4, false); //red
                                 LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
                             }
                         }
                         else
                         {
                             LogiLedStopEffects();
                             LEDEffects::SetLighting(52, 0, 0); //logo red
                         }
                     }
                     else
                     {
                         LogiLedStopEffects();
                         LEDEffects::SetLighting(52, 0, 0); //logo red
                     }
                 }
                 __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
                 {
                 }
             }();
        });
    }
}

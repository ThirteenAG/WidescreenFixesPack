module;

#include <stdafx.h>
#include <cmath>

export module MenuBindings;

import EngineText;

export namespace REMenu
{
    using REText::Read;
    using REText::Write;
    inline uintptr_t* menuVtable = nullptr;
    inline void* initialize = nullptr;
    inline void* processInput = nullptr;
    inline GameRef<void*> inputManager, pad, keyboard, sceneManager, mouseManager, gameLanguage;
    inline uint32_t (__thiscall* pressed)(void*, uint32_t, bool) = nullptr;
    inline uint32_t (__thiscall* repeated)(void*, uint32_t) = nullptr;
    inline void (__thiscall* prepare)(void*, uint32_t) = nullptr;
    inline bool (__thiscall* mouseSelection)(void*) = nullptr;
    inline bool (__thiscall* mouseHit)(void*, void*, int) = nullptr;
    inline uint8_t* countInstruction = nullptr;
    inline uint8_t* secondCountInstruction = nullptr;
    inline bool originalCompiler = false;

    inline void* Relative(const uint8_t* call)
    {
        return const_cast<uint8_t*>(call) + 5 + Read<int32_t>(call, 1);
    }

    inline bool MousePoint(void* gui, POINT& point, bool& clicked, bool& moved)
    {
        auto mouse = mouseManager.get();
        if (!mouse) return false;
        const auto scaleX = Read<float>(gui, 96), scaleY = Read<float>(gui, 100);
        if (!std::isfinite(scaleX) || !std::isfinite(scaleY) || scaleX <= 0 || scaleY <= 0) return false;
        const auto x = Read<int32_t>(mouse, 32), y = Read<int32_t>(mouse, 36);
        const auto logicalX = (x - Read<float>(gui, 64)) / scaleX;
        const auto logicalY = (y - Read<float>(gui, 68)) / scaleY;
        if (!std::isfinite(logicalX) || !std::isfinite(logicalY) ||
            std::abs(logicalX) > 16384 || std::abs(logicalY) > 16384) return false;
        point.x = static_cast<LONG>(logicalX);
        point.y = static_cast<LONG>(logicalY);
        clicked = (Read<uint32_t>(mouse, 64) & 1) != 0;
        moved = x != Read<int32_t>(mouse, 256) || y != Read<int32_t>(mouse, 260);
        return true;
    }

    inline bool Bind()
    {
        menuVtable = nullptr;
        initialize = processInput = nullptr;
        countInstruction = secondCountInstruction = nullptr;
        auto init = hook::pattern("56 6A 00 6A 01 68 ? ? ? ? 8B F1 E8 ? ? ? ? 8B CE C6 86 41 02 00 00 01 E8 ? ? ? ? 68 DF 05 00 00");
        auto entry = init.get_first<uint8_t>();
        char signature[32]{};
        const auto pointer = reinterpret_cast<uintptr_t>(entry);
        sprintf_s(signature, "%02X %02X %02X %02X", pointer & 255, (pointer >> 8) & 255,
            (pointer >> 16) & 255, (pointer >> 24) & 255);
        auto references = REText::DataPattern(signature);
        menuVtable = references.count(1).get_first<uintptr_t>(-20);
        auto currentInput = hook::pattern("53 55 56 57 8B F9 8B 0D ? ? ? ? 6A 00 E8 ? ? ? ? 8B 0D ? ? ? ? 6A 00 6A 00 E8 ? ? ? ? 8B 0D ? ? ? ? 8B F0 6A 00 E8");
        auto oldInput = hook::pattern("51 53 55 56 57 8B F1 8B 0D ? ? ? ? 6A 00 E8 ? ? ? ? 8B 0D ? ? ? ? 6A 00 6A 00 E8 ? ? ? ? 8B 0D ? ? ? ? 6A 00 8B F8 E8");
        originalCompiler = !oldInput.empty(); // Original executable layout
        auto input = originalCompiler ? oldInput.get_first<uint8_t>() : currentInput.get_first<uint8_t>();
        const size_t shift = originalCompiler ? 1 : 0;
        auto managerAddress = Read<void**>(input, 8 + shift);
        inputManager.SetAddress(managerAddress);
        prepare = reinterpret_cast<decltype(prepare)>(Relative(input + 14 + shift));
        pressed = reinterpret_cast<decltype(pressed)>(Relative(input + 29 + shift));
        repeated = reinterpret_cast<decltype(repeated)>(Relative(input + 44 + shift));
        mouseSelection = reinterpret_cast<decltype(mouseSelection)>(Relative(input + (originalCompiler ? 56 : 53)));
        const auto padOffset = originalCompiler ? 63 : 60;
        auto padAddress = Read<void**>(input, padOffset);
        pad.SetAddress(padAddress);
        if (originalCompiler)
            countInstruction = hook::range_pattern(reinterpret_cast<uintptr_t>(input), reinterpret_cast<uintptr_t>(input) + 1100,
                "8D 1C 5D 06 00 00 00").count(1).get_first<uint8_t>();
        else
        {
            auto count = hook::range_pattern(reinterpret_cast<uintptr_t>(input), reinterpret_cast<uintptr_t>(input) + 1100,
                "8D 04 45 05 00 00 00 3B C8 8B C6 0F 83 ? ? ? ? 83 F9 09");
            auto secondCount = hook::range_pattern(reinterpret_cast<uintptr_t>(input), reinterpret_cast<uintptr_t>(input) + 1100,
                "8D 04 45 05 00 00 00 89 87 4C 03 00 00 8B 0D ? ? ? ? E8");
            countInstruction = count.get_first<uint8_t>();
            secondCountInstruction = secondCount.get_first<uint8_t>();
        }
        auto key = hook::range_pattern(reinterpret_cast<uintptr_t>(input), reinterpret_cast<uintptr_t>(input) + 1100,
            "8B 15 ? ? ? ? 8B 8A 74 0A 00 00 8B C1 C1 E8 05 83 E1 1F 8B 84 82 C0 0A 00 00 D3 E8 A8 01");
        keyboard.SetAddress(Read<void**>(key.count(1).get_first(), 2));
        auto currentGuide = hook::pattern("56 8B 35 ? ? ? ? 81 C1 60 03 00 00 50 E8 ? ? ? ? 50 8B CE E8 ? ? ? ? 5E 83 C4 24 C2 04 00");
        auto oldGuide = hook::pattern("56 FF 30 8B 35 ? ? ? ? 81 C1 60 03 00 00 E8 ? ? ? ? 50 8B CE E8 ? ? ? ? 5E 83 C4 24 C2 04 00");
        auto sceneAddress = currentGuide.size() ? Read<void**>(currentGuide.get_first(), 3) : Read<void**>(oldGuide.get_first(), 5);
        sceneManager.SetAddress(sceneAddress);
        auto hit = hook::pattern("8B 91 F8 00 00 00 85 D2 74 10 8B 44 24 04 8B 04 82 89 44 24 04 E9");
        auto hitTarget = Relative(hit.get_first<uint8_t>() + 21);
        mouseHit = reinterpret_cast<decltype(mouseHit)>(hitTarget);
        auto mouseReference = hook::range_pattern(reinterpret_cast<uintptr_t>(hitTarget), reinterpret_cast<uintptr_t>(hitTarget) + 100,
            "8B 0D ? ? ? ? 8D 44 24 ? 50");
        mouseManager.SetAddress(Read<void**>(mouseReference.count(1).get_first(), 2));
        // The options' language setter stores the chosen language at +0x28 of this object
        gameLanguage.SetAddress(Read<void**>(hook::pattern("A1 ? ? ? ? BA 02 00 00 00 8B CF 89 50 28").get_first(), 1));
        REText::Bind();
        initialize = entry;
        processInput = input;
        return true;
    }
}

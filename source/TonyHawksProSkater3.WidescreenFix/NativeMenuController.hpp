#pragma once
#include <cstddef>
#include <cstdint>
#include "NativeMenuABI.hpp"

namespace THPS3Menu
{
    struct Event { uint32_t type, target; Structure* params; };
    static_assert(sizeof(Event) == 12);
    struct Controller;
    using ControllerDestroy = void*(__fastcall*)(Controller*, void*, uint32_t);
    using ControllerInit = void(__fastcall*)(Controller*, void*);
    using ControllerExecute = uint8_t(__fastcall*)(Controller*, void*, Event*);
    struct ControllerTable { ControllerDestroy destroy; ControllerInit init; ControllerExecute execute; };
    static_assert(sizeof(ControllerTable) == 12);
    struct ControllerHeader
    {
        ControllerTable* table = nullptr;
        uint32_t nodeTable = 0;
        void* nodeValue = nullptr;
        uint32_t reserved = 0;
        void* next = nullptr;
        void* previous = nullptr;
        void* owner = nullptr;
    };
    static_assert(sizeof(ControllerHeader) == 28);
    static_assert(offsetof(ControllerHeader, nodeValue) == 8 && offsetof(ControllerHeader, owner) == 24);

    enum class Command { Open, Accept, Back, Left, Right, Focus, Enter };
    struct Controller
    {
        ControllerHeader native;
        uint64_t generation = 0;
        uint32_t row = 0;
        Command command = Command::Accept;
        bool retired = false;
        Controller() = default;
        Controller(const Controller&) = delete;
        Controller& operator=(const Controller&) = delete;
        Controller(Controller&&) = delete;
        Controller& operator=(Controller&&) = delete;
    };
    static_assert(offsetof(Controller, native) == 0);

    // The native UI borrows this embedded list node. Its owner destructor only
    // unlinks it; our storage stays alive through event, input and task unwind.
    // Native4274A0 treats an event target as QB code even for a CFn symbol, so
    // this bridge deliberately uses the proven native controller interface.
    inline void InitializeController(Controller& controller, ControllerTable& table)
    {
        // Fresh value-initialized storage only; never reinitialize an attached
        // node. Keep it at a stable address for its entire native membership.
        reinterpret_cast<void(__thiscall*)(ControllerHeader*)>(0x4CF990)(&controller.native);
        controller.native.table = &table;
    }
    inline void AttachController(void* owner, Controller& controller)
    {
        // Caller proves owner identity immediately before attachment. The
        // manager's missing-ID branch would otherwise pass a null owner.
        reinterpret_cast<void(__thiscall*)(void*, ControllerHeader*)>(0x4CD610)(owner, &controller.native);
    }
    inline void UnlinkController(Controller& controller)
    {
        // Only after every dispatch/input/task depth is zero: unlinking changes
        // the next pointer that an outer native iterator still reads.
        auto* table = controller.native.table;
        reinterpret_cast<void(__thiscall*)(ControllerHeader*)>(0x442680)(&controller.native);
        controller.native.table = table;
        controller.native.owner = nullptr;
        controller.retired = true;
    }
}

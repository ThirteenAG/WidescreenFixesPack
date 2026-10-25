#pragma once
#include "NativeMenuUnwind.hpp"

namespace HD2Menu
{
    inline bool Image(HMODULE module, uintptr_t preferred, uint32_t exactSize)
    {
        if (!module || reinterpret_cast<uintptr_t>(module) != preferred
            || !Contracts::Readable(module, sizeof(IMAGE_DOS_HEADER))) return false;
        auto base = reinterpret_cast<uintptr_t>(module);
        auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 || dos->e_lfanew > 0x100000) return false;
        auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        return Contracts::Readable(nt, sizeof(*nt)) && nt->Signature == IMAGE_NT_SIGNATURE
            && nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386
            && nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC
            && nt->OptionalHeader.ImageBase == preferred && nt->OptionalHeader.SizeOfImage == exactSize;
    }

    inline void Init()
    {
        if (Storage()) return;
        State* state = nullptr;
        try { state = new State; } catch (...) { return; }
        HMODULE module = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &module)) { delete state; return; }
        state->module = module;
        auto game = GetModuleHandleW(nullptr);
        auto graphics = GetModuleHandleW(L"LS3DF.dll");
        // The executable keeps its exact preferred layout. The renderer may
        // relocate: compare its exact bytes after undoing declared PE fixups.
        if (!Image(game, 0x400000, 0x7B8000) || !Contracts::Matches(game, Contracts::Game, 0x7B8000))
        {
            state->Log("unsupported executable/native menu contracts (readable1.12 only)");
            delete state;
            return;
        }
        if (!graphics || !Contracts::Matches(graphics, Contracts::Frames, 0xD16FC, 0x10000000))
        {
            state->Log("unsupported LS3DF body/layout; loaded base "
                + std::to_string(reinterpret_cast<uintptr_t>(graphics)));
            if (graphics)
            {
                const auto base = reinterpret_cast<uintptr_t>(graphics);
                for (const auto& prefix : Contracts::Frames)
                {
                    const auto address = reinterpret_cast<const void*>(base + prefix.rva);
                    if (!Contracts::Readable(address, sizeof(prefix.bytes))
                        || std::memcmp(address, prefix.bytes, sizeof(prefix.bytes)))
                    {
                        state->Log("first differing LS3DF native contract RVA " + std::to_string(prefix.rva));
                        break;
                    }
                }
            }
            delete state;
            return;
        }
        // Pin before any retained callback, string/property, hook or registry
        // pointer can reference code/data in this ASI. It cannot dynamically unload.
        HMODULE pinned = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &pinned) || pinned != module)
        {
            state->Log("native bridge module pin refused");
            delete state;
            return;
        }
        ABI::game = reinterpret_cast<uintptr_t>(game);
        Context::graphics = reinterpret_cast<uintptr_t>(graphics);
        ABI::ready = true;
        // The LS3DF module notification can precede native driver creation.
        // Resolve that pure getter at a complete world frame, before factories.
        Storage() = state;
        try
        {
            auto hook = [](uintptr_t original, void* callback) {
                return safetyhook::create_inline(reinterpret_cast<void*>(ABI::game + original - 0x400000), callback);
            };
            state->input = hook(0x66B3E0, reinterpret_cast<void*>(&Input));
            state->keyboard = hook(0x668670, reinterpret_cast<void*>(&Keyboard));
            state->dispatch = hook(0x65C900, reinterpret_cast<void*>(&Dispatch));
            state->queue = hook(0x66C770, reinterpret_cast<void*>(&Queue));
            state->groupDraw = hook(0x656E30, reinterpret_cast<void*>(&GroupDraw));
            state->textDraw = hook(0x647EE0, reinterpret_cast<void*>(&TextDraw));
            state->windowDestroy = hook(0x668250, reinterpret_cast<void*>(&WindowDestroyed));
            state->buttonDestroy = hook(0x65FD50, reinterpret_cast<void*>(&ButtonDestroyed));
            state->managerDestroy = hook(0x66B140, reinterpret_cast<void*>(&ManagerDestroyed));
            state->world = hook(0x6C20F0, reinterpret_cast<void*>(&World));
            state->enabled = state->input && state->keyboard && state->dispatch && state->queue && state->groupDraw
                && state->textDraw && state->windowDestroy && state->buttonDestroy && state->managerDestroy && state->world;
        }
        catch (...) { state->enabled = false; }
        if (!state->enabled)
        {
            state->world = {}; state->input = {}; state->keyboard = {}; state->dispatch = {}; state->queue = {};
            state->groupDraw = {}; state->textDraw = {}; state->windowDestroy = {}; state->buttonDestroy = {}; state->managerDestroy = {};
            state->Log("native hook transaction refused; all installed hooks restored");
            ABI::ready = false;
            return;
        }
    }
}

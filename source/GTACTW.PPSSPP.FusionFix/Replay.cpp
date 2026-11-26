#include "Game.hpp"

namespace ctw
{
    namespace
    {
        SafetyMipsInline script, manager;
        alignas(16) uint32_t replayStatus[712];
        uintptr_t textBegin, textEnd;

        bool AvailabilityQuery(uintptr_t caller)
        {
            // The dynamically loaded board uses separate status queries to
            // count visible missions, build the list, and unlock the portrait.
            constexpr uint32_t count[] = {
                0x00042080, 0x00442021, 0x8C840000, 0x3205000F,
                0x00A52821, 0x00A42006, 0x30840003, 0x109E0004,
                0x00000000, 0x92C40615,
            };
            constexpr uint32_t list[] = {
                0x00042080, 0x00442021, 0x8C840000, 0x3245000F,
                0x00A52821, 0x00A42006, 0x30840003, 0x34050003,
                0x10850004, 0x00000000, 0x92040615,
            };
            constexpr uint32_t portrait[] = {
                0x00042080, 0x00442021, 0x8C840000, 0x3205000F,
                0x00A52821, 0x00A42006, 0x30840003, 0x34050003,
                0x10850004, 0x00000000, 0x92A40615,
            };
            if ((caller >= textBegin && caller < textEnd) || (caller & 3) || !Guest(caller, 4 + sizeof(list)))
                return false;
            const auto pda = at<uintptr_t>(Address<0x08C11258>());
            if (!Guest(pda, 200) || at<unsigned>(pda + 196) != 0x27)
                return false;

            // PPSSPP may replace the first instruction of the return block
            // with a JIT runblock opcode. The rest of the block is intact.
            const uint32_t head = at<uint32_t>(caller);
            const bool compiled = (head & 0xFF000000u) == 0x68000000u;
            const auto tail = reinterpret_cast<const void*>(caller + 4);
            if ((compiled || head == 0x00122102) && std::memcmp(tail, list, sizeof(list)) == 0)
                return true;
            return (compiled || head == 0x00102102) &&
                (std::memcmp(tail, count, sizeof(count)) == 0 ||
                 std::memcmp(tail, portrait, sizeof(portrait)) == 0);
        }

        uintptr_t StatusView(uintptr_t original, uintptr_t caller)
        {
            if (!Guest(original, sizeof(replayStatus)) || !AvailabilityQuery(caller))
                return original;
            std::memcpy(replayStatus, reinterpret_cast<const void*>(original), sizeof(replayStatus));
            // The replay board alone treats Xin's bonus missions as completed.
            for (unsigned mission : {103u, 104u})
                replayStatus[mission >> 4] |= 3u << (2 * (mission & 15));
            return reinterpret_cast<uintptr_t>(replayStatus);
        }

        uintptr_t Script()
        {
            const auto caller = uintptr_t(__builtin_return_address(0));
            return StatusView(script.call<uintptr_t>(), caller);
        }

        uintptr_t Manager(uintptr_t object)
        {
            const auto caller = uintptr_t(__builtin_return_address(0));
            return StatusView(manager.call<uintptr_t>(object), caller);
        }
    }

    void InstallReplay()
    {
        if (!Option("UnlockXinMissionsInReplayBoard", 0))
            return;
        SceKernelModuleInfo info{};
        if (!FindModule("CTW", info))
            return;
        textBegin = info.text_addr;
        textEnd = textBegin + info.text_size;
        safetymips::Options caller;
        caller.preserve_game_callees = false;
        script = safetymips::create_inline(Address<0x08A05F28>(), Script, caller);
        manager = safetymips::create_inline(Address<0x08A36E2C>(), Manager, caller);
    }
}

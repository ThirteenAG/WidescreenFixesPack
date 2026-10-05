#include "../../external/injector/include/ps2/hooks.hpp"
#include "../../external/injector/include/psp/hooks.hpp"
#include "../../external/injector/include/ps2/patches.hpp"
#include "../../external/injector/include/psp/patches.hpp"
#include <cassert>
#include <type_traits>
static_assert(!std::is_copy_constructible<pcsx2::Hook>::value, "hooks own code");
static_assert(std::is_nothrow_move_constructible<pcsx2::Hook>::value, "moves do not allocate");
static_assert(!std::is_copy_constructible<pcsx2::Patch>::value, "patches own saved words");
static_assert(std::is_nothrow_move_constructible<psp::Patch>::value, "moves do not allocate");
static int patch_read(void* user, uint32_t address, void* out, size_t size)
{ assert(address == 0x1000 && size == 8); memcpy(out, user, size); return 1; }
static int patch_write(void* user, uint32_t address, const void* in, size_t size)
{ assert(address == 0x1000 && size == 8); memcpy(user, in, size); return 1; }
static void patch_flush(void*, uint32_t, size_t) {}
static void ee_named_callback(pcsx2::Registers& regs, void* user)
{ regs.a1 += *static_cast<uint32_t*>(user); }
static void psp_named_callback(psp::Registers& regs)
{ regs.a1 = 4; }
extern "C" __declspec(dllexport) int cpp_patch_api()
{
    uint32_t memory[] = {0x3c081234, 0x35085678};
    pcsx2_hook_backend backend{memory, patch_read, patch_write, nullptr, nullptr, patch_flush};
    {
        pcsx2::Patch first;
        assert(first.create_constant(backend, 0x1000, 0xabcdef01) == PCSX2_HOOK_OK);
        assert(first.enable() == PCSX2_HOOK_OK);
        pcsx2::Patch second(std::move(first));
        assert(!first && second && second.enabled());
        memory[1] ^= 1;
        assert(second.reset() == PCSX2_HOOK_CONFLICT && second.enabled());
        memory[1] ^= 1;
    }
    assert(memory[0] == 0x3c081234 && memory[1] == 0x35085678);
    psp_hook_backend psp_backend{memory, patch_read, patch_write, nullptr, nullptr, patch_flush};
    {
        psp::Patch first;
        assert(first.create_constant(psp_backend, 0x1000, 0x12348000) == PSP_HOOK_OK);
        assert(first.enable() == PSP_HOOK_OK);
        psp::Patch second(std::move(first));
        assert(!first && second.enabled());
    }
    assert(memory[0] == 0x3c081234 && memory[1] == 0x35085678);
    return 1;
}
extern "C" __declspec(dllexport) int cpp_api()
{
    pcsx2_hook_context ee{};
    psp_hook_context allegrex{};
    ee.gpr[5].word[1] = 0xaabbccdd;
    ee.gpr[5].word[2] = 0x11223344;
    ee.gpr[5].word[3] = 0x55667788;
    pcsx2::Registers ee_regs(ee);
    psp::Registers psp_regs(allegrex);
    ee_regs.a1 = 1;
    psp_regs.a1 = 2;
    assert(ee.gpr[5].word[0] == 1 && allegrex.gpr[5].word[0] == 2);
    assert(ee.gpr[5].word[1] == 0xaabbccdd && ee.gpr[5].word[2] == 0x11223344 &&
           ee.gpr[5].word[3] == 0x55667788);
    assert(&ee_regs.a4 == &ee_regs.t0 && &ee_regs.a7 == &ee_regs.t3);
    assert(&ee_regs.fp == &ee_regs.s8 && &psp_regs.fp == &psp_regs.s8);
    static_assert(std::is_const_v<std::remove_reference_t<decltype(ee_regs.zero)>>, "zero is read-only");
    ee_regs.f12 = 1.5f;
    psp_regs.f12 = -2.25f;
    assert(ee.fpr[12] == 0x3fc00000 && allegrex.fpr[12] == 0xc0100000);
    assert(static_cast<float>(ee_regs.f12) == 1.5f && static_cast<float>(psp_regs.f12) == -2.25f);
    ee_regs.f12 *= 2.0f;
    ee_regs.f12 += 1.0f;
    ee_regs.f12 -= 2.0f;
    ee_regs.f12 /= 2.0f;
    assert(static_cast<float>(ee_regs.f12) == 1.0f);
    ee_regs.f0.bits() = 0x7fa12345; // Copy raw NaN bits, without a float conversion.
    ee_regs.f31 = ee_regs.f0;
    assert(ee.fpr[31] == 0x7fa12345);
    assert(&ee_regs.raw() == &ee && &psp_regs.raw() == &allegrex);
    pcsx2::mid_callback<ee_named_callback>(&ee, &allegrex.gpr[5].word[0]);
    psp::mid_callback<psp_named_callback>(&allegrex, nullptr);
    assert(ee.gpr[5].word[0] == 3 && allegrex.gpr[5].word[0] == 4);
    pcsx2::Hook first;
    pcsx2::Hook second(std::move(first));
    psp::Hook psp_hook;
    return !first && !second && second.reset() == PCSX2_HOOK_OK && !psp_hook;
}

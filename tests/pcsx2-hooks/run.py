"""Compile both standalone APIs and execute emitted MIPS code on a small test CPU.

This CPU checks the generated instruction contracts, not PCSX2's JIT/backend.
Run on Windows with Visual Studio installed: python tests/pcsx2-hooks/run.py
"""
import ctypes as C
import os
from pathlib import Path
import random
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build' / 'pcsx2-hooks'
U32 = C.c_uint32
MASK64 = (1 << 64) - 1
MASK128 = (1 << 128) - 1


def signed(value, bits=64):
    return (value & ((1 << (bits - 1)) - 1)) - (value & (1 << (bits - 1)))


def instruction(op, rs=0, rt=0, immediate=0):
    return op << 26 | rs << 21 | rt << 16 | (immediate & 65535)


class CPU:
    def __init__(self, words, base, registers=None, bits=64):
        self.code = {base + i * 4: w for i, w in enumerate(words)}
        self.r = (registers or [0] * 32).copy()
        self.pc = base
        self.mem = {}
        self.hi = 0x123456789abcdef0123456789abcdef0
        self.lo = 0xfedcba9876543210fedcba9876543210
        self.sa = 13
        self.f = [0x12340000 + i for i in range(32)]
        self.fcr = 0x800001
        self.condition = False
        self.callback = None
        self.bits = bits
        self.acc, self.accflag = 0x7f800001, 1
        self.state_images = {}
        self.vf = [0x56780000 + i for i in range(128)]
        self.vc = [0x900 + i for i in range(16)]

    def read(self, address, size):
        return sum(self.mem.get(address + i, 0) << (8 * i) for i in range(size))

    def write(self, address, value, size):
        for i in range(size):
            self.mem[address + i] = (value >> (8 * i)) & 255

    def low(self, r, value):
        if r:
            self.r[r] = ((self.r[r] & (MASK128 ^ MASK64)) | (value & MASK64)) if self.bits == 64 else value & 0xffffffff

    def ordinary(self, w):
        op, rs, rt, rd, fn = w >> 26, w >> 21 & 31, w >> 16 & 31, w >> 11 & 31, w & 63
        imm = signed(w & 65535, 16)
        addr = ((self.r[rs] & MASK64) + imm) & 0xffffffff
        if w == 0:
            return
        if op == 9:
            self.low(rt, signed((self.r[rs] + imm) & 0xffffffff, 32))
        elif op == 25:
            self.low(rt, self.r[rs] + imm)
        elif op == 13:
            self.low(rt, (self.r[rs] & MASK64) | (w & 65535))
        elif op == 15:
            self.low(rt, signed((w & 65535) << 16, 32))
        elif op == 31:
            self.write(addr, self.r[rt], 16)
        elif op == 30:
            if rt:
                self.r[rt] = self.read(addr, 16)
        elif op == 63:
            self.write(addr, self.r[rt], 8)
        elif op == 43:
            self.write(addr, self.r[rt], 4)
        elif op == 35:
            self.low(rt, signed(self.read(addr, 4), 32))
        elif op == 57:
            self.write(addr, self.f[rt], 4)
        elif op == 49:
            self.f[rt] = self.read(addr, 4)
        elif op == 17 and rs == 2:
            self.low(rt, signed(self.fcr, 32))
        elif op == 17 and rs == 6:
            self.fcr = self.r[rt] & 0xffffffff
        elif op == 18 and rs == 3 and w & 128:
            self.low(rt, self.vc[w & 15])
        elif op == 18 and rs == 7 and w & 128:
            self.vc[w & 15] = self.r[rt] & 0xffffffff
        elif op in (54, 62):
            for i in range(4):
                if op == 62:
                    self.write(addr + i * 4, self.vf[rt * 4 + i], 4)
                else:
                    self.vf[rt * 4 + i] = self.read(addr + i * 4, 4)
        elif op == 28 and fn == 9 and (w >> 6 & 31) in (8, 9):
            self.r[rd] = self.hi if (w >> 6 & 31) == 8 else self.lo
        elif op == 28 and fn == 41 and (w >> 6 & 31) in (8, 9):
            if (w >> 6 & 31) == 8:
                self.hi = self.r[rs]
            else:
                self.lo = self.r[rs]
        elif op == 0 and fn == 40:
            self.low(rd, self.sa)
        elif op == 0 and fn == 41:
            self.sa = self.r[rs] & 0xffffffff
        elif op == 0 and fn in (16, 18):
            self.low(rd, self.hi if fn == 16 else self.lo)
        elif op == 0 and fn in (17, 19):
            if fn == 17:
                self.hi = self.r[rs]
            else:
                self.lo = self.r[rs]
        else:
            raise AssertionError(f'unhandled test opcode {w:08x}')

    def step(self):
        w = self.code[self.pc]
        op, rs, rt, fn = w >> 26, w >> 21 & 31, w >> 16 & 31, w & 63
        control = op in (1, 2, 3, 4, 5, 6, 7, 20, 21, 22, 23) or (op in (16, 17, 18) and rs == 8)
        control |= op == 0 and fn in (8, 9)
        if not control:
            self.ordinary(w)
            self.pc += 4
            return
        taken, likely = True, False
        if op in (2, 3):
            target = ((self.pc + 4) & 0xf0000000) | ((w & 0x03ffffff) << 2)
        elif op == 0:
            target = self.r[rs] & 0xffffffff
        else:
            target = (self.pc + 4 + signed(w & 65535, 16) * 4) & 0xffffffff
            if op in (4, 20):
                taken = (self.r[rs] & MASK64) == (self.r[rt] & MASK64)
            elif op in (5, 21):
                taken = (self.r[rs] & MASK64) != (self.r[rt] & MASK64)
            elif op in (6, 22):
                taken = signed(self.r[rs]) <= 0
            elif op in (7, 23):
                taken = signed(self.r[rs]) > 0
            elif op == 1:
                taken = signed(self.r[rs]) >= 0 if rt & 1 else signed(self.r[rs]) < 0
                likely = bool(rt & 2)
            else:
                condition = bool(self.vc[3] & (1 << (rt >> 2))) if op == 18 and self.bits == 32 else self.condition
                taken = condition if rt & 1 else not condition
                likely = bool(rt & 2)
            likely |= op in (20, 21, 22, 23)
        if op == 3 or (op == 0 and fn == 9):
            self.low(31 if op == 3 else (w >> 11 & 31), self.pc + 8)
        if taken or not likely:
            self.ordinary(self.code[self.pc + 4])
        next_pc = target if taken else self.pc + 8
        if taken and target == 0x02000060:
            address, flags, restore = (self.r[i] & 0xffffffff for i in (4, 5, 6))
            if restore:
                if flags & 1: self.acc, self.accflag = self.read(address, 4), self.read(address + 4, 4)
                if flags & 2: self.vf, self.vc = self.state_images[address]
            else:
                if flags & 1: self.write(address, self.acc, 4); self.write(address + 4, self.accflag, 4)
                if flags & 2: self.state_images[address] = self.vf.copy(), self.vc.copy()
            self.low(2, 1); self.low(3, 0xf4)
            next_pc = self.r[31] & 0xffffffff
        if taken and target == 0x03000000 and self.callback:
            self.callback(self)
            next_pc = self.r[31] & 0xffffffff
        self.pc = next_pc

    def run(self):
        for _ in range(1000):
            if self.pc not in self.code:
                return
            self.step()
        raise AssertionError('test CPU looped')


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    project = OUT / 'hooks.vcxproj'
    project.write_text(f'''<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
<ItemGroup Label="ProjectConfigurations"><ProjectConfiguration Include="Release|x64"><Configuration>Release</Configuration><Platform>x64</Platform></ProjectConfiguration></ItemGroup>
<PropertyGroup Label="Globals"><WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion></PropertyGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />
<PropertyGroup Label="Configuration"><ConfigurationType>DynamicLibrary</ConfigurationType><PlatformToolset>v145</PlatformToolset><UseDebugLibraries>false</UseDebugLibraries></PropertyGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />
<PropertyGroup><OutDir>{OUT}\\</OutDir><IntDir>{OUT}\\obj\\</IntDir></PropertyGroup>
<ItemDefinitionGroup><ClCompile><WarningLevel>Level4</WarningLevel><TreatWarningAsError>true</TreatWarningAsError><LanguageStandard>stdcpp17</LanguageStandard><LanguageStandard_C>stdc11</LanguageStandard_C><PreprocessorDefinitions>_CRT_SECURE_NO_WARNINGS</PreprocessorDefinitions><AdditionalIncludeDirectories>{ROOT / 'tests/pcsx2-hooks/psp-stubs'}</AdditionalIncludeDirectories></ClCompile></ItemDefinitionGroup>
<ItemGroup><ClCompile Include="{ROOT / 'tests/pcsx2-hooks/native.c'}"/><ClCompile Include="{ROOT / 'tests/pcsx2-hooks/cpp.cpp'}"/><ClCompile Include="{ROOT / 'tests/pcsx2-hooks/allocators.c'}"/><ClCompile Include="{ROOT / 'tests/pcsx2-hooks/psp_native.c'}"/><ClCompile Include="{ROOT / 'tests/pcsx2-hooks/pcsx2_patch.c'}"/><ClCompile Include="{ROOT / 'tests/pcsx2-hooks/psp_patch.c'}"/><ClCompile Include="{ROOT / 'tests/pcsx2-hooks/guest_contract.c'}"/><ClCompile Include="{ROOT / 'tests/pcsx2-hooks/psp_memory_bound.c'}"/><ClCompile Include="{ROOT / 'tests/pcsx2-hooks/pcsx2_pattern.c'}"/><ClCompile Include="{ROOT / 'tests/pcsx2-hooks/psp_pattern.c'}"/></ItemGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" /></Project>''')
    text = project.read_text()
    entries = ''.join(f'<ClCompile Include="{ROOT / "tests/pcsx2-hooks" / name}"/>'
                      for name in ('pcsx2_frontend.cpp', 'psp_frontend.cpp'))
    project.write_text(text.replace('</ItemGroup>\n<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets"',
                                    entries + '</ItemGroup>\n<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets"'))
    msbuild = Path(os.environ.get('ProgramFiles', 'C:/Program Files')) / 'Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe'
    with (OUT / 'native-build.log').open('w') as log:
        result = subprocess.run([str(msbuild), str(project), '/nologo', '/v:minimal', '/p:Configuration=Release', '/p:Platform=x64'], stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError((OUT / 'native-build.log').read_text())
    library = C.CDLL(str(OUT / 'hooks.dll'))
    library.relocate.argtypes = [C.POINTER(U32), C.c_uint, U32, C.POINTER(U32), C.c_uint, U32, C.POINTER(U32)]
    library.psp_relocate.argtypes = library.relocate.argtypes
    library.mid_code.argtypes = [C.POINTER(U32), C.c_uint, C.POINTER(U32), C.POINTER(U32)]
    library.pool_allocate.argtypes = [C.c_int, C.c_size_t]
    library.pool_allocate.restype = C.c_void_p
    library.pool_free.argtypes = [C.c_int, C.c_void_p]
    library.psp_mid_code.argtypes = [C.POINTER(U32), C.c_uint, C.POINTER(U32)]
    return library


def test_sdk_allocator():
    project = OUT / 'sdk-allocator.vcxproj'
    project.write_text(f'''<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
<ItemGroup Label="ProjectConfigurations"><ProjectConfiguration Include="Release|Win32"><Configuration>Release</Configuration><Platform>Win32</Platform></ProjectConfiguration></ItemGroup>
<PropertyGroup Label="Globals"><WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion></PropertyGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />
<PropertyGroup Label="Configuration"><ConfigurationType>Application</ConfigurationType><PlatformToolset>v145</PlatformToolset><UseDebugLibraries>false</UseDebugLibraries></PropertyGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />
<PropertyGroup><OutDir>{OUT}\\</OutDir><IntDir>{OUT}\\sdk-obj\\</IntDir></PropertyGroup>
<ItemDefinitionGroup><ClCompile><WarningLevel>Level4</WarningLevel><TreatWarningAsError>true</TreatWarningAsError><LanguageStandard_C>stdc11</LanguageStandard_C><PreprocessorDefinitions>_CRT_SECURE_NO_WARNINGS</PreprocessorDefinitions></ClCompile></ItemDefinitionGroup>
<ItemGroup><ClCompile Include="{ROOT / 'tests/pcsx2-hooks/sdk_allocator.c'}"/></ItemGroup>
<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" /></Project>''')
    msbuild = Path(os.environ.get('ProgramFiles', 'C:/Program Files')) / 'Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe'
    with (OUT / 'sdk-build.log').open('w') as log:
        result = subprocess.run([str(msbuild), str(project), '/nologo', '/v:minimal', '/p:Configuration=Release', '/p:Platform=Win32'], stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError((OUT / 'sdk-build.log').read_text())
    subprocess.run([str(OUT / 'sdk-allocator.exe')], check=True)


class Hooks(unittest.TestCase):
    def test_pc_style_frontends_preserve_calls_and_rollback_failures(self):
        self.assertEqual(self.lib.pcsx2_frontend_api(), 1)
        self.assertEqual(self.lib.psp_frontend_api(), 1)

    @classmethod
    def setUpClass(cls):
        cls.lib = build()

    def relocate(self, words, base=0x1000, new_base=0x02000000, capacity=384):
        source = (U32 * len(words))(*words)
        output = (U32 * 384)(*([0xcdcdcdcd] * 384))
        written = U32()
        status = self.lib.relocate(source, len(words), base, output, capacity, new_base, C.byref(written))
        return status, list(output)[:written.value], list(output)

    def test_lifecycle_and_cpp(self):
        self.assertEqual(self.lib.lifecycle(), 1)
        self.assertEqual(self.lib.cpp_api(), 1)

    def test_patch_handles_both_platforms_and_cpp_ownership(self):
        self.assertEqual(self.lib.pcsx2_patch_lifecycle(), 1)
        self.assertEqual(self.lib.psp_patch_lifecycle(), 1)
        self.assertEqual(self.lib.cpp_patch_api(), 1)

    def test_guest_service_contract_and_psp_jit_markers(self):
        self.assertEqual(self.lib.guest_contract(), 1)
        self.assertEqual(self.lib.psp_memory_bound(), 1)

    def test_patterns_are_bounded_and_preserve_wildcards(self):
        self.assertEqual(self.lib.pcsx2_pattern_cases(), 1)
        self.assertEqual(self.lib.psp_pattern_cases(), 1)

    def test_private_sdk_allocator_is_bounded(self):
        test_sdk_allocator()

    def test_psp_mid_preserves_vfpu_prefixes_registers_and_random_state(self):
        for flags in range(8):
            output, written = (U32 * 384)(), U32()
            self.assertEqual(self.lib.psp_mid_code(output, flags, C.byref(written)), 0)
            registers = [0x12340000 + r for r in range(32)]
            registers[0] = 0; registers[29] = 0x00800000
            cpu = CPU(list(output)[:written.value], 0x02000000, registers, bits=32)
            cpu.hi = 0x1234; cpu.lo = 0x5678
            expected = cpu.hi, cpu.lo, cpu.fcr, cpu.f.copy(), cpu.vf.copy(), cpu.vc.copy()
            def callback(machine):
                context = 0x00800000 - 880 + 32
                if flags & 4:
                    self.assertEqual(machine.r[4:12], registers[4:12])
                else:
                    self.assertEqual(machine.r[4], context)
                    self.assertEqual(machine.r[5], 0x12345678)
                self.assertEqual(machine.r[28], 0x024abc00)
                if flags & 2:
                    self.assertEqual(machine.vc[:3], [0xe4, 0xe4, 0])
                self.assertEqual(machine.read(context + 29 * 4, 4), registers[29])
                for r in range(1, 32):
                    if r not in (29, 31):
                        machine.r[r] = 0xffffffff - r
                machine.hi, machine.lo = 1, 2
                if flags & 1:
                    machine.fcr = 0; machine.f = [0] * 32
                if flags & 2:
                    machine.vf = [0] * 128
                    for r in list(range(4)) + list(range(8, 16)):
                        machine.vc[r] = 0
            cpu.callback = callback; cpu.run()
            self.assertEqual(cpu.r, registers)
            self.assertEqual((cpu.hi, cpu.lo, cpu.fcr, cpu.f, cpu.vf, cpu.vc), expected)
            self.assertEqual(cpu.pc, 0x1018)
            self.assertLess(written.value * 4, 1120)

    def test_both_custom_pools_alignment_exhaustion_and_fragmentation(self):
        for platform in (0, 1):
            self.lib.pool_reset(platform)
            self.assertIsNone(self.lib.pool_allocate(platform, 0))
            self.assertIsNone(self.lib.pool_allocate(platform, C.c_size_t(-1).value))
            randomizer = random.Random(5900)
            live = {}
            for iteration in range(2000):
                for pointer, (size, pattern) in live.items():
                    self.assertEqual(C.string_at(pointer, size), bytes([pattern]) * size)
                if live and randomizer.random() < 0.48:
                    pointer = randomizer.choice(list(live))
                    self.lib.pool_free(platform, pointer + 1)  # Interior pointers cannot corrupt metadata.
                    self.lib.pool_free(platform, pointer)
                    self.lib.pool_free(platform, pointer)  # Double frees are ignored.
                    del live[pointer]
                else:
                    size = randomizer.randint(1, 511)
                    pointer = self.lib.pool_allocate(platform, size)
                    if pointer:
                        self.assertEqual(pointer % 16, 0)
                        for other, (other_size, _) in live.items():
                            self.assertTrue(pointer + size <= other or other + other_size <= pointer)
                        pattern = iteration % 255 + 1
                        C.memset(pointer, pattern, size)
                        live[pointer] = size, pattern
                if iteration % 100 == 0:
                    self.lib.pool_initialize(platform)  # Initialization cannot discard live allocations.
            for pointer in live:
                self.lib.pool_free(platform, pointer)
            # Host metadata is 32 bytes after rounding; EE/PSP metadata is 16.
            pointer = self.lib.pool_allocate(platform, 4064)
            self.assertIsNotNone(pointer)
            self.assertIsNone(self.lib.pool_allocate(platform, 16))
            self.lib.pool_free(platform, pointer)
            self.assertIsNotNone(self.lib.pool_allocate(platform, 4064))

    def test_relocation_errors_do_not_write_output(self):
        cases = [([0x0c000800, 0], 4), ([0x0320f809, 0], 4), ([0x04110001, 0], 4),
                 ([0x10000001], 3), ([0x10000001, 0x08000400], 3), ([0x0000000c, 0], 4)]
        for words, error in cases:
            status, _, output = self.relocate(words)
            self.assertEqual(status, error)
            self.assertEqual(output, [0xcdcdcdcd] * 384)
        self.assertEqual(self.relocate([0, 0], capacity=3)[0], 6)
        self.assertEqual(self.relocate([0, 0], new_base=0x20000000)[0], 5)
        self.assertEqual(self.relocate([0, 0], base=0x1001)[0], 2)

    def test_conditional_branches_and_annulled_delays(self):
        randomizer = random.Random(5900)
        branch_words = [instruction(op, 4, 5 if op in (4, 5, 20, 21) else 0, 16)
                        for op in (4, 5, 6, 7, 20, 21, 22, 23)]
        branch_words += [instruction(1, 4, rt, 16) for rt in range(4)]
        branch_words += [instruction(op, 8, rt, 16) for op in (16, 17, 18) for rt in range(4)]
        for word in branch_words:
            for nearby in (False, True):
                new_base = 0x1100 if nearby else 0x02000000
                source = [word, instruction(9, 6, 6, 3), instruction(9, 7, 7, 1)]
                status, relocated, _ = self.relocate(source, new_base=new_base)
                self.assertEqual(status, 0)
                for _ in range(20):
                    registers = [randomizer.getrandbits(128) for _ in range(32)]
                    registers[0] = 0
                    registers[4] = randomizer.choice([0, 1, MASK64])
                    registers[5] = randomizer.choice([0, 1, MASK64])
                    original = CPU(source, 0x1000, registers)
                    moved = CPU(relocated, new_base, registers)
                    original.condition = moved.condition = bool(randomizer.getrandbits(1))
                    original.run(); moved.run()
                    self.assertEqual(original.pc, moved.pc)
                    self.assertEqual(original.r, moved.r)

    def test_psp_vfpu_condition_indices_and_likely_delays(self):
        for index in range(8):
            for mode in range(4):
                for condition in (0, 1):
                    for new_base in (0x1100, 0x02000000):
                        words = [instruction(18, 8, index * 4 + mode, 16), instruction(9, 6, 6, 3), 0]
                        source = (U32 * len(words))(*words)
                        output, written = (U32 * 384)(), U32()
                        self.assertEqual(self.lib.psp_relocate(source, len(words), 0x1000,
                            output, 384, new_base, C.byref(written)), 0)
                        original = CPU(words, 0x1000, bits=32)
                        moved = CPU(list(output)[:written.value], new_base, bits=32)
                        original.vc[3] = moved.vc[3] = condition << index
                        original.run(); moved.run()
                        self.assertEqual(original.pc, moved.pc)
                        self.assertEqual(original.r, moved.r)

    def test_internal_targets_and_absolute_jump(self):
        sources = [
            [instruction(4, 0, 0, 2), instruction(9, 6, 6, 1), instruction(9, 6, 6, 50), instruction(9, 7, 7, 2)],
            [instruction(4, 0, 0, 0), instruction(9, 6, 6, 1), 0],
            [0x08000402, instruction(9, 6, 6, 1), instruction(9, 7, 7, 2)],
            [0x08001000, instruction(9, 6, 6, 1)],
            [0x03e00008, instruction(9, 6, 6, 1)],
        ]
        for source in sources:
            status, moved, _ = self.relocate(source)
            self.assertEqual(status, 0)
            registers = [0] * 32; registers[31] = 0x5000
            first = CPU(source, 0x1000, registers); second = CPU(moved, 0x02000000, registers)
            first.run(); second.run()
            self.assertEqual(first.pc, second.pc)
            self.assertEqual(first.r, second.r)

    def test_mid_preserves_128_bit_state_and_applies_context_edits(self):
        for flags in range(16):
            for edit in (False, True):
                output, written, trampoline = (U32 * 384)(), U32(), U32()
                self.assertEqual(self.lib.mid_code(output, flags, C.byref(written), C.byref(trampoline)), 0)
                words = list(output)[:written.value]
                registers = [random.Random(5900 + r).getrandbits(128) for r in range(32)]
                registers[0] = 0
                registers[29] = (registers[29] & (MASK128 ^ MASK64)) | 0x00800000
                cpu = CPU(words, 0x02000000, registers)
                special = (cpu.hi, cpu.lo, cpu.sa, cpu.fcr, cpu.f.copy(), cpu.acc, cpu.accflag, cpu.vf.copy(), cpu.vc.copy())
                calls = []
                def callback(machine):
                    calls.append(1)
                    context = 0x00800000 - (2800 if flags & 8 else 752) + 64
                    if flags & 2:
                        self.assertEqual(machine.r[4:12], registers[4:12])
                    else:
                        self.assertEqual(machine.r[4] & 0xffffffff, context)
                        self.assertEqual(machine.r[5] & MASK64, 0x12345678)
                    self.assertEqual(machine.r[28] & MASK64, 0x024abc00)
                    self.assertEqual(machine.read(context + 29 * 16, 16), registers[29])
                    if edit:
                        machine.write(context + 4 * 16, 0xabc123, 4)
                    for r in range(1, 32):
                        if r not in (29, 31):
                            machine.r[r] = MASK128 - r
                    machine.hi, machine.lo, machine.sa = 1, 2, 3
                    if flags & 4:
                        self.assertEqual(machine.read(context + 680, 4), special[5])
                        machine.acc = 0x80000000; machine.accflag = 0
                    if flags & 8: machine.vf = [0] * 128; machine.vc = [0] * 16
                    if flags & 1:
                        machine.fcr = 0x123; machine.f = [0] * 32
                cpu.callback = callback
                cpu.run()
                expected = registers.copy()
                if edit:
                    expected[4] = (expected[4] & ~0xffffffff) | 0xabc123
                self.assertEqual(cpu.r, expected)
                self.assertEqual((cpu.hi, cpu.lo, cpu.sa, cpu.fcr, cpu.f, cpu.acc, cpu.accflag, cpu.vf, cpu.vc), special)
                self.assertEqual(cpu.pc, 0x1018)
                self.assertEqual(len(calls), 1)
                self.assertLess(written.value * 4, (650 if flags & 1 else 400) + (120 if flags & 12 else 0))


if __name__ == '__main__':
    unittest.main()

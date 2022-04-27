#include "mips.h"

uint16_t swap16(uint16_t value)
{
    return (value << 8) | (value >> 8);
}

uint32_t swap32(uint32_t value)
{
    value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
    return (value << 16) | (value >> 16);
}

uint64_t swap64(uint64_t value)
{
    value = ((value << 8) & 0xFF00FF00FF00FF00ULL) | ((value >> 8) & 0x00FF00FF00FF00FFULL);
    value = ((value << 16) & 0xFFFF0000FFFF0000ULL) | ((value >> 16) & 0x0000FFFF0000FFFFULL);
    return (value << 32) | (value >> 32);
}

//Instruction set
uint32_t write_directly(uint32_t data)
{
    return (swap32(data));
}
uint32_t nop()
{
    return (0x00000000);
}
uint32_t bc0f(int32_t offset)
{
    return (0x4100FF00 | (offset & 0xffff));
}
uint32_t di()
{
    return (0x42000039);
}
uint32_t div1(RegisterID rs, RegisterID rt)
{
    return (0x7000001a | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t ei()
{
    return (0x42000038);
}
uint32_t eret()
{
    return (0x42000018);
}
uint32_t madd(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000000 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t maddu(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000001 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t mfhi1(RegisterID rd)
{
    return (0x70000010 | (rd << OP_SH_RD));
}
uint32_t mflo1(RegisterID rd)
{
    return (0x70000012 | (rd << OP_SH_RD));
}
uint32_t mfsa(RegisterID rd)
{
    return (0x00000028 | (rd << OP_SH_RD));
}
uint32_t mthi1(RegisterID rs)
{
    return (0x70000011 | (rs << OP_SH_RS));
}
uint32_t mtlo1(RegisterID rs)
{
    return (0x70000013 | (rs << OP_SH_RS));
}
uint32_t mtsa(RegisterID rs)
{
    return (0x00000029 | (rs << OP_SH_RS));
}
uint32_t mtsab(RegisterID rs, int32_t imm)
{
    return (0x04180000 | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
}
uint32_t mult(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x00000018 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t mult1(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000018 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t multu(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x00000019 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t multu1(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000019 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t _sync()
{
    return (0x0000000f);
}
uint32_t syscall()
{
    return (0x0000000c);
}
uint32_t tge(RegisterID rs, RegisterID rt)
{
    return (0x0000bb30 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t tlbwi()
{
    return (0x42000002);
}
uint32_t abss(FPRegisterID fd, FPRegisterID fs)
{
    return (0x46000005 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
}
uint32_t adds(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
{
    return (0x46000000 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t addas(FPRegisterID fs, FPRegisterID ft)
{
    return (0x46000018 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t bc1f(int32_t offset)
{
    return (0x45000000 | (offset & 0xffff));
}
uint32_t bc1fl(int32_t offset)
{
    return (0x45020000 | (offset & 0xffff));
}
uint32_t bc1tl(int32_t offset)
{
    return (0x45030000 | (offset & 0xffff));
}
uint32_t ceqs(FPRegisterID fs, FPRegisterID ft)
{
    return (0x46000032 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t cvtsw(FPRegisterID fd, FPRegisterID fs)
{
    return (0x46800020 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
}
uint32_t cvtws(FPRegisterID fd, FPRegisterID fs)
{
    return (0x46000024 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
}
uint32_t divs(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
{
    return (0x46000003 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t madds(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
{
    return (0x4600001c | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t maddas(FPRegisterID fs, FPRegisterID ft)
{
    return (0x4600001e | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t maxs(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
{
    return (0x46000028 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t mfc1(RegisterID rt, FPRegisterID fs)
{
    return (0x44000000 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
}
uint32_t mins(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
{
    return (0x46000029 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t movs(FPRegisterID fd, FPRegisterID fs)
{
    return (0x46000006 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
}
uint32_t msubs(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
{
    return (0x4600001d | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t msubas(FPRegisterID fs, FPRegisterID ft)
{
    return (0x4600001f | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t mtc1(RegisterID rt, FPRegisterID fs)
{
    return (0x44800000 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
}
uint32_t muls(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
{
    return (0x46000002 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t mulas(FPRegisterID fs, FPRegisterID ft)
{
    return (0x4600001a | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t negs(FPRegisterID fd, FPRegisterID fs)
{
    return (0x46000007 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
}
uint32_t rsqrts(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
{
    return (0x46000016 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t sqrts(FPRegisterID fd, FPRegisterID ft)
{
    return (0x46000004 | (fd << OP_SH_FD) | (ft << OP_SH_FT));
}
uint32_t subs(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
{
    return (0x46000001 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t paddh(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000108 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t paddsw(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000408 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t paddw(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000008 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pand(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000489 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pcgth(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000188 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pcpyh(RegisterID rd, RegisterID rt)
{
    return (0x700006e9 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
}
uint32_t pcpyld(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000389 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pcpyud(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700003a9 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pexch(RegisterID rd, RegisterID rt)
{
    return (0x700006a9 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
}
uint32_t pexew(RegisterID rd, RegisterID rt)
{
    return (0x70000789 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
}
uint32_t pextlb(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000688 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pextlh(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000588 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pextlw(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000488 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pextub(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700006a8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pextuw(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700004a8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pmaxh(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700001c8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pmaxw(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700000c8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pmfhi(RegisterID rd)
{
    return (0x70000209 | (rd << OP_SH_RD));
}
uint32_t pmflo(RegisterID rd)
{
    return (0x70000249 | (rd << OP_SH_RD));
}
uint32_t pminh(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700001e8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pminw(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700000e8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pmulth(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000709 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pnor(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700004e9 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t por(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700004a9 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t ppacb(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700006c8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t ppach(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700005c8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t prot3w(RegisterID rd, RegisterID rt)
{
    return (0x700007c9 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
}
uint32_t psllh(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x70000034 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t psllw(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x7000003c | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t psrah(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x70000037 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t psraw(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x7000003f | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t psrlh(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x70000036 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t psrlw(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x7000003e | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t psubb(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000248 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t psubw(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x70000048 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t pxor(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700004c9 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t qfsrv(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x700006e8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t cles(FPRegisterID fs, FPRegisterID ft)
{
    return (0x46000036 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t clts(FPRegisterID fs, FPRegisterID ft)
{
    return (0x46000034 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
}
uint32_t add(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x00000020 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t addu(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x00000021 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t _and(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x00000024 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t daddu(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x0000002d | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t _div(RegisterID rs, RegisterID rt)
{
    return (0x0000001a | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t divu(RegisterID rs, RegisterID rt)
{
    return (0x0000001b | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t dsll(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x00000038 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t dsll32(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x0000003c | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t dsllv(RegisterID rd, RegisterID rt, RegisterID rs)
{
    return (0x00000014 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
}
uint32_t dsra(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x0000003b | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t dsra32(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x0000003f | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t dsrl(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x0000003a | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t dsrl32(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x0000003e | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t dsrlv(RegisterID rd, RegisterID rt, RegisterID rs)
{
    return (0x00000016 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
}
uint32_t dsubu(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x0000002f | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t jalr(RegisterID rs)
{
    return (0x0000f809 | (rs << OP_SH_RS));
}
uint32_t jr(RegisterID rs)
{
    return (0x00000008 | (rs << OP_SH_RS));
}
uint32_t mfhi(RegisterID rd)
{
    return (0x00000010 | (rd << OP_SH_RD));
}
uint32_t mflo(RegisterID rd)
{
    return (0x00000012 | (rd << OP_SH_RD));
}
uint32_t movn(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x0000000b | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t movz(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x0000000a | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t mthi(RegisterID rs)
{
    return (0x00000011 | (rs << OP_SH_RS));
}
uint32_t mtlo(RegisterID rs)
{
    return (0x00000013 | (rs << OP_SH_RS));
}
uint32_t nor(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x00000027 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t _or(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x00000025 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t sll(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0000000000 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t sllv(RegisterID rd, RegisterID rt, RegisterID rs)
{
    return (0x00000004 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
}
uint32_t slt(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x0000002a | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t sltu(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x0000002b | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t sra(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x00000003 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t srav(RegisterID rd, RegisterID rt, RegisterID rs)
{
    return (0x00000007 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
}
uint32_t srl(RegisterID rd, RegisterID rt, int32_t shamt)
{
    return (0x00000002 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
}
uint32_t srlv(RegisterID rd, RegisterID rt, RegisterID rs)
{
    return (0x00000006 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
}
uint32_t sub(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x00000022 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t subu(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x00000023 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t _xor(RegisterID rd, RegisterID rs, RegisterID rt)
{
    return (0x00000026 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
}
uint32_t addi(RegisterID rt, RegisterID rs, int32_t imm)
{
    return (0x20000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
}
uint32_t addiu(RegisterID rt, RegisterID rs, int32_t imm)
{
    return (0x24000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
}
uint32_t andi(RegisterID rt, RegisterID rs, int32_t imm)
{
    return (0x30000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
}
uint32_t daddi(RegisterID rt, RegisterID rs, int32_t imm)
{
    return (0x60000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
}
uint32_t daddiu(RegisterID rt, RegisterID rs, int32_t imm)
{
    return (0x64000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
}
uint32_t lb(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0x80000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t lbu(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0x90000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t ld(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0xDC000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t ldl(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0x68000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t ldr(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0x6C000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t lh(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0x84000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t lhu(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0x94000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t lui(RegisterID rt, int32_t imm)
{
    return (0x3c000000 | (rt << OP_SH_RT) | (*(int32_t*)&imm & 0xffff));
}
uint32_t lw(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0x8c000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t lwl(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0x88000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t lwr(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0x98000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t lwu(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0x9C000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t ori(RegisterID rt, RegisterID rs, int32_t imm)
{
    return (0x34000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
}
uint32_t sq(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0x7C000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t slti(RegisterID rt, RegisterID rs, int32_t imm)
{
    return (0x28000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
}
uint32_t sltiu(RegisterID rt, RegisterID rs, int32_t imm)
{
    return (0x2c000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
}
uint32_t sb(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0xa0000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t sh(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0xa4000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t sw(RegisterID rt, RegisterID rs, int32_t offset)
{
    return (0xac000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t lwc1(FPRegisterID ft, RegisterID rs, int32_t offset)
{
    return (0xc4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t swc1(FPRegisterID ft, RegisterID rs, int32_t offset)
{
    return (0xe4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS) | (offset & 0xffff));
}
uint32_t xori(RegisterID rt, RegisterID rs, int32_t imm)
{
    return (0x38000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
}
uint32_t li(RegisterID dest, int32_t imm)
{
    return (0x34000000 | (dest << OP_SH_RT) | (*(int32_t*)&imm & 0xffff));
}
uint32_t j(int32_t address)
{
    return ((0x08000000 | ((address & 0x0fffffff) >> 2)));
}
uint32_t jal(int32_t address)
{
    return ((0x0C000000 | ((address & 0x0fffffff) >> 2)));
}
uint32_t b(int32_t imm)
{
    return (0x10000000 | (imm & 0xffff));
}
uint32_t beq(RegisterID rs, RegisterID rt, int32_t imm)
{
    return (0x10000000 | (rs << OP_SH_RS) | (rt << OP_SH_RT) | (imm & 0xffff));
}
uint32_t bne(RegisterID rs, RegisterID rt, int32_t imm)
{
    return (0x14000000 | (rs << OP_SH_RS) | (rt << OP_SH_RT) | (imm & 0xffff));
}
uint32_t move(RegisterID rd, RegisterID rs)
{
    /* addu */
    return (0x00000021 | (rd << OP_SH_RD) | (rs << OP_SH_RS));
}
uint32_t moveq(RegisterID rd, RegisterID rs)
{
    return (0x700004a9 | (rd << OP_SH_RD) | (rs << OP_SH_RS));
}
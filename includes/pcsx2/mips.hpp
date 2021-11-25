#include <cstdint>
#include <ostream>

namespace mips
{
    namespace
    {
        template<typename T>
        inline uint8_t get_prefix(T& r)
        {
            if (r.value < 8)
                return r.value;

            r.value -= 8;
            return 1;
        }

        template<typename T>
        inline void write_binary(std::ostream& os, T value, std::streamsize size = 4)
        {
#if BIGENDIAN
            value = swap32(value);
#endif
            os.write(reinterpret_cast<const char*>(&value), size);
        }

        inline uint16_t swap16(uint16_t value)
        {
            return (value << 8) | (value >> 8);
        }

        inline uint32_t swap32(uint32_t value)
        {
            value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
            return (value << 16) | (value >> 16);
        }

        inline uint64_t swap64(uint64_t value)
        {
            value = ((value << 8) & 0xFF00FF00FF00FF00ULL) | ((value >> 8) & 0x00FF00FF00FF00FFULL);
            value = ((value << 16) & 0xFFFF0000FFFF0000ULL) | ((value >> 16) & 0x0000FFFF0000FFFFULL);
            return (value << 32) | (value >> 32);
        }
    }

    typedef enum : int8_t {
        r0 = 0,
        r1,
        r2,
        r3,
        r4,
        r5,
        r6,
        r7,
        r8,
        r9,
        r10,
        r11,
        r12,
        r13,
        r14,
        r15,
        r16,
        r17,
        r18,
        r19,
        r20,
        r21,
        r22,
        r23,
        r24,
        r25,
        r26,
        r27,
        r28,
        r29,
        r30,
        r31,
        zero = r0,
        at = r1,
        v0 = r2,
        v1 = r3,
        a0 = r4,
        a1 = r5,
        a2 = r6,
        a3 = r7,
        t0 = r8,
        t1 = r9,
        t2 = r10,
        t3 = r11,
        t4 = r12,
        t5 = r13,
        t6 = r14,
        t7 = r15,
        s0 = r16,
        s1 = r17,
        s2 = r18,
        s3 = r19,
        s4 = r20,
        s5 = r21,
        s6 = r22,
        s7 = r23,
        t8 = r24,
        t9 = r25,
        k0 = r26,
        k1 = r27,
        gp = r28,
        sp = r29,
        fp = r30,
        ra = r31,
        InvalidGPRReg = -1,
    } RegisterID;

    typedef enum : int8_t {
        fir = 0,
        fccr = 25,
        fexr = 26,
        fenr = 28,
        fcsr = 31,
        pc
    } SPRegisterID;

    typedef enum : int8_t {
        f0,
        f1,
        f2,
        f3,
        f4,
        f5,
        f6,
        f7,
        f8,
        f9,
        f10,
        f11,
        f12,
        f13,
        f14,
        f15,
        f16,
        f17,
        f18,
        f19,
        f20,
        f21,
        f22,
        f23,
        f24,
        f25,
        f26,
        f27,
        f28,
        f29,
        f30,
        f31,
        InvalidFPRReg = -1,
    } FPRegisterID;

    enum {
        OP_SH_RD = 11,
        OP_SH_RT = 16,
        OP_SH_RS = 21,
        OP_SH_SHAMT = 6,
        OP_SH_CODE = 16,
        OP_SH_FD = 6,
        OP_SH_FS = 11,
        OP_SH_FT = 16,
        OP_SH_MSB = 11,
        OP_SH_LSB = 6
    };

    enum {
        FP_CAUSE_INVALID_OPERATION = 1 << 16 // FCSR Bits
    };

    //Instruction set

    inline std::ostream& write_directly(std::ostream& os, uint32_t data)
    {
        write_binary(os, swap32(data));
        return os;
    }

    inline std::ostream& nop(std::ostream& os)
    {
        write_binary(os, 0x00000000);
        return os;
    }

    inline std::ostream& sync(std::ostream& os)
    {
        write_binary(os, 0x0000000f);
        return os;
    }

    inline std::ostream& loadDelayNop(std::ostream& os)
    {
        nop(os);
        return os;
    }

    inline std::ostream& copDelayNop(std::ostream& os)
    {
        nop(os);
        return os;
    }

    inline std::ostream& move(std::ostream& os, RegisterID rd, RegisterID rs)
    {
        /* addu */
        write_binary(os, 0x00000021 | (rd << OP_SH_RD) | (rs << OP_SH_RS));
        return os;
    }

    inline std::ostream& ext(std::ostream& os, RegisterID rt, RegisterID rs, int pos, int size)
    {
        int msb = size - 1;
        write_binary(os, 0x7c000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (pos << OP_SH_LSB) | (msb << OP_SH_MSB));
        return os;
    }

    inline std::ostream& mfhc1(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        write_binary(os, 0x4460000 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
        return os;
    }

    template<typename T>
    inline std::ostream& lui(std::ostream& os, RegisterID rt, T imm)
    {
        write_binary(os, 0x3c000000 | (rt << OP_SH_RT) | (*(int32_t*)&imm & 0xffff));
        return os;
    }

    inline std::ostream& clz(std::ostream& os, RegisterID rd, RegisterID rs)
    {
        write_binary(os, 0x70000020 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rd << OP_SH_RT));
        return os;
    }

    template<typename T>
    inline std::ostream& addiu(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        write_binary(os, 0x24000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
        return os;
    }

    inline std::ostream& addu(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        write_binary(os, 0x00000021 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
        return os;
    }

    inline std::ostream& subu(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        write_binary(os, 0x00000023 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
        return os;
    }

    inline std::ostream& mult(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        write_binary(os, 0x00000018 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
        return os;
    }

    inline std::ostream& div(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        write_binary(os, 0x0000001a | (rs << OP_SH_RS) | (rt << OP_SH_RT));
        return os;
    }

    inline std::ostream& mfhi(std::ostream& os, RegisterID rd)
    {
        write_binary(os, 0x00000010 | (rd << OP_SH_RD));
        return os;
    }

    inline std::ostream& mflo(std::ostream& os, RegisterID rd)
    {
        write_binary(os, 0x00000012 | (rd << OP_SH_RD));
        return os;
    }

    inline std::ostream& mul(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        mult(os, rs, rt);
        mflo(os, rd);
        return os;
    }

    inline std::ostream& andInsn(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        write_binary(os, 0x00000024 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
        return os;
    }

    template<typename T>
    inline std::ostream& andi(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        write_binary(os, 0x30000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
        return os;
    }

    inline std::ostream& nor(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        write_binary(os, 0x00000027 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
        return os;
    }

    inline std::ostream& orInsn(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        write_binary(os, 0x00000025 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
        return os;
    }

    template<typename T>
    inline std::ostream& ori(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        write_binary(os, 0x34000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
        return os;
    }

    inline std::ostream& xorInsn(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        write_binary(os, 0x00000026 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
        return os;
    }

    template<typename T>
    inline std::ostream& xori(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        write_binary(os, 0x38000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
        return os;
    }

    inline std::ostream& slt(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        write_binary(os, 0x0000002a | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
        return os;
    }

    inline std::ostream& sltu(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        write_binary(os, 0x0000002b | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
        return os;
    }

    template<typename T>
    inline std::ostream& slti(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        write_binary(os, 0x28000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
        return os;
    }

    template<typename T>
    inline std::ostream& sltiu(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        write_binary(os, 0x2c000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
        return os;
    }

    inline std::ostream& sll(std::ostream& os, RegisterID rd, RegisterID rt, int shamt)
    {
        write_binary(os, 0x00000000 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
        return os;
    }

    inline std::ostream& sllv(std::ostream& os, RegisterID rd, RegisterID rt, RegisterID rs)
    {
        write_binary(os, 0x00000004 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
        return os;
    }

    inline std::ostream& sra(std::ostream& os, RegisterID rd, RegisterID rt, int shamt)
    {
        write_binary(os, 0x00000003 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
        return os;
    }

    inline std::ostream& srav(std::ostream& os, RegisterID rd, RegisterID rt, RegisterID rs)
    {
        write_binary(os, 0x00000007 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
        return os;
    }

    inline std::ostream& srl(std::ostream& os, RegisterID rd, RegisterID rt, int shamt)
    {
        write_binary(os, 0x00000002 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
        return os;
    }

    inline std::ostream& srlv(std::ostream& os, RegisterID rd, RegisterID rt, RegisterID rs)
    {
        write_binary(os, 0x00000006 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
        return os;
    }

    inline std::ostream& lb(std::ostream& os, RegisterID rt, RegisterID rs, int offset)
    {
        write_binary(os, 0x80000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
        loadDelayNop(os);
        return os;
    }

    inline std::ostream& lbu(std::ostream& os, RegisterID rt, RegisterID rs, int offset)
    {
        write_binary(os, 0x90000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
        loadDelayNop(os);
        return os;
    }

    inline std::ostream& lw(std::ostream& os, RegisterID rt, RegisterID rs, int offset)
    {
        write_binary(os, 0x8c000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
        loadDelayNop(os);
        return os;
    }

    inline std::ostream& lwl(std::ostream& os, RegisterID rt, RegisterID rs, int offset)
    {
        write_binary(os, 0x88000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
        loadDelayNop(os);
        return os;
    }

    inline std::ostream& lwr(std::ostream& os, RegisterID rt, RegisterID rs, int offset)
    {
        write_binary(os, 0x98000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
        loadDelayNop(os);
        return os;
    }

    inline std::ostream& lh(std::ostream& os, RegisterID rt, RegisterID rs, int offset)
    {
        write_binary(os, 0x84000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
        loadDelayNop(os);
        return os;
    }

    inline std::ostream& lhu(std::ostream& os, RegisterID rt, RegisterID rs, int offset)
    {
        write_binary(os, 0x94000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
        loadDelayNop(os);
        return os;
    }

    inline std::ostream& sb(std::ostream& os, RegisterID rt, RegisterID rs, int offset)
    {
        write_binary(os, 0xa0000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
        return os;
    }

    inline std::ostream& sh(std::ostream& os, RegisterID rt, RegisterID rs, int offset)
    {
        write_binary(os, 0xa4000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
        return os;
    }

    inline std::ostream& sw(std::ostream& os, RegisterID rt, RegisterID rs, int offset)
    {
        write_binary(os, 0xac000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
        return os;
    }

    inline std::ostream& jr(std::ostream& os, RegisterID rs)
    {
        write_binary(os, 0x00000008 | (rs << OP_SH_RS));
        return os;
    }

    inline std::ostream& jalr(std::ostream& os, RegisterID rs)
    {
        write_binary(os, 0x0000f809 | (rs << OP_SH_RS));
        return os;
    }

    inline std::ostream& jal(std::ostream& os)
    {
        write_binary(os, 0x0c000000);
        return os;
    }

    inline std::ostream& b(std::ostream& os, int imm)
    {
        write_binary(os, 0x10000000 | (imm & 0xffff));
        return os;
    }

    inline std::ostream& bkpt(std::ostream& os)
    {
        int value = 512; // BRK_BUG
        write_binary(os, 0x0000000d | ((value & 0x3ff) << OP_SH_CODE));
        return os;
    }

    inline std::ostream& bgez(std::ostream& os, RegisterID rs, int imm)
    {
        write_binary(os, 0x04010000 | (rs << OP_SH_RS) | (imm & 0xffff));
        return os;
    }

    inline std::ostream& bgtz(std::ostream& os, RegisterID rs, int imm)
    {
        write_binary(os, 0x1C000000 | (rs << OP_SH_RS) | (imm & 0xffff));
        return os;
    }

    inline std::ostream& bltz(std::ostream& os, RegisterID rs, int imm)
    {
        write_binary(os, 0x04000000 | (rs << OP_SH_RS) | (imm & 0xffff));
        return os;
    }

    inline std::ostream& beq(std::ostream& os, RegisterID rs, RegisterID rt, int imm)
    {
        write_binary(os, 0x10000000 | (rs << OP_SH_RS) | (rt << OP_SH_RT) | (imm & 0xffff));
        return os;
    }

    inline std::ostream& bne(std::ostream& os, RegisterID rs, RegisterID rt, int imm)
    {
        write_binary(os, 0x14000000 | (rs << OP_SH_RS) | (rt << OP_SH_RT) | (imm & 0xffff));
        return os;
    }

    inline std::ostream& bc1t(std::ostream& os, int offset)
    {
        write_binary(os, 0x45010000 | (offset & 0xffff));
        return os;
    }

    inline std::ostream& bc1f(std::ostream& os, int offset)
    {
        write_binary(os, 0x45000000 | (offset & 0xffff));
        return os;
    }

    inline std::ostream& addd(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46200000 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        return os;
    }

    inline std::ostream& adds(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46100000 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        return os;
    }

    inline std::ostream& subd(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46200001 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        return os;
    }

    inline std::ostream& subs(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46100001 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        return os;
    }

    inline std::ostream& muld(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46200002 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        return os;
    }

    inline std::ostream& muls(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46100002 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        return os;
    }

    inline std::ostream& divd(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46200003 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        return os;
    }

    inline std::ostream& divs(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46100003 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        return os;
    }

    inline std::ostream& lwc1(std::ostream& os, FPRegisterID ft, RegisterID rs, int offset)
    {
        write_binary(os, 0xc4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS) | (offset & 0xffff));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& ldc1(std::ostream& os, FPRegisterID ft, RegisterID rs, int offset)
    {
        write_binary(os, 0xd4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS) | (offset & 0xffff));
        return os;
    }

    inline std::ostream& swc1(std::ostream& os, FPRegisterID ft, RegisterID rs, int offset)
    {
        write_binary(os, 0xe4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS) | (offset & 0xffff));
        return os;
    }

    inline std::ostream& sdc1(std::ostream& os, FPRegisterID ft, RegisterID rs, int offset)
    {
        write_binary(os, 0xf4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS) | (offset & 0xffff));
        return os;
    }

    inline std::ostream& mtc1(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        write_binary(os, 0x44800000 | (fs << OP_SH_FS) | (rt << OP_SH_RT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& mthc1(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        write_binary(os, 0x44e00000 | (fs << OP_SH_FS) | (rt << OP_SH_RT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& mfc1(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        write_binary(os, 0x44000000 | (fs << OP_SH_FS) | (rt << OP_SH_RT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& sqrtd(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46200004 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& sqrts(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46100004 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& absd(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46200005 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& abss(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46100005 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& movd(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46200006 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& movs(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46100006 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& negd(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46200007 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& negs(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46100007 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& truncwd(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x4620000d | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& truncws(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x4610000d | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& cvtdw(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46800021 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& cvtsw(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46800020 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& cvtds(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46000021 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& cvtwd(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46200024 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& cvtsd(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46200020 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& ceqd(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46200032 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cngtd(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x4620003f | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cnged(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x4620003d | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cltd(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x4620003c | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cled(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x4620003e | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cueqd(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46200033 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& coled(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46200036 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& coltd(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46200034 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& culed(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46200037 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cultd(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46200035 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }


    inline std::ostream& cvtws(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46100024 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& cvtss(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        write_binary(os, 0x46100020 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
        return os;
    }

    inline std::ostream& ceqs(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46100032 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cngts(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x4610003f | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cnges(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x4610003d | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& clts(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46000034 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cles(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x4610003e | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cueqs(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46100033 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& coles(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46100036 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& colts(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46100034 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cules(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46100037 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cults(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        write_binary(os, 0x46100035 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop(os);
        return os;
    }

    inline std::ostream& cfc1(std::ostream& os, RegisterID rt, SPRegisterID fs)
    {
        write_binary(os, 0x44400000 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
        copDelayNop(os);
        return os;
    }

    /* Set an immediate value to a register. This may generate 1 or 2 instructions.  */
    template<typename T>
    inline std::ostream& li(std::ostream& os, RegisterID dest, T imm)
    {
        int32_t _imm = *(int32_t*)&imm;
        if (_imm >= -32768 && _imm <= 32767)
            addiu(os, dest, zero, _imm);
        else if (_imm >= 0 && _imm < 65536)
            ori(os, dest, zero, imm);
        else {
            lui(os, dest, _imm >> 16);
            if (_imm & 0xffff)
                ori(os, dest, dest, _imm);
        }
        return os;
    }

    /* Set an immediate value to a register. This will generate 2 instructions.  */
    template<typename T>
    inline std::ostream& li2(std::ostream& os, RegisterID dest, T imm)
    {
        int32_t _imm = *(int32_t*)&imm;
        lui(os, dest, _imm >> 16);
        ori(os, dest, dest, _imm);
        return os;
    }

    inline std::ostream& j(std::ostream& os, uint32_t address)
    {
        write_binary(os, (0x08000000 | ((address & 0x0fffffff) >> 2)));
        return os;
    }

    // Assembly helpers for moving data between fp and registers.
    inline std::ostream& vmov(std::ostream& os, RegisterID rd1, RegisterID rd2, FPRegisterID rn)
    {
        mfc1(os, rd1, rn);
        mfc1(os, rd2, FPRegisterID(rn + 1));
        return os;
    }

    inline std::ostream& vmov(std::ostream& os, FPRegisterID rd, RegisterID rn1, RegisterID rn2)
    {
        mtc1(os, rn1, rd);
        mtc1(os, rn2, FPRegisterID(rd + 1));
        return os;
    }
}

/*
 * https://github.com/WebKit/webkit/blob/master/Source/JavaScriptCore/assembler/MIPSAssembler.h
 * Copyright (C) 2009-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2009 University of Szeged
 * All rights reserved.
 * Copyright (C) 2010 MIPS Technologies, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MIPS TECHNOLOGIES, INC. ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL MIPS TECHNOLOGIES, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /*
  * https://github.com/71/asmdot/blob/master/languages/cpp/src/mips.cpp
  * The MIT License
  *
  * Copyright (c) 2018 Gregoire Geis
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  * THE SOFTWARE.
  */
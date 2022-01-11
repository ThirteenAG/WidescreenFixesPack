#include <cstdint>
#include <ostream>

namespace powerpc
{
    namespace
    {
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

        template<typename T>
        inline std::ostream& write_binary(std::ostream& os, T value, std::streamsize size = 4)
        {
            value = swap32(value);
            os.write(reinterpret_cast<const char*>(&value), size);
            return os;
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
        sp = r1,
        rtoc = r2,
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
    inline std::ostream& nop(std::ostream& os)
    {
        return write_binary(os, (0x60000000));
    }

    inline std::ostream& blr(std::ostream& os)
    {
        return write_binary(os, (0x4E800020));
    }

    template<typename T>
    inline std::ostream& li(std::ostream& os, RegisterID rd, T simm)
    {
        return write_binary(os, (0x38000000 | (rd << 21) | (*(int32_t*)&simm & 0xffff)));
    }

    inline std::ostream& add(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000214 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& addo(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000614 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& addc(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000014 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& addco(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000414 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& adde(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000114 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& addeo(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000514 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    template<typename T>
    inline std::ostream& addi(std::ostream& os, RegisterID rd, RegisterID ra, T simm)
    {
        return write_binary(os, (0x38000000 | (rd << 21) | (ra << 16) | (*(int32_t*)&simm & 0xffff)));
    }

    template<typename T>
    inline std::ostream& addic(std::ostream& os, RegisterID rd, RegisterID ra, T simm)
    {
        return write_binary(os, (0x30000000 | (rd << 21) | (ra << 16) | (*(int32_t*)&simm & 0xffff)));
    }

    template<typename T>
    inline std::ostream& addis(std::ostream& os, RegisterID rd, RegisterID ra, T simm)
    {
        return write_binary(os, (0x3C000000 | (rd << 21) | (ra << 16) | (*(int32_t*)&simm & 0xffff)));
    }

    inline std::ostream& addme(std::ostream& os, RegisterID rd, RegisterID ra)
    {
        return write_binary(os, (0x7C0001D4 | (rd << 21) | (ra << 16)) | 1);
    }

    inline std::ostream& addmeo(std::ostream& os, RegisterID rd, RegisterID ra)
    {
        return write_binary(os, (0x7C0005D4 | (rd << 21) | (ra << 16)) | 1);
    }

    inline std::ostream& addze(std::ostream& os, RegisterID rd, RegisterID ra)
    {
        return write_binary(os, (0x7C000194 | (rd << 21) | (ra << 16)) | 1);
    }

    inline std::ostream& addzeo(std::ostream& os, RegisterID rd, RegisterID ra)
    {
        return write_binary(os, (0x7C000594 | (rd << 21) | (ra << 16)) | 1);
    }

    inline std::ostream& _and(std::ostream& os, RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return write_binary(os, (0x7C000038 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    template<typename T>
    inline std::ostream& andi(std::ostream& os, RegisterID ra, RegisterID rs, T uimm)
    {
        return write_binary(os, (0x70000000 | (rs << 21) | (ra << 16) | (*(uint32_t*)&uimm & 0xffff)));
    }

    template<typename T>
    inline std::ostream& andis(std::ostream& os, RegisterID ra, RegisterID rs, T uimm)
    {
        return write_binary(os, (0x74000000 | (rs << 21) | (ra << 16) | (*(uint32_t*)&uimm & 0xffff)));
    }

    inline std::ostream& cntlzw(std::ostream& os, RegisterID ra, RegisterID rs)
    {
        return write_binary(os, (0x7C000034 | (rs << 21) | (ra << 16)) | 1);
    }

    inline std::ostream& divw(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0003D6 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& divwo(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0007D6 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& divwu(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000396 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& divwuo(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000796 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& eieio(std::ostream& os)
    {
        return write_binary(os, (0x7C0006AC));
    }

    inline std::ostream& eqv(std::ostream& os, RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return write_binary(os, (0x7C000238 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& extsb(std::ostream& os, RegisterID ra, RegisterID rs)
    {
        return write_binary(os, (0x7C000774 | (rs << 21) | (ra << 16)) | 1);
    }

    inline std::ostream& extsh(std::ostream& os, RegisterID ra, RegisterID rs)
    {
        return write_binary(os, (0x7C000734 | (rs << 21) | (ra << 16)) | 1);
    }

    inline std::ostream& lbzux(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0000EE | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lbzx(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0000AE | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lhaux(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0002EE | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lhax(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0002AE | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lhbrx(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00062C | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lhzux(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00026E | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lhzx(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00022E | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lswx(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00042A | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lwbrx(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00042C | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lwzux(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00006E | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lwzx(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00002E | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& mfcr(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, (0x7C000026 | (rd << 21)));
    }

    inline std::ostream& mulhw(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000096 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& mulhwu(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000016 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    template<typename T>
    inline std::ostream& mulli(std::ostream& os, RegisterID rd, RegisterID ra, T simm)
    {
        return write_binary(os, (0x1C000000 | (rd << 21) | (ra << 16) | (*(int32_t*)&simm & 0xffff)));
    }

    inline std::ostream& mullw(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0001D6 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& mullwo(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0005D6 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& nand(std::ostream& os, RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return write_binary(os, (0x7C0003B8 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& nor(std::ostream& os, RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return write_binary(os, (0x7C0000F8 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& _or(std::ostream& os, RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return write_binary(os, (0x7C000378 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& orc(std::ostream& os, RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return write_binary(os, (0x7C000338 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    template<typename T>
    inline std::ostream& ori(std::ostream& os, RegisterID ra, RegisterID rs, T uimm)
    {
        return write_binary(os, (0x60000000 | (rs << 21) | (ra << 16) | (*(uint32_t*)&uimm & 0xffff)));
    }

    template<typename T>
    inline std::ostream& oris(std::ostream& os, RegisterID ra, RegisterID rs, T uimm)
    {
        return write_binary(os, (0x64000000 | (rs << 21) | (ra << 16) | (*(uint32_t*)&uimm & 0xffff)));
    }

    inline std::ostream& slw(std::ostream& os, RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return write_binary(os, (0x7C000030 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& sraw(std::ostream& os, RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return write_binary(os, (0x7C000630 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& srw(std::ostream& os, RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return write_binary(os, (0x7C000430 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& stbux(std::ostream& os, RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0001EE | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& stbx(std::ostream& os, RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0001AE | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& sthbrx(std::ostream& os, RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00072C | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& sthux(std::ostream& os, RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00036E | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& sthx(std::ostream& os, RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00032E | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& stswx(std::ostream& os, RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00052A | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& stwbrx(std::ostream& os, RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00052C | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& stwux(std::ostream& os, RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00016E | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& stwx(std::ostream& os, RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00012E | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& subf(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000050 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& subfo(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000450 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& subfc(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000010 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& subfco(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000410 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& subfe(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000110 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline std::ostream& subfeo(std::ostream& os, RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C000510 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    template<typename T>
    inline std::ostream& subfic(std::ostream& os, RegisterID rd, RegisterID ra, T simm)
    {
        return write_binary(os, (0x20000000 | (rd << 21) | (ra << 16) | (*(int32_t*)&simm & 0xffff)));
    }

    inline std::ostream& subfme(std::ostream& os, RegisterID rd, RegisterID ra)
    {
        return write_binary(os, (0x7C0001D0 | (rd << 21) | (ra << 16)) | 1);
    }

    inline std::ostream& subfmeo(std::ostream& os, RegisterID rd, RegisterID ra)
    {
        return write_binary(os, (0x7C0005D0 | (rd << 21) | (ra << 16)) | 1);
    }

    inline std::ostream& subfze(std::ostream& os, RegisterID rd, RegisterID ra)
    {
        return write_binary(os, (0x7C000190 | (rd << 21) | (ra << 16)) | 1);
    }

    inline std::ostream& subfzeo(std::ostream& os, RegisterID rd, RegisterID ra)
    {
        return write_binary(os, (0x7C000590 | (rd << 21) | (ra << 16)) | 1);
    }

    inline std::ostream& sync(std::ostream& os)
    {
        return write_binary(os, (0x7C0004AC));
    }

    inline std::ostream& _xor(std::ostream& os, RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return write_binary(os, (0x7C000278 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    template<typename T>
    inline std::ostream& xori(std::ostream& os, RegisterID ra, RegisterID rs, T uimm)
    {
        return write_binary(os, (0x68000000 | (rs << 21) | (ra << 16) | (*(uint32_t*)&uimm & 0xffff)));
    }

    template<typename T>
    inline std::ostream& xoris(std::ostream& os, RegisterID ra, RegisterID rs, T uimm)
    {
        return write_binary(os, (0x6C000000 | (rs << 21) | (ra << 16) | (*(uint32_t*)&uimm & 0xffff)));
    }

    inline std::ostream& lfsx(std::ostream& os, FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00042E | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lfsux(std::ostream& os, FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00046E | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lfdx(std::ostream& os, FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0004AE | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& lfdux(std::ostream& os, FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0004EE | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& stfsx(std::ostream& os, FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00052E | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& stfsux(std::ostream& os, FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C00056E | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& stfdx(std::ostream& os, FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0005AE | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& stfdux(std::ostream& os, FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0005EE | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& stfiwx(std::ostream& os, FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return write_binary(os, (0x7C0007AE | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline std::ostream& fmr(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC000090 | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fneg(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC000050 | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fabs(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC000210 | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fnabs(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC000110 | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fadd(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frb)
    {
        return write_binary(os, (0xFC00002A | (frt << 21) | (fra << 16)) | 1);
    }

    inline std::ostream& fsub(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frb)
    {
        return write_binary(os, (0xFC000028 | (frt << 21) | (fra << 16)) | 1);
    }

    inline std::ostream& fadds(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frb)
    {
        return write_binary(os, (0xEC00002A | (frt << 21) | (fra << 16)) | 1);
    }

    inline std::ostream& fsubs(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frb)
    {
        return write_binary(os, (0xEC000028 | (frt << 21) | (fra << 16)) | 1);
    }

    inline std::ostream& fmul(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frc)
    {
        return write_binary(os, (0xFC000032 | (frt << 21) | (fra << 16)) | 1);
    }

    inline std::ostream& fdiv(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frb)
    {
        return write_binary(os, (0xFC000024 | (frt << 21) | (fra << 16)) | 1);
    }

    inline std::ostream& fmuls(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frc)
    {
        return write_binary(os, (0xEC000032 | (frt << 21) | (fra << 16)) | 1);
    }

    inline std::ostream& fdivs(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frb)
    {
        return write_binary(os, (0xEC000024 | (frt << 21) | (fra << 16)) | 1);
    }

    inline std::ostream& fmadd(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return write_binary(os, (0xFC00003A | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline std::ostream& fmsub(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return write_binary(os, (0xFC000038 | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline std::ostream& fmadds(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return write_binary(os, (0xEC00003A | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline std::ostream& fmsubs(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return write_binary(os, (0xEC000038 | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline std::ostream& fnmadd(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return write_binary(os, (0xFC00003E | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline std::ostream& fnmsub(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return write_binary(os, (0xFC00003C | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline std::ostream& fnmadds(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return write_binary(os, (0xEC00003E | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline std::ostream& fnmsubs(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return write_binary(os, (0xEC00003C | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline std::ostream& frsp(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC000018 | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fctid(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC00065C | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fctidz(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC00065E | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fctiw(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC00001C | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fctiwz(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC00001E | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fctfid(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC00069C | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& mffs(std::ostream& os, FPRegisterID frt)
    {
        return write_binary(os, (0xFC00048E | (frt << 21)) | 1);
    }

    inline std::ostream& mtfsf(std::ostream& os, FPRegisterID flm, FPRegisterID frb)
    {
        return write_binary(os, (0xFC00058E | (flm << 17) | (frb << 11)) | 1);
    }

    inline std::ostream& fsqrt(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC00002C | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fre(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC000030 | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fsqrts(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xEC00002C | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fres(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xEC000030 | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& frsqrte(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xFC000034 | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& frsqrtes(std::ostream& os, FPRegisterID frt, FPRegisterID frb)
    {
        return write_binary(os, (0xEC000034 | (frt << 21) | (frb << 11)) | 1);
    }

    inline std::ostream& fsel(std::ostream& os, FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return write_binary(os, (0xFC00002E | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline std::ostream& dssall(std::ostream& os)
    {
        return write_binary(os, (0x7E00066C));
    }

    ///////////////////////////////

    inline uint32_t nop()
    {
        return swap32((0x60000000));
    }

    inline uint32_t blr()
    {
        return swap32((0x4E800020));
    }

    template<typename T>
    inline uint32_t li(RegisterID rd, T simm)
    {
        return swap32((0x38000000 | (rd << 21) | (*(int32_t*)&simm & 0xffff)));
    }

    inline uint32_t add(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000214 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t addo(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000614 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t addc(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000014 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t addco(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000414 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t adde(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000114 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t addeo(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000514 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    template<typename T>
    inline uint32_t addi(RegisterID rd, RegisterID ra, T simm)
    {
        return swap32((0x38000000 | (rd << 21) | (ra << 16) | (*(int32_t*)&simm & 0xffff)));
    }

    template<typename T>
    inline uint32_t addic(RegisterID rd, RegisterID ra, T simm)
    {
        return swap32((0x30000000 | (rd << 21) | (ra << 16) | (*(int32_t*)&simm & 0xffff)));
    }

    template<typename T>
    inline uint32_t addis(RegisterID rd, RegisterID ra, T simm)
    {
        return swap32((0x3C000000 | (rd << 21) | (ra << 16) | (*(int32_t*)&simm & 0xffff)));
    }

    inline uint32_t addme(RegisterID rd, RegisterID ra)
    {
        return swap32((0x7C0001D4 | (rd << 21) | (ra << 16)) | 1);
    }

    inline uint32_t addmeo(RegisterID rd, RegisterID ra)
    {
        return swap32((0x7C0005D4 | (rd << 21) | (ra << 16)) | 1);
    }

    inline uint32_t addze(RegisterID rd, RegisterID ra)
    {
        return swap32((0x7C000194 | (rd << 21) | (ra << 16)) | 1);
    }

    inline uint32_t addzeo(RegisterID rd, RegisterID ra)
    {
        return swap32((0x7C000594 | (rd << 21) | (ra << 16)) | 1);
    }

    inline uint32_t _and(RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return swap32((0x7C000038 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    template<typename T>
    inline uint32_t andi(RegisterID ra, RegisterID rs, T uimm)
    {
        return swap32((0x70000000 | (rs << 21) | (ra << 16) | (*(uint32_t*)&uimm & 0xffff)));
    }

    template<typename T>
    inline uint32_t andis(RegisterID ra, RegisterID rs, T uimm)
    {
        return swap32((0x74000000 | (rs << 21) | (ra << 16) | (*(uint32_t*)&uimm & 0xffff)));
    }

    inline uint32_t cntlzw(RegisterID ra, RegisterID rs)
    {
        return swap32((0x7C000034 | (rs << 21) | (ra << 16)) | 1);
    }

    inline uint32_t divw(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0003D6 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t divwo(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0007D6 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t divwu(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000396 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t divwuo(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000796 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t eieio()
    {
        return swap32((0x7C0006AC));
    }

    inline uint32_t eqv(RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return swap32((0x7C000238 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t extsb(RegisterID ra, RegisterID rs)
    {
        return swap32((0x7C000774 | (rs << 21) | (ra << 16)) | 1);
    }

    inline uint32_t extsh(RegisterID ra, RegisterID rs)
    {
        return swap32((0x7C000734 | (rs << 21) | (ra << 16)) | 1);
    }

    inline uint32_t lbzux(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0000EE | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lbzx(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0000AE | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lhaux(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0002EE | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lhax(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0002AE | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lhbrx(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00062C | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lhzux(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00026E | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lhzx(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00022E | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lswx(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00042A | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lwbrx(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00042C | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lwzux(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00006E | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lwzx(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00002E | (rd << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t mfcr(RegisterID rd)
    {
        return swap32((0x7C000026 | (rd << 21)));
    }

    inline uint32_t mulhw(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000096 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t mulhwu(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000016 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    template<typename T>
    inline uint32_t mulli(RegisterID rd, RegisterID ra, T simm)
    {
        return swap32((0x1C000000 | (rd << 21) | (ra << 16) | (*(int32_t*)&simm & 0xffff)));
    }

    inline uint32_t mullw(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0001D6 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t mullwo(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0005D6 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t nand(RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return swap32((0x7C0003B8 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t nor(RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return swap32((0x7C0000F8 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t _or(RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return swap32((0x7C000378 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t orc(RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return swap32((0x7C000338 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    template<typename T>
    inline uint32_t ori(RegisterID ra, RegisterID rs, T uimm)
    {
        return swap32((0x60000000 | (rs << 21) | (ra << 16) | (*(uint32_t*)&uimm & 0xffff)));
    }

    template<typename T>
    inline uint32_t oris(RegisterID ra, RegisterID rs, T uimm)
    {
        return swap32((0x64000000 | (rs << 21) | (ra << 16) | (*(uint32_t*)&uimm & 0xffff)));
    }

    inline uint32_t slw(RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return swap32((0x7C000030 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t sraw(RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return swap32((0x7C000630 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t srw(RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return swap32((0x7C000430 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t stbux(RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0001EE | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t stbx(RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0001AE | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t sthbrx(RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00072C | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t sthux(RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00036E | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t sthx(RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00032E | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t stswx(RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00052A | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t stwbrx(RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00052C | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t stwux(RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00016E | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t stwx(RegisterID rs, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00012E | (rs << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t subf(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000050 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t subfo(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000450 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t subfc(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000010 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t subfco(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000410 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t subfe(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000110 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    inline uint32_t subfeo(RegisterID rd, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C000510 | (rd << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    template<typename T>
    inline uint32_t subfic(RegisterID rd, RegisterID ra, T simm)
    {
        return swap32((0x20000000 | (rd << 21) | (ra << 16) | (*(int32_t*)&simm & 0xffff)));
    }

    inline uint32_t subfme(RegisterID rd, RegisterID ra)
    {
        return swap32((0x7C0001D0 | (rd << 21) | (ra << 16)) | 1);
    }

    inline uint32_t subfmeo(RegisterID rd, RegisterID ra)
    {
        return swap32((0x7C0005D0 | (rd << 21) | (ra << 16)) | 1);
    }

    inline uint32_t subfze(RegisterID rd, RegisterID ra)
    {
        return swap32((0x7C000190 | (rd << 21) | (ra << 16)) | 1);
    }

    inline uint32_t subfzeo(RegisterID rd, RegisterID ra)
    {
        return swap32((0x7C000590 | (rd << 21) | (ra << 16)) | 1);
    }

    inline uint32_t sync()
    {
        return swap32((0x7C0004AC));
    }

    inline uint32_t _xor(RegisterID ra, RegisterID rs, RegisterID rb)
    {
        return swap32((0x7C000278 | (rs << 21) | (ra << 16) | (rb << 11)) | 1);
    }

    template<typename T>
    inline uint32_t xori(RegisterID ra, RegisterID rs, T uimm)
    {
        return swap32((0x68000000 | (rs << 21) | (ra << 16) | (*(uint32_t*)&uimm & 0xffff)));
    }

    template<typename T>
    inline uint32_t xoris(RegisterID ra, RegisterID rs, T uimm)
    {
        return swap32((0x6C000000 | (rs << 21) | (ra << 16) | (*(uint32_t*)&uimm & 0xffff)));
    }

    inline uint32_t lfsx(FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00042E | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lfsux(FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00046E | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lfdx(FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0004AE | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t lfdux(FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0004EE | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t stfsx(FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00052E | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t stfsux(FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C00056E | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t stfdx(FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0005AE | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t stfdux(FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0005EE | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t stfiwx(FPRegisterID frt, RegisterID ra, RegisterID rb)
    {
        return swap32((0x7C0007AE | (frt << 21) | (ra << 16) | (rb << 11)));
    }

    inline uint32_t fmr(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC000090 | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fneg(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC000050 | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fabs(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC000210 | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fnabs(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC000110 | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fadd(FPRegisterID frt, FPRegisterID fra, FPRegisterID frb)
    {
        return swap32((0xFC00002A | (frt << 21) | (fra << 16)) | 1);
    }

    inline uint32_t fsub(FPRegisterID frt, FPRegisterID fra, FPRegisterID frb)
    {
        return swap32((0xFC000028 | (frt << 21) | (fra << 16)) | 1);
    }

    inline uint32_t fadds(FPRegisterID frt, FPRegisterID fra, FPRegisterID frb)
    {
        return swap32((0xEC00002A | (frt << 21) | (fra << 16)) | 1);
    }

    inline uint32_t fsubs(FPRegisterID frt, FPRegisterID fra, FPRegisterID frb)
    {
        return swap32((0xEC000028 | (frt << 21) | (fra << 16)) | 1);
    }

    inline uint32_t fmul(FPRegisterID frt, FPRegisterID fra, FPRegisterID frc)
    {
        return swap32((0xFC000032 | (frt << 21) | (fra << 16)) | 1);
    }

    inline uint32_t fdiv(FPRegisterID frt, FPRegisterID fra, FPRegisterID frb)
    {
        return swap32((0xFC000024 | (frt << 21) | (fra << 16)) | 1);
    }

    inline uint32_t fmuls(FPRegisterID frt, FPRegisterID fra, FPRegisterID frc)
    {
        return swap32((0xEC000032 | (frt << 21) | (fra << 16)) | 1);
    }

    inline uint32_t fdivs(FPRegisterID frt, FPRegisterID fra, FPRegisterID frb)
    {
        return swap32((0xEC000024 | (frt << 21) | (fra << 16)) | 1);
    }

    inline uint32_t fmadd(FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return swap32((0xFC00003A | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline uint32_t fmsub(FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return swap32((0xFC000038 | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline uint32_t fmadds(FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return swap32((0xEC00003A | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline uint32_t fmsubs(FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return swap32((0xEC000038 | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline uint32_t fnmadd(FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return swap32((0xFC00003E | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline uint32_t fnmsub(FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return swap32((0xFC00003C | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline uint32_t fnmadds(FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return swap32((0xEC00003E | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline uint32_t fnmsubs(FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return swap32((0xEC00003C | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline uint32_t frsp(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC000018 | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fctid(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC00065C | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fctidz(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC00065E | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fctiw(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC00001C | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fctiwz(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC00001E | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fctfid(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC00069C | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t mffs(FPRegisterID frt)
    {
        return swap32((0xFC00048E | (frt << 21)) | 1);
    }

    inline uint32_t mtfsf(FPRegisterID flm, FPRegisterID frb)
    {
        return swap32((0xFC00058E | (flm << 17) | (frb << 11)) | 1);
    }

    inline uint32_t fsqrt(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC00002C | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fre(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC000030 | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fsqrts(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xEC00002C | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fres(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xEC000030 | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t frsqrte(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xFC000034 | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t frsqrtes(FPRegisterID frt, FPRegisterID frb)
    {
        return swap32((0xEC000034 | (frt << 21) | (frb << 11)) | 1);
    }

    inline uint32_t fsel(FPRegisterID frt, FPRegisterID fra, FPRegisterID frc, FPRegisterID frb)
    {
        return swap32((0xFC00002E | (frt << 21) | (fra << 16) | (frc << 6) | (frb << 11)) | 1);
    }

    inline uint32_t dssall()
    {
        return swap32((0x7E00066C));
    }
}

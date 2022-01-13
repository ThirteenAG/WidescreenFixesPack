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
        inline std::ostream& write_binary(std::ostream& os, T value, std::streamsize size = 4)
        {
#if BIGENDIAN
            value = swap32(value);
#endif
            os.write(reinterpret_cast<const char*>(&value), size);
            return os;
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
    //With __***__ marked functions without tests below
    inline uint32_t write_directly(uint32_t data)
    {
        return (swap32(data));
    }

    inline uint32_t nop()
    {
        return (0x00000000);
    }

    inline uint32_t __mfbpc__(RegisterID rt)
    {
        return (0x4000c000 | (rt << OP_SH_RT));
    }

    inline uint32_t __mfdab__(RegisterID rt)
    {
        return (0x4000c004 | (rt << OP_SH_RT));
    }

    inline uint32_t __mfdabm__(RegisterID rt)
    {
        return (0x4000c005 | (rt << OP_SH_RT));
    }

    inline uint32_t __mfdvb__(RegisterID rt)
    {
        return (0x4000c006 | (rt << OP_SH_RT));
    }

    inline uint32_t __mfdvbm__(RegisterID rt)
    {
        return (0x4000c007 | (rt << OP_SH_RT));
    }

    inline uint32_t __mfiab__(RegisterID rt)
    {
        return (0x4000c002 | (rt << OP_SH_RT));
    }

    inline uint32_t __mfiabm__(RegisterID rt)
    {
        return (0x4000c003 | (rt << OP_SH_RT));
    }

    inline uint32_t __mfpc__(RegisterID rt, int32_t i)
    {
        return (0x4000c801 | (rt << OP_SH_RT) | (i & 0xffff));
    }

    inline uint32_t __mfps__(RegisterID rt, int32_t i)
    {
        return (0x4000c800 | (rt << OP_SH_RT) | (i & 0xffff));
    }

    inline uint32_t __mtbpc__(RegisterID rt)
    {
        return (0x4080c000 | (rt << OP_SH_RT));
    }

    inline uint32_t __mtdab__(RegisterID rt)
    {
        return (0x4080c004 | (rt << OP_SH_RT));
    }

    inline uint32_t __mtdabm__(RegisterID rt)
    {
        return (0x4080c005 | (rt << OP_SH_RT));
    }

    inline uint32_t __mtdvb__(RegisterID rt)
    {
        return (0x4080c006 | (rt << OP_SH_RT));
    }

    inline uint32_t __mtdvbm__(RegisterID rt)
    {
        return (0x4080c007 | (rt << OP_SH_RT));
    }

    inline uint32_t __mtpc__(RegisterID rt, int32_t i)
    {
        return (0x4080c801 | (rt << OP_SH_RT) | (i & 0xffff));
    }

    inline uint32_t __mtps__(RegisterID rt, int32_t i)
    {
        return (0x4080c800 | (rt << OP_SH_RT) | (i & 0xffff));
    }

    inline uint32_t bc0f(int32_t offset)
    {
        return (0x4100FF00 | (offset & 0xffff));
    }

    inline uint32_t __bc0fl__(int32_t offset)
    {
        return (0x41020000 | (offset & 0xffff));
    }

    inline uint32_t __bc0t__(int32_t offset)
    {
        return (0x41010000 | (offset & 0xffff));
    }

    inline uint32_t __bc0tl__(int32_t offset)
    {
        return (0x41030000 | (offset & 0xffff));
    }

    inline uint32_t __bc2f__(int32_t offset)
    {
        return (0x49000000 | (offset & 0xffff));
    }

    inline uint32_t __bc2fl__(int32_t offset)
    {
        return (0x49020000 | (offset & 0xffff));
    }

    inline uint32_t __bc2t__(int32_t offset)
    {
        return (0x49010000 | (offset & 0xffff));
    }

    inline uint32_t __bc2tl__(int32_t offset)
    {
        return (0x49030000 | (offset & 0xffff));
    }

    inline uint32_t di()
    {
        return (0x42000039);
    }

    inline uint32_t div1(RegisterID rs, RegisterID rt)
    {
        return (0x7000001a | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __divu1__(RegisterID rs, RegisterID rt)
    {
        return (0x7000001b | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t ei()
    {
        return (0x42000038);
    }

    inline uint32_t eret()
    {
        return (0x42000018);
    }

    inline uint32_t madd(RegisterID rs, RegisterID rt)
    {
        return (0x70000000 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t madd(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000000 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __madd1__(RegisterID rs, RegisterID rt)
    {
        return (0x70000020 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __madd1__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000020 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t maddu(RegisterID rs, RegisterID rt)
    {
        return (0x70000001 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t maddu(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000001 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __maddu1__(RegisterID rs, RegisterID rt)
    {
        return (0x70000021 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __maddu1__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000021 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __mfc0__(RegisterID rt, RegisterID rd)
    {
        return (0x40000000 | (rt << OP_SH_RT) | (rd << OP_SH_RD));
    }

    inline uint32_t mfhi1(RegisterID rd)
    {
        return (0x70000010 | (rd << OP_SH_RD));
    }

    inline uint32_t mflo1(RegisterID rd)
    {
        return (0x70000012 | (rd << OP_SH_RD));
    }

    inline uint32_t mfsa(RegisterID rd)
    {
        return (0x00000028 | (rd << OP_SH_RD));
    }

    inline uint32_t __mtc0__(RegisterID rt, RegisterID rd)
    {
        return (0x40800000 | (rt << OP_SH_RT) | (rd << OP_SH_RD));
    }

    inline uint32_t __mtiab__(RegisterID rt)
    {
        return (0x4080c002 | (rt << OP_SH_RT));
    }

    inline uint32_t __mtiabm__(RegisterID rt)
    {
        return (0x4080c003 | (rt << OP_SH_RT));
    }

    inline uint32_t mthi1(RegisterID rs)
    {
        return (0x70000011 | (rs << OP_SH_RS));
    }

    inline uint32_t mtlo1(RegisterID rs)
    {
        return (0x70000013 | (rs << OP_SH_RS));
    }

    inline uint32_t mtsa(RegisterID rs)
    {
        return (0x00000029 | (rs << OP_SH_RS));
    }

    template<typename T>
    inline uint32_t mtsab(RegisterID rs, T imm)
    {
        return (0x04180000 | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    template<typename T>
    inline uint32_t __mtsah__(RegisterID rs, T imm)
    {
        return (0x04190000 | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    inline uint32_t mult(RegisterID rs, RegisterID rt)
    {
        return (0x00000018 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t mult(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x00000018 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t mult1(RegisterID rs, RegisterID rt)
    {
        return (0x70000018 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t mult1(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000018 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t multu(RegisterID rs, RegisterID rt)
    {
        return (0x00000019 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t multu(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x00000019 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t multu1(RegisterID rs, RegisterID rt)
    {
        return (0x70000019 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t multu1(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000019 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __syncl__()
    {
        return (0x0000000f);
    }

    inline uint32_t sync()
    {
        return (0x0000000f);
    }

    inline uint32_t __syncp__()
    {
        return (0x0000040f);
    }

    inline uint32_t syscall()
    {
        return (0x0000000c);
    }

    inline uint32_t __teq__(RegisterID rs, RegisterID rt)
    {
        return (0x00000034 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    template<typename T>
    inline uint32_t __teqi__(RegisterID rs, T imm)
    {
        return (0x040c0000 | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    inline uint32_t tge(RegisterID rs, RegisterID rt)
    {
        return (0x0000bb30 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    template<typename T>
    inline uint32_t __tgei__(RegisterID rs, T imm)
    {
        return (0x04080000 | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    template<typename T>
    inline uint32_t __tgeiu__(RegisterID rs, T imm)
    {
        return (0x04090000 | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    inline uint32_t __tgeu__(RegisterID rs, RegisterID rt)
    {
        return (0x00000031 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __tlt__(RegisterID rs, RegisterID rt)
    {
        return (0x00000032 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    template<typename T>
    inline uint32_t __tlti__(RegisterID rs, T imm)
    {
        return (0x040a0000 | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    template<typename T>
    inline uint32_t __tltiu__(RegisterID rs, T imm)
    {
        return (0x040b0000 | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    inline uint32_t __tltu__(RegisterID rs, RegisterID rt)
    {
        return (0x00000033 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __tne__(RegisterID rs, RegisterID rt)
    {
        return (0x00000036 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    template<typename T>
    inline uint32_t __tnei__(RegisterID rs, T imm)
    {
        return (0x040e0000 | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    inline uint32_t __tlbp__()
    {
        return (0x42000008);
    }

    inline uint32_t __tlbr__()
    {
        return (0x42000001);
    }

    inline uint32_t tlbwi()
    {
        return (0x42000002);
    }

    inline uint32_t __tlbwr__()
    {
        return (0x42000006);
    }

    inline uint32_t abss(FPRegisterID fd, FPRegisterID fs)
    {
        return (0x46000005 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
    }

    inline uint32_t __absd__(FPRegisterID fd, FPRegisterID fs)
    {
        return (0x46200005 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
    }

    inline uint32_t adds(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000000 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t __addd__(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46200000 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t addas(FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000018 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t bc1f(int32_t offset)
    {
        return (0x45000000 | (offset & 0xffff));
    }

    inline uint32_t bc1fl(int32_t offset)
    {
        return (0x45020000 | (offset & 0xffff));
    }

    inline uint32_t __bc1t__(int32_t offset)
    {
        return (0x45010000 | (offset & 0xffff));
    }

    inline uint32_t bc1tl(int32_t offset)
    {
        return (0x45030000 | (offset & 0xffff));
    }

    inline uint32_t ceqs(FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000032 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t __cfs__(FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000030 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t __cfc1__(RegisterID rt, FPRegisterID fs)
    {
        return (0x44400000 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
    }

    inline uint32_t __ctc1__(RegisterID rt, FPRegisterID fs)
    {
        return (0x44c00000 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
    }

    inline uint32_t cvtsw(FPRegisterID fd, FPRegisterID fs)
    {
        return (0x46800020 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
    }

    inline uint32_t cvtws(FPRegisterID fd, FPRegisterID fs)
    {
        return (0x46000024 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
    }

    inline uint32_t divs(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000003 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t madds(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return (0x4600001c | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t maddas(FPRegisterID fs, FPRegisterID ft)
    {
        return (0x4600001e | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t maxs(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000028 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t mfc1(RegisterID rt, FPRegisterID fs)
    {
        return (0x44000000 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
    }

    inline uint32_t mins(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000029 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t movs(FPRegisterID fd, FPRegisterID fs)
    {
        return (0x46000006 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
    }

    inline uint32_t msubs(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return (0x4600001d | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t msubas(FPRegisterID fs, FPRegisterID ft)
    {
        return (0x4600001f | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t mtc1(RegisterID rt, FPRegisterID fs)
    {
        return (0x44800000 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
    }

    inline uint32_t muls(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000002 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t mulas(FPRegisterID fs, FPRegisterID ft)
    {
        return (0x4600001a | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t negs(FPRegisterID fd, FPRegisterID fs)
    {
        return (0x46000007 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
    }

    inline uint32_t rsqrts(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000016 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t sqrts(FPRegisterID fd, FPRegisterID ft)
    {
        return (0x46000004 | (fd << OP_SH_FD) | (ft << OP_SH_FT));
    }

    inline uint32_t subs(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000001 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t __subd__(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46200001 | (fd << OP_SH_FD) | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t __subas__(FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000019 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t __sqrtd__(FPRegisterID fd, FPRegisterID fs)
    {
        return (0x46202004 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
    }

    inline uint32_t __rsqrtd__(FPRegisterID fd, FPRegisterID fs)
    {
        return (0x46200016 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
    }

    inline uint32_t __pabsh__(RegisterID rd, RegisterID rt)
    {
        return (0x70000168 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
    }

    inline uint32_t __pabsw__(RegisterID rd, RegisterID rt)
    {
        return (0x70000068 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
    }

    inline uint32_t __paddb__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000208 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t paddh(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000108 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __paddsb__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000608 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __paddsh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000508 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t paddsw(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000408 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __paddub__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000628 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __padduh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000528 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __padduw__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000428 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t paddw(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000008 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __padsbh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000128 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pand(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000489 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pceqb__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700002a8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pceqh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700001a8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pceqw__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700000a8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pcgtb__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000288 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pcgth(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000188 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pcgtw__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000088 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pcpyh(RegisterID rd, RegisterID rt)
    {
        return (0x700006e9 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
    }

    inline uint32_t pcpyld(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000389 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pcpyud(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700003a9 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pdivbw__(RegisterID rs, RegisterID rt)
    {
        return (0x70000749 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pdivuw__(RegisterID rs, RegisterID rt)
    {
        return (0x70000369 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pdivw__(RegisterID rs, RegisterID rt)
    {
        return (0x70000349 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pexch(RegisterID rd, RegisterID rt)
    {
        return (0x700006a9 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
    }

    inline uint32_t __pexeh__(RegisterID rd, RegisterID rt)
    {
        return (0x70000689 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
    }

    inline uint32_t pexew(RegisterID rd, RegisterID rt)
    {
        return (0x70000789 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
    }

    inline uint32_t __pext5__(RegisterID rd, RegisterID rt)
    {
        return (0x70000788 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
    }

    inline uint32_t pextlb(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000688 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pextlh(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000588 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pextlw(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000488 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pextub(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700006a8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pextuh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700005a8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pextuw(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700004a8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __phmadh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000449 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __phmsbh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000549 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pinteh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700002a9 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pinth__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000289 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __plzcw__(RegisterID rd, RegisterID rs)
    {
        return (0x70000004 | (rd << OP_SH_RD) | (rs << OP_SH_RS));
    }

    inline uint32_t __pmaddh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000409 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pmadduw__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000029 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pmaddw__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000009 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pmaxh(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700001c8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pmaxw(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700000c8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pmfhi(RegisterID rd)
    {
        return (0x70000209 | (rd << OP_SH_RD));
    }

    inline uint32_t __pmfhllh__(RegisterID rd)
    {
        return (0x700000f0 | (rd << OP_SH_RD));
    }

    inline uint32_t __pmfhllw__(RegisterID rd)
    {
        return (0x70000030 | (rd << OP_SH_RD));
    }

    inline uint32_t __pmfhlsh__(RegisterID rd)
    {
        return (0x70000130 | (rd << OP_SH_RD));
    }

    inline uint32_t __pmfhlslw__(RegisterID rd)
    {
        return (0x700000b0 | (rd << OP_SH_RD));
    }

    inline uint32_t __pmfhluw__(RegisterID rd)
    {
        return (0x70000070 | (rd << OP_SH_RD));
    }

    inline uint32_t pmflo(RegisterID rd)
    {
        return (0x70000249 | (rd << OP_SH_RD));
    }

    inline uint32_t pminh(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700001e8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pminw(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700000e8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pmsubh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000509 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pmsubw__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000109 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pmthi__(RegisterID rs)
    {
        return (0x70000229 | (rs << OP_SH_RS));
    }

    inline uint32_t __pmthllw__(RegisterID rs)
    {
        return (0x70000031 | (rs << OP_SH_RS));
    }

    inline uint32_t __pmtlo__(RegisterID rs)
    {
        return (0x70000269 | (rs << OP_SH_RS));
    }

    inline uint32_t pmulth(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000709 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pmultuw__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000329 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __pmultw__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000309 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pnor(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700004e9 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t por(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700004a9 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __ppac5__(RegisterID rd, RegisterID rt)
    {
        return (0x700007c8 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
    }

    inline uint32_t ppacb(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700006c8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t ppach(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700005c8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __ppacw__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700004c8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __prevh__(RegisterID rd, RegisterID rt)
    {
        return (0x700006c9 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
    }

    inline uint32_t prot3w(RegisterID rd, RegisterID rt)
    {
        return (0x700007c9 | (rd << OP_SH_RD) | (rt << OP_SH_RT));
    }

    inline uint32_t psllh(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x70000034 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t __psllvw__(RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return (0x70000089 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
    }

    inline uint32_t psllw(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x7000003c | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t psrah(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x70000037 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t __psravw__(RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return (0x700000e9 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
    }

    inline uint32_t psraw(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x7000003f | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t psrlh(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x70000036 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t __psrlvw__(RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return (0x700000c9 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
    }

    inline uint32_t psrlw(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x7000003e | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t psubb(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000248 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __psubh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000148 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __psubsb__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000648 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __psubsh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000548 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __psubsw__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000448 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __psubub__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000668 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __psubuh__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000568 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __psubuw__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000468 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t psubw(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x70000048 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t pxor(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700004c9 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t qfsrv(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x700006e8 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t __cfc2__(RegisterID rt, RegisterID rd)
    {
        return (0x48400000 | (rt << OP_SH_RT) | (rd << OP_SH_RD));
    }

    inline uint32_t __cfc2ni__(RegisterID rt, RegisterID rd)
    {
        return (0x48400000 | (rt << OP_SH_RT) | (rd << OP_SH_RD));
    }

    inline uint32_t __cfc2i__(RegisterID rt, RegisterID rd)
    {
        return (0x48400001 | (rt << OP_SH_RT) | (rd << OP_SH_RD));
    }

    inline uint32_t __ctc2__(RegisterID rt, RegisterID rd)
    {
        return (0x48c00000 | (rt << OP_SH_RT) | (rd << OP_SH_RD));
    }

    inline uint32_t __ctc2ni__(RegisterID rt, RegisterID rd)
    {
        return (0x48c00000 | (rt << OP_SH_RT) | (rd << OP_SH_RD));
    }

    inline uint32_t __ctc2i__(RegisterID rt, RegisterID rd)
    {
        return (0x48c00001 | (rt << OP_SH_RT) | (rd << OP_SH_RD));
    }

    inline uint32_t __lqc2__(FPRegisterID ft, int32_t offset)
    {
        return (0xd8000000 | (ft << OP_SH_FT) | (offset & 0xffff));
    }

    inline uint32_t __qmfc2__(RegisterID rt, FPRegisterID fs)
    {
        return (0x48200000 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
    }

    inline uint32_t __qmfc2ni__(RegisterID rt, FPRegisterID fs)
    {
        return (0x48200000 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
    }

    inline uint32_t __qmfc2i__(RegisterID rt, FPRegisterID fs)
    {
        return (0x48200001 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
    }

    inline uint32_t __qmtc2__(RegisterID rt, FPRegisterID fs)
    {
        return (0x48a00000 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
    }

    inline uint32_t __qmtc2ni__(RegisterID rt, FPRegisterID fs)
    {
        return (0x48a00000 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
    }

    inline uint32_t __qmtc2i__(RegisterID rt, FPRegisterID fs)
    {
        return (0x48a00001 | (rt << OP_SH_RT) | (fs << OP_SH_FS));
    }

    inline uint32_t __sqc2__(FPRegisterID ft, int32_t offset)
    {
        return (0xf8000000 | (ft << OP_SH_FT) | (offset & 0xffff));
    }

    inline uint32_t cles(FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000036 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t clts(FPRegisterID fs, FPRegisterID ft)
    {
        return (0x46000034 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
    }

    inline uint32_t add(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x00000020 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t addu(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x00000021 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t _and(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x00000024 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t ___break__()
    {
        return (0x0000000d);
    }

    inline uint32_t __dadd__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x0000002c | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t daddu(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x0000002d | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t div(RegisterID rs, RegisterID rt)
    {
        return (0x0000001a | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t divu(RegisterID rs, RegisterID rt)
    {
        return (0x0000001b | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t dsll(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x00000038 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t dsll32(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x0000003c | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t dsllv(RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return (0x00000014 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
    }

    inline uint32_t dsra(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x0000003b | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t dsra32(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x0000003f | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t __dsrav__(RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return (0x00000017 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
    }

    inline uint32_t dsrl(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x0000003a | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t dsrl32(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x0000003e | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t dsrlv(RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return (0x00000016 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
    }

    inline uint32_t __dsub__(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x0000002e | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t dsubu(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x0000002f | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t jalr(RegisterID rs)
    {
        return (0x0000f809 | (rs << OP_SH_RS));
    }

    inline uint32_t jr(RegisterID rs)
    {
        return (0x00000008 | (rs << OP_SH_RS));
    }

    inline uint32_t mfhi(RegisterID rd)
    {
        return (0x00000010 | (rd << OP_SH_RD));
    }

    inline uint32_t mflo(RegisterID rd)
    {
        return (0x00000012 | (rd << OP_SH_RD));
    }

    inline uint32_t movn(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x0000000b | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t movz(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x0000000a | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t mthi(RegisterID rs)
    {
        return (0x00000011 | (rs << OP_SH_RS));
    }

    inline uint32_t mtlo(RegisterID rs)
    {
        return (0x00000013 | (rs << OP_SH_RS));
    }

    inline uint32_t nor(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x00000027 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t _or(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x00000025 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t sll(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0000000000 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t sllv(RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return (0x00000004 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
    }

    inline uint32_t slt(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x0000002a | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t sltu(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x0000002b | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t sra(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x00000003 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t srav(RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return (0x00000007 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
    }

    inline uint32_t srl(RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return (0x00000002 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    inline uint32_t srlv(RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return (0x00000006 | (rd << OP_SH_RD) | (rt << OP_SH_RT) | (rs << OP_SH_RS));
    }

    inline uint32_t sub(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x00000022 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t subu(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x00000023 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    inline uint32_t _xor(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return (0x00000026 | (rd << OP_SH_RD) | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    template<typename T>
    inline uint32_t addi(RegisterID rt, RegisterID rs, T imm)
    {
        return (0x20000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    template<typename T>
    inline uint32_t addiu(RegisterID rt, RegisterID rs, T imm)
    {
        return (0x24000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    template<typename T>
    inline uint32_t andi(RegisterID rt, RegisterID rs, T imm)
    {
        return (0x30000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    inline uint32_t __cache__(RegisterID rs, int32_t offset)
    {
        return (0x0000002f | (rs << OP_SH_CODE) | (offset & 0xffff));
    }

    template<typename T>
    inline uint32_t daddi(RegisterID rt, RegisterID rs, T imm)
    {
        return (0x60000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    template<typename T>
    inline uint32_t daddiu(RegisterID rt, RegisterID rs, T imm)
    {
        return (0x64000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    inline uint32_t lb(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0x80000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t lbu(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0x90000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t ld(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0xDC000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t ldl(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0x68000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t ldr(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0x6C000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t lh(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0x84000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t lhu(RegisterID rt, int32_t offset)
    {
        return (0x00000025 | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t lhu(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0x94000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t __lq__(RegisterID rt, int32_t offset)
    {
        return (0x0000001e | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    template<typename T>
    inline uint32_t lui(RegisterID rt, T imm)
    {
        return (0x3c000000 | (rt << OP_SH_RT) | (*(int32_t*)&imm & 0xffff));
    }

    inline uint32_t __ll__(RegisterID rt, int32_t offset)
    {
        return (0x00000030 | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t lw(RegisterID rt, int32_t offset)
    {
        return (0x00000023 | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t lw(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0x8c000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t lwc1(FPRegisterID ft, int32_t offset)
    {
        return (0x00000031 | (ft << OP_SH_FT) | (offset & 0xffff));
    }

    inline uint32_t lwl(RegisterID rt, int32_t offset)
    {
        return (0x00000022 | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t lwl(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0x88000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t lwr(RegisterID rt, int32_t offset)
    {
        return (0x00000026 | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t lwr(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0x98000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t lwu(RegisterID rt, int32_t offset)
    {
        return (0x00000027 | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t lwu(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0x9C000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    template<typename T>
    inline uint32_t ori(RegisterID rt, RegisterID rs, T imm)
    {
        return (0x34000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    inline uint32_t __pref__(RegisterID rs, int32_t offset)
    {
        return (0x00000033 | (rs << OP_SH_CODE) | (offset & 0xffff));
    }

    inline uint32_t sb(RegisterID rt, int32_t offset)
    {
        return (0x00000028 | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t __sc__(RegisterID rt, int32_t offset)
    {
        return (0x00000038 | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t __sd__(RegisterID rt, int32_t offset)
    {
        return (0x0000003f | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t sq(RegisterID rt, int32_t offset)
    {
        return (0x0000001f | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t sq(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0x7C000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t __sdl__(RegisterID rt, int32_t offset)
    {
        return (0x0000002c | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t __sdl__(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0xB3000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t __sdr__(RegisterID rt, int32_t offset)
    {
        return (0x0000002d | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t __sdr__(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0xB7000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t sh(RegisterID rt, int32_t offset)
    {
        return (0x00000029 | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    template<typename T>
    inline uint32_t slti(RegisterID rt, RegisterID rs, T imm)
    {
        return (0x28000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    template<typename T>
    inline uint32_t sltiu(RegisterID rt, RegisterID rs, T imm)
    {
        return (0x2c000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    inline uint32_t sw(RegisterID rt, int32_t offset)
    {
        return (0x0000002b | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t sb(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0xa0000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t sh(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0xa4000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t sw(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0xac000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t __swl__(RegisterID rt, int32_t offset)
    {
        return (0x0000002a | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t __swl__(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0xa9000000 | (rt << OP_SH_FT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t __swr__(RegisterID rt, int32_t offset)
    {
        return (0x0000002e | (rt << OP_SH_RT) | (offset & 0xffff));
    }

    inline uint32_t __swr__(RegisterID rt, RegisterID rs, int32_t offset)
    {
        return (0xB9000000 | (rt << OP_SH_FT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t swc1(FPRegisterID ft, int32_t offset)
    {
        return (0x00000039 | (ft << OP_SH_FT) | (offset & 0xffff));
    }

    inline uint32_t lwc1(FPRegisterID ft, RegisterID rs, int32_t offset)
    {
        return (0xc4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t __ldc1__(FPRegisterID ft, RegisterID rs, int32_t offset)
    {
        return (0xd4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t swc1(FPRegisterID ft, RegisterID rs, int32_t offset)
    {
        return (0xe4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    inline uint32_t __sdc1__(FPRegisterID ft, RegisterID rs, int32_t offset)
    {
        return (0xf4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS) | (offset & 0xffff));
    }

    template<typename T>
    inline uint32_t xori(RegisterID rt, RegisterID rs, T imm)
    {
        return (0x38000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS) | (*(int32_t*)&imm & 0xffff));
    }

    inline uint32_t j(int32_t address)
    {
        return ((0x08000000 | ((address & 0x0fffffff) >> 2)));
    }

    inline uint32_t jal(int32_t address)
    {
        return ((0x0C000000 | ((address & 0x0fffffff) >> 2)));
    }

    inline uint32_t b(int32_t imm)
    {
        return (0x10000000 | (imm & 0xffff));
    }

    inline uint32_t beq(RegisterID rs, RegisterID rt, int32_t imm)
    {
        return (0x10000000 | (rs << OP_SH_RS) | (rt << OP_SH_RT) | (imm & 0xffff));
    }

    inline uint32_t __bltz__(RegisterID rs, int32_t imm)
    {
        return (0x04000000 | (rs << OP_SH_RS) | (imm & 0xffff));
    }

    inline uint32_t bne(RegisterID rs, RegisterID rt, int32_t imm)
    {
        return (0x14000000 | (rs << OP_SH_RS) | (rt << OP_SH_RT) | (imm & 0xffff));
    }

    inline uint32_t __bnel__(RegisterID rs, RegisterID rt, int32_t imm)
    {
        return (0x54000000 | (rs << OP_SH_RS) | (rt << OP_SH_RT) | (imm & 0xffff));
    }

    inline uint32_t move(RegisterID rd, RegisterID rs)
    {
        /* addu */
        return (0x00000021 | (rd << OP_SH_RD) | (rs << OP_SH_RS));
    }

    inline uint32_t moveq(RegisterID rd, RegisterID rs)
    {
        return (0x700004a9 | (rd << OP_SH_RD) | (rs << OP_SH_RS));
    }

    ///////////////////////////////////////////////////
    inline std::ostream& write_directly(std::ostream& os, uint32_t data)
    {
        return write_binary(os, write_directly(data));
    }

    inline std::ostream& nop(std::ostream& os)
    {
        return write_binary(os, nop());
    }

    inline std::ostream& __mfbpc__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mfbpc__(rt));
    }

    inline std::ostream& __mfdab__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mfdab__(rt));
    }

    inline std::ostream& __mfdabm__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mfdabm__(rt));
    }

    inline std::ostream& __mfdvb__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mfdvb__(rt));
    }

    inline std::ostream& __mfdvbm__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mfdvbm__(rt));
    }

    inline std::ostream& __mfiab__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mfiab__(rt));
    }

    inline std::ostream& __mfiabm__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mfiabm__(rt));
    }

    inline std::ostream& __mfpc__(std::ostream& os, RegisterID rt, int32_t i)
    {
        return write_binary(os, __mfpc__(rt, i));
    }

    inline std::ostream& __mfps__(std::ostream& os, RegisterID rt, int32_t i)
    {
        return write_binary(os, __mfps__(rt, i));
    }

    inline std::ostream& __mtbpc__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mtbpc__(rt));
    }

    inline std::ostream& __mtdab__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mtdab__(rt));
    }

    inline std::ostream& __mtdabm__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mtdabm__(rt));
    }

    inline std::ostream& __mtdvb__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mtdvb__(rt));
    }

    inline std::ostream& __mtdvbm__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mtdvbm__(rt));
    }

    inline std::ostream& __mtpc__(std::ostream& os, RegisterID rt, int32_t i)
    {
        return write_binary(os, __mtpc__(rt, i));
    }

    inline std::ostream& __mtps__(std::ostream& os, RegisterID rt, int32_t i)
    {
        return write_binary(os, __mtps__(rt, i));
    }

    inline std::ostream& bc0f(std::ostream& os, int32_t offset)
    {
        return write_binary(os, bc0f(offset));
    }

    inline std::ostream& __bc0fl__(std::ostream& os, int32_t offset)
    {
        return write_binary(os, __bc0fl__(offset));
    }

    inline std::ostream& __bc0t__(std::ostream& os, int32_t offset)
    {
        return write_binary(os, __bc0t__(offset));
    }

    inline std::ostream& __bc0tl__(std::ostream& os, int32_t offset)
    {
        return write_binary(os, __bc0tl__(offset));
    }

    inline std::ostream& __bc2f__(std::ostream& os, int32_t offset)
    {
        return write_binary(os, __bc2f__(offset));
    }

    inline std::ostream& __bc2fl__(std::ostream& os, int32_t offset)
    {
        return write_binary(os, __bc2fl__(offset));
    }

    inline std::ostream& __bc2t__(std::ostream& os, int32_t offset)
    {
        return write_binary(os, __bc2t__(offset));
    }

    inline std::ostream& __bc2tl__(std::ostream& os, int32_t offset)
    {
        return write_binary(os, __bc2tl__(offset));
    }

    inline std::ostream& di(std::ostream& os)
    {
        return write_binary(os, di());
    }

    inline std::ostream& div1(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, div1(rs, rt));
    }

    inline std::ostream& __divu1__(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __divu1__(rs, rt));
    }

    inline std::ostream& ei(std::ostream& os)
    {
        return write_binary(os, ei());
    }

    inline std::ostream& eret(std::ostream& os)
    {
        return write_binary(os, eret());
    }

    inline std::ostream& madd(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, madd(rs, rt));
    }

    inline std::ostream& madd(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, madd(rd, rs, rt));
    }

    inline std::ostream& __madd1__(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __madd1__(rs, rt));
    }

    inline std::ostream& __madd1__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __madd1__(rd, rs, rt));
    }

    inline std::ostream& maddu(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, maddu(rs, rt));
    }

    inline std::ostream& maddu(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, maddu(rd, rs, rt));
    }

    inline std::ostream& __maddu1__(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __maddu1__(rs, rt));
    }

    inline std::ostream& __maddu1__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __maddu1__(rd, rs, rt));
    }

    inline std::ostream& __mfc0__(std::ostream& os, RegisterID rt, RegisterID rd)
    {
        return write_binary(os, __mfc0__(rt, rd));
    }

    inline std::ostream& mfhi1(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, mfhi1(rd));
    }

    inline std::ostream& mflo1(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, mflo1(rd));
    }

    inline std::ostream& mfsa(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, mfsa(rd));
    }

    inline std::ostream& __mtc0__(std::ostream& os, RegisterID rt, RegisterID rd)
    {
        return write_binary(os, __mtc0__(rt, rd));
    }

    inline std::ostream& __mtiab__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mtiab__(rt));
    }

    inline std::ostream& __mtiabm__(std::ostream& os, RegisterID rt)
    {
        return write_binary(os, __mtiabm__(rt));
    }

    inline std::ostream& mthi1(std::ostream& os, RegisterID rs)
    {
        return write_binary(os, mthi1(rs));
    }

    inline std::ostream& mtlo1(std::ostream& os, RegisterID rs)
    {
        return write_binary(os, mtlo1(rs));
    }

    inline std::ostream& mtsa(std::ostream& os, RegisterID rs)
    {
        return write_binary(os, mtsa(rs));
    }

    template<typename T>
    inline std::ostream& mtsab(std::ostream& os, RegisterID rs, T imm)
    {
        return write_binary(os, mtsab(rs, imm));
    }

    template<typename T>
    inline std::ostream& __mtsah__(std::ostream& os, RegisterID rs, T imm)
    {
        return write_binary(os, __mtsah__(rs, imm));
    }

    inline std::ostream& mult(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, mult(rs, rt));
    }

    inline std::ostream& mult(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, mult(rd, rs, rt));
    }

    inline std::ostream& mult1(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, mult1(rs, rt));
    }

    inline std::ostream& mult1(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, mult1(rd, rs, rt));
    }

    inline std::ostream& multu(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, multu(rs, rt));
    }

    inline std::ostream& multu(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, multu(rd, rs, rt));
    }

    inline std::ostream& multu1(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, multu1(rs, rt));
    }

    inline std::ostream& multu1(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, multu1(rd, rs, rt));
    }

    inline std::ostream& __syncl__(std::ostream& os)
    {
        return write_binary(os, __syncl__());
    }

    inline std::ostream& sync(std::ostream& os)
    {
        return write_binary(os, sync());
    }

    inline std::ostream& __syncp__(std::ostream& os)
    {
        return write_binary(os, __syncp__());
    }

    inline std::ostream& syscall(std::ostream& os)
    {
        return write_binary(os, syscall());
    }

    inline std::ostream& __teq__(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __teq__(rs, rt));
    }

    template<typename T>
    inline std::ostream& __teqi__(std::ostream& os, RegisterID rs, T imm)
    {
        return write_binary(os, __teqi__(rs, imm));
    }

    inline std::ostream& tge(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, tge(rs, rt));
    }

    template<typename T>
    inline std::ostream& __tgei__(std::ostream& os, RegisterID rs, T imm)
    {
        return write_binary(os, __tgei__(rs, imm));
    }

    template<typename T>
    inline std::ostream& __tgeiu__(std::ostream& os, RegisterID rs, T imm)
    {
        return write_binary(os, __tgeiu__(rs, imm));
    }

    inline std::ostream& __tgeu__(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __tgeu__(rs, rt));
    }

    inline std::ostream& __tlt__(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __tlt__(rs, rt));
    }

    template<typename T>
    inline std::ostream& __tlti__(std::ostream& os, RegisterID rs, T imm)
    {
        return write_binary(os, __tlti__(rs, imm));
    }

    template<typename T>
    inline std::ostream& __tltiu__(std::ostream& os, RegisterID rs, T imm)
    {
        return write_binary(os, __tltiu__(rs, imm));
    }

    inline std::ostream& __tltu__(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __tltu__(rs, rt));
    }

    inline std::ostream& __tne__(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __tne__(rs, rt));
    }

    template<typename T>
    inline std::ostream& __tnei__(std::ostream& os, RegisterID rs, T imm)
    {
        return write_binary(os, __tnei__(rs, imm));
    }

    inline std::ostream& __tlbp__(std::ostream& os)
    {
        return write_binary(os, __tlbp__());
    }

    inline std::ostream& __tlbr__(std::ostream& os)
    {
        return write_binary(os, __tlbr__());
    }

    inline std::ostream& tlbwi(std::ostream& os)
    {
        return write_binary(os, tlbwi());
    }

    inline std::ostream& __tlbwr__(std::ostream& os)
    {
        return write_binary(os, __tlbwr__());
    }

    inline std::ostream& abss(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        return write_binary(os, abss(fd, fs));
    }

    inline std::ostream& __absd__(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        return write_binary(os, __absd__(fd, fs));
    }

    inline std::ostream& adds(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, adds(fd, fs, ft));
    }

    inline std::ostream& __addd__(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, __addd__(fd, fs, ft));
    }

    inline std::ostream& addas(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, addas(fs, ft));
    }

    inline std::ostream& bc1f(std::ostream& os, int32_t offset)
    {
        return write_binary(os, bc1f(offset));
    }

    inline std::ostream& bc1fl(std::ostream& os, int32_t offset)
    {
        return write_binary(os, bc1fl(offset));
    }

    inline std::ostream& __bc1t__(std::ostream& os, int32_t offset)
    {
        return write_binary(os, __bc1t__(offset));
    }

    inline std::ostream& bc1tl(std::ostream& os, int32_t offset)
    {
        return write_binary(os, bc1tl(offset));
    }

    inline std::ostream& ceqs(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, ceqs(fs, ft));
    }

    inline std::ostream& __cfs__(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, __cfs__(fs, ft));
    }

    inline std::ostream& __cfc1__(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        return write_binary(os, __cfc1__(rt, fs));
    }

    inline std::ostream& __ctc1__(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        return write_binary(os, __ctc1__(rt, fs));
    }

    inline std::ostream& cvtsw(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        return write_binary(os, cvtsw(fd, fs));
    }

    inline std::ostream& cvtws(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        return write_binary(os, cvtws(fd, fs));
    }

    inline std::ostream& divs(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, divs(fd, fs, ft));
    }

    inline std::ostream& madds(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, madds(fd, fs, ft));
    }

    inline std::ostream& maddas(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, maddas(fs, ft));
    }

    inline std::ostream& maxs(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, maxs(fd, fs, ft));
    }

    inline std::ostream& mfc1(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        return write_binary(os, mfc1(rt, fs));
    }

    inline std::ostream& mins(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, mins(fd, fs, ft));
    }

    inline std::ostream& movs(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        return write_binary(os, movs(fd, fs));
    }

    inline std::ostream& msubs(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, msubs(fd, fs, ft));
    }

    inline std::ostream& msubas(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, msubas(fs, ft));
    }

    inline std::ostream& mtc1(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        return write_binary(os, mtc1(rt, fs));
    }

    inline std::ostream& muls(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, muls(fd, fs, ft));
    }

    inline std::ostream& mulas(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, mulas(fs, ft));
    }

    inline std::ostream& negs(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        return write_binary(os, negs(fd, fs));
    }

    inline std::ostream& rsqrts(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, rsqrts(fd, fs, ft));
    }

    inline std::ostream& sqrts(std::ostream& os, FPRegisterID fd, FPRegisterID ft)
    {
        return write_binary(os, sqrts(fd, ft));
    }

    inline std::ostream& subs(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, subs(fd, fs, ft));
    }

    inline std::ostream& __subd__(std::ostream& os, FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, __subd__(fd, fs, ft));
    }

    inline std::ostream& __subas__(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, __subas__(fs, ft));
    }

    inline std::ostream& __sqrtd__(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        return write_binary(os, __sqrtd__(fd, fs));
    }

    inline std::ostream& __rsqrtd__(std::ostream& os, FPRegisterID fd, FPRegisterID fs)
    {
        return write_binary(os, __rsqrtd__(fd, fs));
    }

    inline std::ostream& __pabsh__(std::ostream& os, RegisterID rd, RegisterID rt)
    {
        return write_binary(os, __pabsh__(rd, rt));
    }

    inline std::ostream& __pabsw__(std::ostream& os, RegisterID rd, RegisterID rt)
    {
        return write_binary(os, __pabsw__(rd, rt));
    }

    inline std::ostream& __paddb__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __paddb__(rd, rs, rt));
    }

    inline std::ostream& paddh(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, paddh(rd, rs, rt));
    }

    inline std::ostream& __paddsb__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __paddsb__(rd, rs, rt));
    }

    inline std::ostream& __paddsh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __paddsh__(rd, rs, rt));
    }

    inline std::ostream& paddsw(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, paddsw(rd, rs, rt));
    }

    inline std::ostream& __paddub__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __paddub__(rd, rs, rt));
    }

    inline std::ostream& __padduh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __padduh__(rd, rs, rt));
    }

    inline std::ostream& __padduw__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __padduw__(rd, rs, rt));
    }

    inline std::ostream& paddw(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, paddw(rd, rs, rt));
    }

    inline std::ostream& __padsbh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __padsbh__(rd, rs, rt));
    }

    inline std::ostream& pand(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pand(rd, rs, rt));
    }

    inline std::ostream& __pceqb__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pceqb__(rd, rs, rt));
    }

    inline std::ostream& __pceqh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pceqh__(rd, rs, rt));
    }

    inline std::ostream& __pceqw__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pceqw__(rd, rs, rt));
    }

    inline std::ostream& __pcgtb__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pcgtb__(rd, rs, rt));
    }

    inline std::ostream& pcgth(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pcgth(rd, rs, rt));
    }

    inline std::ostream& __pcgtw__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pcgtw__(rd, rs, rt));
    }

    inline std::ostream& pcpyh(std::ostream& os, RegisterID rd, RegisterID rt)
    {
        return write_binary(os, pcpyh(rd, rt));
    }

    inline std::ostream& pcpyld(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pcpyld(rd, rs, rt));
    }

    inline std::ostream& pcpyud(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pcpyud(rd, rs, rt));
    }

    inline std::ostream& __pdivbw__(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pdivbw__(rs, rt));
    }

    inline std::ostream& __pdivuw__(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pdivuw__(rs, rt));
    }

    inline std::ostream& __pdivw__(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pdivw__(rs, rt));
    }

    inline std::ostream& pexch(std::ostream& os, RegisterID rd, RegisterID rt)
    {
        return write_binary(os, pexch(rd, rt));
    }

    inline std::ostream& __pexeh__(std::ostream& os, RegisterID rd, RegisterID rt)
    {
        return write_binary(os, __pexeh__(rd, rt));
    }

    inline std::ostream& pexew(std::ostream& os, RegisterID rd, RegisterID rt)
    {
        return write_binary(os, pexew(rd, rt));
    }

    inline std::ostream& __pext5__(std::ostream& os, RegisterID rd, RegisterID rt)
    {
        return write_binary(os, __pext5__(rd, rt));
    }

    inline std::ostream& pextlb(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pextlb(rd, rs, rt));
    }

    inline std::ostream& pextlh(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pextlh(rd, rs, rt));
    }

    inline std::ostream& pextlw(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pextlw(rd, rs, rt));
    }

    inline std::ostream& pextub(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pextub(rd, rs, rt));
    }

    inline std::ostream& __pextuh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pextuh__(rd, rs, rt));
    }

    inline std::ostream& pextuw(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pextuw(rd, rs, rt));
    }

    inline std::ostream& __phmadh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __phmadh__(rd, rs, rt));
    }

    inline std::ostream& __phmsbh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __phmsbh__(rd, rs, rt));
    }

    inline std::ostream& __pinteh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pinteh__(rd, rs, rt));
    }

    inline std::ostream& __pinth__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pinth__(rd, rs, rt));
    }

    inline std::ostream& __plzcw__(std::ostream& os, RegisterID rd, RegisterID rs)
    {
        return write_binary(os, __plzcw__(rd, rs));
    }

    inline std::ostream& __pmaddh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pmaddh__(rd, rs, rt));
    }

    inline std::ostream& __pmadduw__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pmadduw__(rd, rs, rt));
    }

    inline std::ostream& __pmaddw__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pmaddw__(rd, rs, rt));
    }

    inline std::ostream& pmaxh(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pmaxh(rd, rs, rt));
    }

    inline std::ostream& pmaxw(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pmaxw(rd, rs, rt));
    }

    inline std::ostream& pmfhi(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, pmfhi(rd));
    }

    inline std::ostream& __pmfhllh__(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, __pmfhllh__(rd));
    }

    inline std::ostream& __pmfhllw__(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, __pmfhllw__(rd));
    }

    inline std::ostream& __pmfhlsh__(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, __pmfhlsh__(rd));
    }

    inline std::ostream& __pmfhlslw__(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, __pmfhlslw__(rd));
    }

    inline std::ostream& __pmfhluw__(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, __pmfhluw__(rd));
    }

    inline std::ostream& pmflo(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, pmflo(rd));
    }

    inline std::ostream& pminh(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pminh(rd, rs, rt));
    }

    inline std::ostream& pminw(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pminw(rd, rs, rt));
    }

    inline std::ostream& __pmsubh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pmsubh__(rd, rs, rt));
    }

    inline std::ostream& __pmsubw__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pmsubw__(rd, rs, rt));
    }

    inline std::ostream& __pmthi__(std::ostream& os, RegisterID rs)
    {
        return write_binary(os, __pmthi__(rs));
    }

    inline std::ostream& __pmthllw__(std::ostream& os, RegisterID rs)
    {
        return write_binary(os, __pmthllw__(rs));
    }

    inline std::ostream& __pmtlo__(std::ostream& os, RegisterID rs)
    {
        return write_binary(os, __pmtlo__(rs));
    }

    inline std::ostream& pmulth(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pmulth(rd, rs, rt));
    }

    inline std::ostream& __pmultuw__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pmultuw__(rd, rs, rt));
    }

    inline std::ostream& __pmultw__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __pmultw__(rd, rs, rt));
    }

    inline std::ostream& pnor(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pnor(rd, rs, rt));
    }

    inline std::ostream& por(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, por(rd, rs, rt));
    }

    inline std::ostream& __ppac5__(std::ostream& os, RegisterID rd, RegisterID rt)
    {
        return write_binary(os, __ppac5__(rd, rt));
    }

    inline std::ostream& ppacb(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, ppacb(rd, rs, rt));
    }

    inline std::ostream& ppach(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, ppach(rd, rs, rt));
    }

    inline std::ostream& __ppacw__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __ppacw__(rd, rs, rt));
    }

    inline std::ostream& __prevh__(std::ostream& os, RegisterID rd, RegisterID rt)
    {
        return write_binary(os, __prevh__(rd, rt));
    }

    inline std::ostream& prot3w(std::ostream& os, RegisterID rd, RegisterID rt)
    {
        return write_binary(os, prot3w(rd, rt));
    }

    inline std::ostream& psllh(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, psllh(rd, rt, shamt));
    }

    inline std::ostream& __psllvw__(std::ostream& os, RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return write_binary(os, __psllvw__(rd, rt, rs));
    }

    inline std::ostream& psllw(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, psllw(rd, rt, shamt));
    }

    inline std::ostream& psrah(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, psrah(rd, rt, shamt));
    }

    inline std::ostream& __psravw__(std::ostream& os, RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return write_binary(os, __psravw__(rd, rt, rs));
    }

    inline std::ostream& psraw(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, psraw(rd, rt, shamt));
    }

    inline std::ostream& psrlh(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, psrlh(rd, rt, shamt));
    }

    inline std::ostream& __psrlvw__(std::ostream& os, RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return write_binary(os, __psrlvw__(rd, rt, rs));
    }

    inline std::ostream& psrlw(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, psrlw(rd, rt, shamt));
    }

    inline std::ostream& psubb(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, psubb(rd, rs, rt));
    }

    inline std::ostream& __psubh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __psubh__(rd, rs, rt));
    }

    inline std::ostream& __psubsb__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __psubsb__(rd, rs, rt));
    }

    inline std::ostream& __psubsh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __psubsh__(rd, rs, rt));
    }

    inline std::ostream& __psubsw__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __psubsw__(rd, rs, rt));
    }

    inline std::ostream& __psubub__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __psubub__(rd, rs, rt));
    }

    inline std::ostream& __psubuh__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __psubuh__(rd, rs, rt));
    }

    inline std::ostream& __psubuw__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __psubuw__(rd, rs, rt));
    }

    inline std::ostream& psubw(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, psubw(rd, rs, rt));
    }

    inline std::ostream& pxor(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, pxor(rd, rs, rt));
    }

    inline std::ostream& qfsrv(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, qfsrv(rd, rs, rt));
    }

    inline std::ostream& __cfc2__(std::ostream& os, RegisterID rt, RegisterID rd)
    {
        return write_binary(os, __cfc2__(rt, rd));
    }

    inline std::ostream& __cfc2ni__(std::ostream& os, RegisterID rt, RegisterID rd)
    {
        return write_binary(os, __cfc2ni__(rt, rd));
    }

    inline std::ostream& __cfc2i__(std::ostream& os, RegisterID rt, RegisterID rd)
    {
        return write_binary(os, __cfc2i__(rt, rd));
    }

    inline std::ostream& __ctc2__(std::ostream& os, RegisterID rt, RegisterID rd)
    {
        return write_binary(os, __ctc2__(rt, rd));
    }

    inline std::ostream& __ctc2ni__(std::ostream& os, RegisterID rt, RegisterID rd)
    {
        return write_binary(os, __ctc2ni__(rt, rd));
    }

    inline std::ostream& __ctc2i__(std::ostream& os, RegisterID rt, RegisterID rd)
    {
        return write_binary(os, __ctc2i__(rt, rd));
    }

    inline std::ostream& __lqc2__(std::ostream& os, FPRegisterID ft, int32_t offset)
    {
        return write_binary(os, __lqc2__(ft, offset));
    }

    inline std::ostream& __qmfc2__(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        return write_binary(os, __qmfc2__(rt, fs));
    }

    inline std::ostream& __qmfc2ni__(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        return write_binary(os, __qmfc2ni__(rt, fs));
    }

    inline std::ostream& __qmfc2i__(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        return write_binary(os, __qmfc2i__(rt, fs));
    }

    inline std::ostream& __qmtc2__(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        return write_binary(os, __qmtc2__(rt, fs));
    }

    inline std::ostream& __qmtc2ni__(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        return write_binary(os, __qmtc2ni__(rt, fs));
    }

    inline std::ostream& __qmtc2i__(std::ostream& os, RegisterID rt, FPRegisterID fs)
    {
        return write_binary(os, __qmtc2i__(rt, fs));
    }

    inline std::ostream& __sqc2__(std::ostream& os, FPRegisterID ft, int32_t offset)
    {
        return write_binary(os, __sqc2__(ft, offset));
    }

    inline std::ostream& cles(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, cles(fs, ft));
    }

    inline std::ostream& clts(std::ostream& os, FPRegisterID fs, FPRegisterID ft)
    {
        return write_binary(os, clts(fs, ft));
    }

    inline std::ostream& add(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, add(rd, rs, rt));
    }

    inline std::ostream& addu(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, addu(rd, rs, rt));
    }

    inline std::ostream& _and(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, _and(rd, rs, rt));
    }

    inline std::ostream& ___break__(std::ostream& os)
    {
        return write_binary(os, ___break__());
    }

    inline std::ostream& __dadd__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __dadd__(rd, rs, rt));
    }

    inline std::ostream& daddu(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, daddu(rd, rs, rt));
    }

    inline std::ostream& div(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, div(rs, rt));
    }

    inline std::ostream& divu(std::ostream& os, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, divu(rs, rt));
    }

    inline std::ostream& dsll(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, dsll(rd, rt, shamt));
    }

    inline std::ostream& dsll32(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, dsll32(rd, rt, shamt));
    }

    inline std::ostream& dsllv(std::ostream& os, RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return write_binary(os, dsllv(rd, rt, rs));
    }

    inline std::ostream& dsra(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, dsra(rd, rt, shamt));
    }

    inline std::ostream& dsra32(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, dsra32(rd, rt, shamt));
    }

    inline std::ostream& __dsrav__(std::ostream& os, RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return write_binary(os, __dsrav__(rd, rt, rs));
    }

    inline std::ostream& dsrl(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, dsrl(rd, rt, shamt));
    }

    inline std::ostream& dsrl32(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, dsrl32(rd, rt, shamt));
    }

    inline std::ostream& dsrlv(std::ostream& os, RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return write_binary(os, dsrlv(rd, rt, rs));
    }

    inline std::ostream& __dsub__(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, __dsub__(rd, rs, rt));
    }

    inline std::ostream& dsubu(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, dsubu(rd, rs, rt));
    }

    inline std::ostream& jalr(std::ostream& os, RegisterID rs)
    {
        return write_binary(os, jalr(rs));
    }

    inline std::ostream& jr(std::ostream& os, RegisterID rs)
    {
        return write_binary(os, jr(rs));
    }

    inline std::ostream& mfhi(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, mfhi(rd));
    }

    inline std::ostream& mflo(std::ostream& os, RegisterID rd)
    {
        return write_binary(os, mflo(rd));
    }

    inline std::ostream& movn(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, movn(rd, rs, rt));
    }

    inline std::ostream& movz(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, movz(rd, rs, rt));
    }

    inline std::ostream& mthi(std::ostream& os, RegisterID rs)
    {
        return write_binary(os, mthi(rs));
    }

    inline std::ostream& mtlo(std::ostream& os, RegisterID rs)
    {
        return write_binary(os, mtlo(rs));
    }

    inline std::ostream& nor(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, nor(rd, rs, rt));
    }

    inline std::ostream& _or(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, _or(rd, rs, rt));
    }

    inline std::ostream& sll(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, sll(rd, rt, shamt));
    }

    inline std::ostream& sllv(std::ostream& os, RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return write_binary(os, sllv(rd, rt, rs));
    }

    inline std::ostream& slt(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, slt(rd, rs, rt));
    }

    inline std::ostream& sltu(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, sltu(rd, rs, rt));
    }

    inline std::ostream& sra(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, sra(rd, rt, shamt));
    }

    inline std::ostream& srav(std::ostream& os, RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return write_binary(os, srav(rd, rt, rs));
    }

    inline std::ostream& srl(std::ostream& os, RegisterID rd, RegisterID rt, int32_t shamt)
    {
        return write_binary(os, srl(rd, rt, shamt));
    }

    inline std::ostream& srlv(std::ostream& os, RegisterID rd, RegisterID rt, RegisterID rs)
    {
        return write_binary(os, srlv(rd, rt, rs));
    }

    inline std::ostream& sub(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, sub(rd, rs, rt));
    }

    inline std::ostream& subu(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, subu(rd, rs, rt));
    }

    inline std::ostream& _xor(std::ostream& os, RegisterID rd, RegisterID rs, RegisterID rt)
    {
        return write_binary(os, _xor(rd, rs, rt));
    }

    template<typename T>
    inline std::ostream& addi(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        return write_binary(os, addi(rt, rs, imm));
    }

    template<typename T>
    inline std::ostream& addiu(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        return write_binary(os, addiu(rt, rs, imm));
    }

    template<typename T>
    inline std::ostream& andi(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        return write_binary(os, andi(rt, rs, imm));
    }

    inline std::ostream& __cache__(std::ostream& os, RegisterID rs, int32_t offset)
    {
        return write_binary(os, __cache__(rs, offset));
    }

    template<typename T>
    inline std::ostream& daddi(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        return write_binary(os, daddi(rt, rs, imm));
    }

    template<typename T>
    inline std::ostream& daddiu(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        return write_binary(os, daddiu(rt, rs, imm));
    }

    inline std::ostream& lb(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, lb(rt, rs, offset));
    }

    inline std::ostream& lbu(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, lbu(rt, rs, offset));
    }

    inline std::ostream& ld(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, ld(rt, rs, offset));
    }

    inline std::ostream& ldl(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, ldl(rt, rs, offset));
    }

    inline std::ostream& ldr(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, ldr(rt, rs, offset));
    }

    inline std::ostream& lh(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, lh(rt, rs, offset));
    }

    inline std::ostream& lhu(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, lhu(rt, offset));
    }

    inline std::ostream& lhu(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, lhu(rt, rs, offset));
    }

    inline std::ostream& __lq__(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, __lq__(rt, offset));
    }

    template<typename T>
    inline std::ostream& lui(std::ostream& os, RegisterID rt, T imm)
    {
        return write_binary(os, lui(rt, imm));
    }

    inline std::ostream& __ll__(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, __ll__(rt, offset));
    }

    inline std::ostream& lw(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, lw(rt, offset));
    }

    inline std::ostream& lw(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, lw(rt, rs, offset));
    }

    inline std::ostream& lwc1(std::ostream& os, FPRegisterID ft, int32_t offset)
    {
        return write_binary(os, lwc1(ft, offset));
    }

    inline std::ostream& lwl(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, lwl(rt, offset));
    }

    inline std::ostream& lwl(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, lwl(rt, rs, offset));
    }

    inline std::ostream& lwr(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, lwr(rt, offset));
    }

    inline std::ostream& lwr(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, lwr(rt, rs, offset));
    }

    inline std::ostream& lwu(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, lwu(rt, offset));
    }

    inline std::ostream& lwu(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, lwu(rt, rs, offset));
    }

    template<typename T>
    inline std::ostream& ori(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        return write_binary(os, ori(rt, rs, imm));
    }

    inline std::ostream& __pref__(std::ostream& os, RegisterID rs, int32_t offset)
    {
        return write_binary(os, __pref__(rs, offset));
    }

    inline std::ostream& sb(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, sb(rt, offset));
    }

    inline std::ostream& __sc__(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, __sc__(rt, offset));
    }

    inline std::ostream& __sd__(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, __sd__(rt, offset));
    }

    inline std::ostream& sq(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, sq(rt, offset));
    }

    inline std::ostream& sq(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, sq(rt, rs, offset));
    }

    inline std::ostream& __sdl__(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, __sdl__(rt, offset));
    }

    inline std::ostream& __sdl__(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, __sdl__(rt, rs, offset));
    }

    inline std::ostream& __sdr__(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, __sdr__(rt, offset));
    }

    inline std::ostream& __sdr__(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, __sdr__(rt, rs, offset));
    }

    inline std::ostream& sh(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, sh(rt, offset));
    }

    template<typename T>
    inline std::ostream& slti(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        return write_binary(os, slti(rt, rs, imm));
    }

    template<typename T>
    inline std::ostream& sltiu(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        return write_binary(os, sltiu(rt, rs, imm));
    }

    inline std::ostream& sw(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, sw(rt, offset));
    }

    inline std::ostream& sb(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, sb(rt, rs, offset));
    }

    inline std::ostream& sh(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, sh(rt, rs, offset));
    }

    inline std::ostream& sw(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, sw(rt, rs, offset));
    }

    inline std::ostream& __swl__(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, __swl__(rt, offset));
    }

    inline std::ostream& __swl__(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, __swl__(rt, rs, offset));
    }

    inline std::ostream& __swr__(std::ostream& os, RegisterID rt, int32_t offset)
    {
        return write_binary(os, __swr__(rt, offset));
    }

    inline std::ostream& __swr__(std::ostream& os, RegisterID rt, RegisterID rs, int32_t offset)
    {
        return write_binary(os, __swr__(rt, rs, offset));
    }

    inline std::ostream& swc1(std::ostream& os, FPRegisterID ft, int32_t offset)
    {
        return write_binary(os, swc1(ft, offset));
    }

    inline std::ostream& lwc1(std::ostream& os, FPRegisterID ft, RegisterID rs, int32_t offset)
    {
        return write_binary(os, lwc1(ft, rs, offset));
    }

    inline std::ostream& __ldc1__(std::ostream& os, FPRegisterID ft, RegisterID rs, int32_t offset)
    {
        return write_binary(os, __ldc1__(ft, rs, offset));
    }

    inline std::ostream& swc1(std::ostream& os, FPRegisterID ft, RegisterID rs, int32_t offset)
    {
        return write_binary(os, swc1(ft, rs, offset));
    }

    inline std::ostream& __sdc1__(std::ostream& os, FPRegisterID ft, RegisterID rs, int32_t offset)
    {
        return write_binary(os, __sdc1__(ft, rs, offset));
    }

    template<typename T>
    inline std::ostream& xori(std::ostream& os, RegisterID rt, RegisterID rs, T imm)
    {
        return write_binary(os, xori(rt, rs, imm));
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

    inline std::ostream& j(std::ostream& os, int32_t address)
    {
        return write_binary(os, j(address));
    }

    inline std::ostream& jal(std::ostream& os, int32_t address)
    {
        return write_binary(os, jal(address));
    }

    inline std::ostream& b(std::ostream& os, int32_t imm)
    {
        return write_binary(os, b(imm));
    }

    inline std::ostream& beq(std::ostream& os, RegisterID rs, RegisterID rt, int32_t imm)
    {
        return write_binary(os, beq(rs, rt, imm));
    }

    inline std::ostream& __bltz__(std::ostream& os, RegisterID rs, int32_t imm)
    {
        return write_binary(os, __bltz__(rs, imm));
    }

    inline std::ostream& bne(std::ostream& os, RegisterID rs, RegisterID rt, int32_t imm)
    {
        return write_binary(os, bne(rs, rt, imm));
    }

    inline std::ostream& __bnel__(std::ostream& os, RegisterID rs, RegisterID rt, int32_t imm)
    {
        return write_binary(os, __bnel__(rs, rt, imm));
    }

    inline std::ostream& move(std::ostream& os, RegisterID rd, RegisterID rs)
    {
        return write_binary(os, move(rd, rs));
    }

    inline std::ostream& moveq(std::ostream& os, RegisterID rd, RegisterID rs)
    {
        return write_binary(os, moveq(rd, rs));
    }

    // Tests
#if _DEBUG
    class TestCase
    {
    public:
        struct MipsTest
        {
            std::string opcode;
            const unsigned char hex_code[4];
            std::function<void(std::ostringstream& buf)> asm_code_sample;
        };

        TestCase()
        {
            runTest();
        }

        void runTest()
        {
            auto checkInstr = [](auto& var)
            {
                static bool bOnce = false;
                std::ostringstream a;
                var.asm_code_sample(a);
                std::ostringstream b;
                b.write(reinterpret_cast<const char*>(var.hex_code), sizeof(var.hex_code));

                if (a.str() != b.str())
                {
                    if (var.opcode == "j" || var.opcode == "b" || var.opcode == "move")
                        return;

                    if (!bOnce)
                    {
                        AllocConsole();
                        freopen("conin$", "r", stdin);
                        freopen("conout$", "w", stdout);
                        freopen("conout$", "w", stderr);
                        printf("Tests Failed On:\n");
                        bOnce = true;
                    }

                    assert(a.str() == b.str());
                    printf(var.opcode.c_str());
                    printf("\n");
                }
            };

            std::vector<MipsTest> testDataMain = {
                { "abss",     { 0x05, 0x01, 0x00, 0x46}, ([](std::ostringstream& buf) { abss(buf,f4,f0);            })},    // abss      f4,f0
                { "add",      { 0x20, 0x10, 0x05, 0x00}, ([](std::ostringstream& buf) { add(buf,v0,zero,a1);        })},    // add       v0,zero,a1
                { "addas",    { 0x18, 0x00, 0x01, 0x46}, ([](std::ostringstream& buf) { addas(buf,f0,f1);           })},    // addas     f0,f1
                { "addi",     { 0xff, 0xff, 0xa5, 0x20}, ([](std::ostringstream& buf) { addi(buf,a1,a1,-0x1);       })},    // addi       a1,a1,-0x1
                { "addiu",    { 0xc0, 0x00, 0xbd, 0x27}, ([](std::ostringstream& buf) { addiu(buf,sp,sp,0xc0);      })},    // addiu      sp,sp,0xc0
                { "adds",     { 0x40, 0xad, 0x14, 0x46}, ([](std::ostringstream& buf) { adds(buf,f21,f21,f20);      })},    // adds      f21,f21,f20
                { "addu",     { 0x21, 0x80, 0x02, 0x02}, ([](std::ostringstream& buf) { addu(buf,s0,s0,v0);         })},    // addu       s0,s0,v0
                { "and",      { 0x24, 0x10, 0x43, 0x00}, ([](std::ostringstream& buf) { _and(buf,v0,v0,v1);         })},    // and        v0,v0,v1
                { "andi",     { 0xff, 0xff, 0x42, 0x30}, ([](std::ostringstream& buf) { andi(buf,v0,v0,0xffff);     })},    // andi       v0,v0,0xffff
                { "b",        { 0x12, 0x00, 0x00, 0x10}, ([](std::ostringstream& buf) { b(buf,0x12);                })},    // b          lab001009b8
                { "bc0f",     { 0xf9, 0xff, 0x00, 0x41}, ([](std::ostringstream& buf) { bc0f(buf,0xf9);             })},    // bc0f       lab002724bc
                { "bc1f",     { 0x0f, 0x00, 0x00, 0x45}, ([](std::ostringstream& buf) { bc1f(buf,0x0f);             })},    // bc1f       lab00100fe4
                { "bc1fl",    { 0x10, 0x00, 0x02, 0x45}, ([](std::ostringstream& buf) { bc1fl(buf,0x10);            })},    // bc1fl      lab00100e68
                { "bc1tl",    { 0x06, 0x00, 0x03, 0x45}, ([](std::ostringstream& buf) { bc1tl(buf,0x06);            })},    // bc1tl      lab00101194
                { "beq",      { 0x0b, 0x00, 0x60, 0x10}, ([](std::ostringstream& buf) { beq(buf,v1,zero,0x0b);      })},    // beq        v1,zero,lab00100974
                { "bne",      { 0x0d, 0x00, 0x60, 0x14}, ([](std::ostringstream& buf) { bne(buf,v1,zero,0x0d);      })},    // bne        v1,zero,lab0010099c
                { "ceqs",     { 0x32, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { ceqs(buf,f1,f0);            })},    // ceqs     f1,f0
                { "cles",     { 0x36, 0x00, 0x01, 0x46}, ([](std::ostringstream& buf) { cles(buf,f0,f1);            })},    // cles     f0,f1
                { "clts",     { 0x34, 0xb0, 0x00, 0x46}, ([](std::ostringstream& buf) { clts(buf,f22,f0);           })},    // clts     f22,f0
                { "cvtsw",    { 0x20, 0x00, 0x80, 0x46}, ([](std::ostringstream& buf) { cvtsw(buf,f0,f0);           })},    // cvtsw    f0,f0
                { "cvtws",    { 0x64, 0x00, 0x00, 0x46}, ([](std::ostringstream& buf) { cvtws(buf,f1,f0);           })},    // cvtws    f1,f0
                { "daddiu",   { 0x11, 0x00, 0xe7, 0x64}, ([](std::ostringstream& buf) { daddiu(buf,a3,a3,0x11);     })},    // daddiu     a3,a3,0x11
                { "daddu",    { 0x2d, 0x58, 0x63, 0x01}, ([](std::ostringstream& buf) { daddu(buf,t3,t3,v1);        })},    // daddu      t3,t3,v1
                { "di",       { 0x39, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { di(buf);                    })},    // di
                { "div",      { 0x1a, 0x00, 0x43, 0x00}, ([](std::ostringstream& buf) { div(buf,v0,v1);             })},    // div        v0,v1
                { "divs",     { 0x03, 0x38, 0x16, 0x46}, ([](std::ostringstream& buf) { divs(buf,f0,f7,f22);        })},    // divs      f0,f7,f22
                { "divu",     { 0x1b, 0x00, 0x64, 0x00}, ([](std::ostringstream& buf) { divu(buf,v1,a0);            })},    // divu       v1,a0
                { "dsll",     { 0x78, 0x10, 0x02, 0x00}, ([](std::ostringstream& buf) { dsll(buf,v0,v0,0x1);        })},    // dsll       v0,v0,0x1
                { "dsll32",   { 0x3c, 0x10, 0x02, 0x00}, ([](std::ostringstream& buf) { dsll32(buf,v0,v0,0x0);      })},    // dsll32     v0,v0,0x0
                { "dsllv",    { 0x14, 0x38, 0xa2, 0x00}, ([](std::ostringstream& buf) { dsllv(buf,a3,v0,a1);        })},    // dsllv      a3,v0,a1
                { "dsra",     { 0x3b, 0x16, 0x02, 0x00}, ([](std::ostringstream& buf) { dsra(buf,v0,v0,0x18);       })},    // dsra       v0,v0,0x18
                { "dsra32",   { 0x3f, 0x10, 0x02, 0x00}, ([](std::ostringstream& buf) { dsra32(buf,v0,v0,0x0);      })},    // dsra32     v0,v0,0x0
                { "dsrl",     { 0x3a, 0x1c, 0x03, 0x00}, ([](std::ostringstream& buf) { dsrl(buf,v1,v1,0x10);       })},    // dsrl       v1,v1,0x10
                { "dsrl32",   { 0x3e, 0x20, 0x02, 0x00}, ([](std::ostringstream& buf) { dsrl32(buf,a0,v0,0x0);      })},    // dsrl32    a0,v0,0x0
                { "dsrlv",    { 0x16, 0x58, 0xab, 0x00}, ([](std::ostringstream& buf) { dsrlv(buf,t3,t3,a1);        })},    // dsrlv      t3,t3,a1
                { "dsubu",    { 0x2f, 0x18, 0x03, 0x00}, ([](std::ostringstream& buf) { dsubu(buf,v1,zero,v1);      })},    // dsubu      v1,zero,v1
                { "ei",       { 0x38, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { ei(buf);                    })},    // ei
                { "eret",     { 0x18, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { eret(buf);                  })},    // eret
                { "jal",      { 0x01, 0x00, 0x00, 0x0c}, ([](std::ostringstream& buf) { jal(buf,4);                 })},    // jal        4
                { "jalr",     { 0x09, 0xf8, 0x40, 0x00}, ([](std::ostringstream& buf) { jalr(buf,v0);               })},    // jalr       v0
                { "jr",       { 0x08, 0x00, 0xe0, 0x03}, ([](std::ostringstream& buf) { jr(buf,ra);                 })},    // jr         ra
                { "lb",       { 0x89, 0x07, 0x64, 0x80}, ([](std::ostringstream& buf) { lb(buf,a0,v1,0x789);        })},    // lb         a0,0x789(v1)
                { "lbu",      { 0x0c, 0x02, 0x42, 0x92}, ([](std::ostringstream& buf) { lbu(buf,v0,s2,0x20c);       })},    // lbu       v0,0x20c(s2)
                { "ld",       { 0x08, 0x00, 0xbf, 0xdf}, ([](std::ostringstream& buf) { ld(buf,ra,sp,0x8);          })},    // ld         ra,0x8(sp)
                { "ldl",      { 0x07, 0x00, 0x82, 0x68}, ([](std::ostringstream& buf) { ldl(buf,v0,a0,0x7);         })},    // ldl        v0,0x7(a0)
                { "ldr",      { 0x00, 0x00, 0x82, 0x6c}, ([](std::ostringstream& buf) { ldr(buf,v0,a0,0x0);         })},    // ldr        v0,0x0(a0)
                { "lh",       { 0x56, 0x00, 0x83, 0x84}, ([](std::ostringstream& buf) { lh(buf,v1,a0,0x56);         })},    // lh         v1,0x56(a0)
                { "lhu",      { 0x06, 0x02, 0x45, 0x96}, ([](std::ostringstream& buf) { lhu(buf,a1,s2,0x206);       })},    // lhu        a1,0x206(s2)
                { "li",       { 0x02, 0x00, 0x06, 0x24}, ([](std::ostringstream& buf) { li(buf,a2,0x2);             })},    // li         a2,0x2
                { "lui",      { 0x80, 0x37, 0x01, 0x3c}, ([](std::ostringstream& buf) { lui(buf,at,0x3780);         })},    // lui        at,0x3780
                { "lwc1",     { 0xf8, 0xde, 0x20, 0xc6}, ([](std::ostringstream& buf) { lwc1(buf,f0,s1,-0x2108);    })},    // lwc1       f0,-0x2108(s1)
                { "lwl",      { 0x0d, 0x00, 0x02, 0x8a}, ([](std::ostringstream& buf) { lwl(buf,v0,s0,0xd);         })},    // lwl        v0,0xd(s0)
                { "lwr",      { 0x0a, 0x00, 0x02, 0x9a}, ([](std::ostringstream& buf) { lwr(buf,v0,s0,0xa);         })},    // lwr        v0,0xa(s0)
                { "lwu",      { 0x0c, 0x00, 0x44, 0x9c}, ([](std::ostringstream& buf) { lwu(buf,a0,v0,0xc);         })},    // lwu        a0,0xc(v0)
                { "madd",     { 0x00, 0x00, 0x14, 0x72}, ([](std::ostringstream& buf) { madd(buf,s0,s4);            })},    // madd      s0,s4
                { "maddas",   { 0x1e, 0x18, 0x03, 0x46}, ([](std::ostringstream& buf) { maddas(buf,f3,f3);          })},    // maddas    f3,f3
                { "madds",    { 0x5c, 0x20, 0x04, 0x46}, ([](std::ostringstream& buf) { madds(buf,f1,f4,f4);        })},    // madds     f1,f4,f4
                { "maxs",     { 0x68, 0x05, 0x15, 0x46}, ([](std::ostringstream& buf) { maxs(buf,f21,f0,f21);       })},    // maxs      f21,f0,f21
                { "mfc1",     { 0x00, 0x00, 0x03, 0x44}, ([](std::ostringstream& buf) { mfc1(buf,v1,f0);            })},    // mfc1       v1,f0
                { "mfhi",     { 0x10, 0x10, 0x00, 0x00}, ([](std::ostringstream& buf) { mfhi(buf,v0);               })},    // mfhi       v0
                { "mfhi1",    { 0x10, 0x80, 0x00, 0x70}, ([](std::ostringstream& buf) { mfhi1(buf,s0);              })},    // mfhi1      s0
                { "mflo",     { 0x12, 0x18, 0x00, 0x00}, ([](std::ostringstream& buf) { mflo(buf,v1);               })},    // mflo       v1
                { "mflo1",    { 0x12, 0x30, 0x00, 0x70}, ([](std::ostringstream& buf) { mflo1(buf,a2);              })},    // mflo1      a2
                { "mins",     { 0x29, 0x0b, 0x07, 0x46}, ([](std::ostringstream& buf) { mins(buf,f12,f1,f7);        })},    // mins     f12,f1,f7
                { "moveq",    { 0xa9, 0x44, 0x40, 0x71}, ([](std::ostringstream& buf) { moveq(buf,t0,t2);           })},    // moveq      t0,t2
                { "movn",     { 0x0b, 0x20, 0x02, 0x00}, ([](std::ostringstream& buf) { movn(buf,a0,zero,v0);       })},    // movn       a0,zero,v0
                { "movs",     { 0x46, 0xa5, 0x00, 0x46}, ([](std::ostringstream& buf) { movs(buf,f21,f20);          })},    // movs     f21,f20
                { "movz",     { 0x0a, 0x18, 0x02, 0x00}, ([](std::ostringstream& buf) { movz(buf,v1,zero,v0);       })},    // movz       v1,zero,v0
                { "mtc1",     { 0x00, 0x08, 0x81, 0x44}, ([](std::ostringstream& buf) { mtc1(buf,at,f1);            })},    // mtc1       at,f1
                { "mthi",     { 0x11, 0x00, 0x40, 0x00}, ([](std::ostringstream& buf) { mthi(buf,v0);               })},    // mthi       v0
                { "mthi1",    { 0x11, 0x00, 0x40, 0x70}, ([](std::ostringstream& buf) { mthi1(buf,v0);              })},    // mthi1      v0
                { "mtlo",     { 0x13, 0x00, 0x40, 0x02}, ([](std::ostringstream& buf) { mtlo(buf,s2);               })},    // mtlo       s2
                { "mtlo1",    { 0x13, 0x00, 0x40, 0x70}, ([](std::ostringstream& buf) { mtlo1(buf,v0);              })},    // mtlo1      v0
                { "mtsa",     { 0x29, 0x00, 0x40, 0x00}, ([](std::ostringstream& buf) { mtsa(buf,v0);               })},    // mtsa       v0
                { "mtsab",    { 0x00, 0x00, 0x18, 0x05}, ([](std::ostringstream& buf) { mtsab(buf,t0,0x0);          })},    // mtsab      t0,0x0
                { "mulas",    { 0x1a, 0x10, 0x02, 0x46}, ([](std::ostringstream& buf) { mulas(buf,f2,f2);           })},    // mulas     f2,f2
                { "muls",     { 0x02, 0x00, 0x01, 0x46}, ([](std::ostringstream& buf) { muls(buf,f0,f0,f1);         })},    // muls      f0,f0,f1
                { "mult",     { 0x18, 0x20, 0x82, 0x00}, ([](std::ostringstream& buf) { mult(buf,a0,a0,v0);         })},    // mult       a0,a0,v0
                { "mult1",    { 0x18, 0x30, 0xc5, 0x70}, ([](std::ostringstream& buf) { mult1(buf,a2,a2,a1);        })},    // mult1      a2,a2,a1
                { "multu",    { 0x19, 0x00, 0x48, 0x00}, ([](std::ostringstream& buf) { multu(buf,v0,t0);           })},    // multu      v0,t0
                { "multu1",   { 0x19, 0x00, 0x02, 0x72}, ([](std::ostringstream& buf) { multu1(buf,s0,v0);          })},    // multu1     s0,v0
                { "negs",     { 0x07, 0xa5, 0x00, 0x46}, ([](std::ostringstream& buf) { negs(buf,f20,f20);          })},    // negs      f20,f20
                { "nop",      { 0x00, 0x00, 0x00, 0x00}, ([](std::ostringstream& buf) { nop(buf);                   })},    // nop
                { "nor",      { 0x27, 0x18, 0x03, 0x00}, ([](std::ostringstream& buf) { nor(buf,v1,zero,v1);        })},    // nor        v1,zero,v1
                { "or",       { 0x25, 0x18, 0x62, 0x00}, ([](std::ostringstream& buf) { _or(buf,v1,v1,v0);          })},    // or         v1,v1,v0
                { "ori",      { 0x36, 0xfa, 0x21, 0x34}, ([](std::ostringstream& buf) { ori(buf,at,at,0xfa36);      })},    // ori        at,at,0xfa36
                { "paddh",    { 0x08, 0x41, 0x19, 0x71}, ([](std::ostringstream& buf) { paddh(buf,t0,t0,t9);        })},    // paddh      t0,t0,t9
                { "pand",     { 0x89, 0x14, 0x43, 0x70}, ([](std::ostringstream& buf) { pand(buf,v0,v0,v1);         })},    // pand       v0,v0,v1
                { "pcgth",    { 0x88, 0x49, 0x40, 0x71}, ([](std::ostringstream& buf) { pcgth(buf,t1,t2,zero);      })},    // pcgth      t1,t2,zero
                { "pcpyh",    { 0xe9, 0x46, 0x0a, 0x70}, ([](std::ostringstream& buf) { pcpyh(buf,t0,t2);           })},    // pcpyh      t0,t2
                { "pcpyld",   { 0x89, 0x53, 0x28, 0x71}, ([](std::ostringstream& buf) { pcpyld(buf,t2,t1,t0);       })},    // pcpyld     t2,t1,t0
                { "pcpyud",   { 0xa9, 0x1b, 0x49, 0x70}, ([](std::ostringstream& buf) { pcpyud(buf,v1,v0,t1);       })},    // pcpyud     v1,v0,t1
                { "pextlb",   { 0x88, 0x46, 0x0a, 0x70}, ([](std::ostringstream& buf) { pextlb(buf,t0,zero,t2);     })},    // pextlb     t0,zero,t2
                { "pextlw",   { 0x88, 0x54, 0x48, 0x71}, ([](std::ostringstream& buf) { pextlw(buf,t2,t2,t0);       })},    // pextlw     t2,t2,t0
                { "pextub",   { 0xa8, 0x4e, 0x0a, 0x70}, ([](std::ostringstream& buf) { pextub(buf,t1,zero,t2);     })},    // pextub     t1,zero,t2
                { "pmaxh",    { 0xc8, 0x41, 0x00, 0x71}, ([](std::ostringstream& buf) { pmaxh(buf,t0,t0,zero);      })},    // pmaxh      t0,t0,zero
                { "pminh",    { 0xe8, 0x41, 0x03, 0x71}, ([](std::ostringstream& buf) { pminh(buf,t0,t0,v1);        })},    // pminh      t0,t0,v1
                { "pnor",     { 0xe9, 0xcc, 0x00, 0x70}, ([](std::ostringstream& buf) { pnor(buf,t9,zero,zero);     })},    // pnor       t9,zero,zero
                { "por",      { 0xa9, 0x14, 0x03, 0x70}, ([](std::ostringstream& buf) { por(buf,v0,zero,v1);        })},    // por        v0,zero,v1
                { "ppacb",    { 0xc8, 0x4e, 0x28, 0x71}, ([](std::ostringstream& buf) { ppacb(buf,t1,t1,t0);        })},    // ppacb      t1,t1,t0
                { "psllh",    { 0x74, 0xc8, 0x19, 0x70}, ([](std::ostringstream& buf) { psllh(buf,t9,t9,0x1);       })},    // psllh      t9,t9,0x1
                { "psrah",    { 0xf7, 0x53, 0x0b, 0x70}, ([](std::ostringstream& buf) { psrah(buf,t2,t3,0xf);       })},    // psrah      t2,t3,0xf
                { "psrlh",    { 0xf6, 0xcb, 0x19, 0x70}, ([](std::ostringstream& buf) { psrlh(buf,t9,t9,0xf);       })},    // psrlh      t9,t9,0xf
                { "psubb",    { 0x48, 0x12, 0x4a, 0x70}, ([](std::ostringstream& buf) { psubb(buf,v0,v0,t2);        })},    // psubb      v0,v0,t2
                { "psubw",    { 0x48, 0x38, 0x43, 0x70}, ([](std::ostringstream& buf) { psubw(buf,a3,v0,v1);        })},    // psubw      a3,v0,v1
                { "pxor",     { 0xc9, 0x14, 0x49, 0x70}, ([](std::ostringstream& buf) { pxor(buf,v0,v0,t1);         })},    // pxor       v0,v0,t1
                { "qfsrv",    { 0xe8, 0x56, 0x28, 0x71}, ([](std::ostringstream& buf) { qfsrv(buf,t2,t1,t0);        })},    // qfsrv      t2,t1,t0
                { "rsqrts",   { 0x16, 0x00, 0x03, 0x46}, ([](std::ostringstream& buf) { rsqrts(buf,f0,f0,f3);       })},    // rsqrts    f0,f0,f3
                { "sb",       { 0x09, 0x02, 0x44, 0xa2}, ([](std::ostringstream& buf) { sb(buf,a0,s2,0x209);        })},    // sb        a0,0x209(s2)
                { "sh",       { 0x06, 0x02, 0x42, 0xa6}, ([](std::ostringstream& buf) { sh(buf,v0,s2,0x206);        })},    // sh        v0,0x206(s2)
                { "sll",      { 0x80, 0x11, 0x02, 0x00}, ([](std::ostringstream& buf) { sll(buf,v0,v0,0x6);         })},    // sll        v0,v0,0x6
                { "sllv",     { 0x04, 0x10, 0xa2, 0x00}, ([](std::ostringstream& buf) { sllv(buf,v0,v0,a1);         })},    // sllv       v0,v0,a1
                { "slt",      { 0x2a, 0x20, 0x86, 0x00}, ([](std::ostringstream& buf) { slt(buf,a0,a0,a2);          })},    // slt        a0,a0,a2
                { "slti",     { 0x07, 0x00, 0x62, 0x28}, ([](std::ostringstream& buf) { slti(buf,v0,v1,0x7);        })},    // slti      v0,v1,0x7
                { "sltiu",    { 0x01, 0x00, 0x44, 0x2c}, ([](std::ostringstream& buf) { sltiu(buf,a0,v0,0x1);       })},    // sltiu     a0,v0,0x1
                { "sltu",     { 0x2b, 0x20, 0x02, 0x00}, ([](std::ostringstream& buf) { sltu(buf,a0,zero,v0);       })},    // sltu      a0,zero,v0
                { "sqrts",    { 0x04, 0x00, 0x00, 0x46}, ([](std::ostringstream& buf) { sqrts(buf,f0,f0);           })},    // sqrts     f0,f0
                { "sra",      { 0x83, 0x18, 0x02, 0x00}, ([](std::ostringstream& buf) { sra(buf,v1,v0,0x2);         })},    // sra        v1,v0,0x2
                { "srav",     { 0x07, 0x10, 0x44, 0x00}, ([](std::ostringstream& buf) { srav(buf,v0,a0,v0);         })},    // srav       v0,a0,v0
                { "srl",      { 0x42, 0x1a, 0x03, 0x00}, ([](std::ostringstream& buf) { srl(buf,v1,v1,0x9);         })},    // srl        v1,v1,0x9
                { "srlv",     { 0x06, 0x10, 0x44, 0x00}, ([](std::ostringstream& buf) { srlv(buf,v0,a0,v0);         })},    // srlv       v0,a0,v0
                { "subs",     { 0x01, 0xa5, 0x15, 0x46}, ([](std::ostringstream& buf) { subs(buf,f20,f20,f21);      })},    // subs      f20,f20,f21
                { "subu",     { 0x23, 0x28, 0x50, 0x02}, ([](std::ostringstream& buf) { subu(buf,a1,s2,s0);         })},    // subu       a1,s2,s0
                { "sw",       { 0x00, 0x00, 0x02, 0xae}, ([](std::ostringstream& buf) { sw(buf,v0,s0,0x0);          })},    // sw         v0,0x0(s0)
                { "swc1",     { 0x00, 0x00, 0x00, 0xe6}, ([](std::ostringstream& buf) { swc1(buf,f0,s0,0x0);        })},    // swc1       f0,0x0(s0)
                { "syscall",  { 0x0c, 0x00, 0x00, 0x00}, ([](std::ostringstream& buf) { syscall(buf);               })},    // syscall
                { "xor",      { 0x26, 0x10, 0x44, 0x00}, ([](std::ostringstream& buf) { _xor(buf,v0,v0,a0);         })},    // xor        v0,v0,a0
                { "xori",     { 0x27, 0x00, 0x42, 0x38}, ([](std::ostringstream& buf) { xori(buf,v0,v0,0x27);       })},    // xori       v0,v0,0x27
                { "daddi",    { 0x00, 0x80, 0x0e, 0x60}, ([](std::ostringstream& buf) { daddi(buf,t6,zero,-0x8000); })},    // daddi      t6,zero,-0x8000
                { "div1",     { 0x1a, 0x00, 0x2d, 0x72}, ([](std::ostringstream& buf) { div1(buf,s1,t5);            })},    // div1       s1,t5
                { "lw",       { 0x00, 0x00, 0x44, 0x8c}, ([](std::ostringstream& buf) { lw(buf,a0,v0,0x0);          })},    // lw         a0,0x0(v0)
                { "maddu",    { 0x01, 0x28, 0xa5, 0x70}, ([](std::ostringstream& buf) { maddu(buf,a1,a1,a1);        })},    // maddu      a1,a1,a1
                { "mfsa",     { 0x28, 0x10, 0x00, 0x00}, ([](std::ostringstream& buf) { mfsa(buf,v0);               })},    // mfsa       v0
                { "msubas",   { 0x1f, 0xa0, 0x14, 0x46}, ([](std::ostringstream& buf) { msubas(buf,f20,f20);        })},    // msubas    f20,f20
                { "msubs",    { 0x5d, 0x0b, 0x0d, 0x46}, ([](std::ostringstream& buf) { msubs(buf,f13,f1,f13);      })},    // msubs    f13,f1,f13
                { "paddsw",   { 0x08, 0x14, 0x48, 0x70}, ([](std::ostringstream& buf) { paddsw(buf,v0,v0,t0);       })},    // paddsw     v0,v0,t0
                { "paddw",    { 0x08, 0x60, 0x8b, 0x70}, ([](std::ostringstream& buf) { paddw(buf,t4,a0,t3);        })},    // paddw      t4,a0,t3
                { "pexch",    { 0xa9, 0x36, 0x06, 0x70}, ([](std::ostringstream& buf) { pexch(buf,a2,a2);           })},    // pexch      a2,a2
                { "pexew",    { 0x89, 0x4f, 0x08, 0x70}, ([](std::ostringstream& buf) { pexew(buf,t1,t0);           })},    // pexew      t1,t0
                { "pextlh",   { 0x88, 0x45, 0x08, 0x70}, ([](std::ostringstream& buf) { pextlh(buf,t0,zero,t0);     })},    // pextlh     t0,zero,t0
                { "pextuw",   { 0xa8, 0x6c, 0x28, 0x71}, ([](std::ostringstream& buf) { pextuw(buf,t5,t1,t0);       })},    // pextuw     t5,t1,t0
                { "pmaxw",    { 0xc8, 0x88, 0x2f, 0x72}, ([](std::ostringstream& buf) { pmaxw(buf,s1,s1,t7);        })},    // pmaxw      s1,s1,t7
                { "pmfhi",    { 0x09, 0x32, 0x00, 0x70}, ([](std::ostringstream& buf) { pmfhi(buf,a2);              })},    // pmfhi      a2
                { "pmflo",    { 0x49, 0x42, 0x00, 0x70}, ([](std::ostringstream& buf) { pmflo(buf,t0);              })},    // pmflo      t0
                { "pminw",    { 0xe8, 0x88, 0x38, 0x72}, ([](std::ostringstream& buf) { pminw(buf,s1,s1,t8);        })},    // pminw      s1,s1,t8
                { "pmulth",   { 0x09, 0x37, 0xc9, 0x70}, ([](std::ostringstream& buf) { pmulth(buf,a2,a2,t1);       })},    // pmulth     a2,a2,t1
                { "ppach",    { 0xc8, 0x15, 0x02, 0x70}, ([](std::ostringstream& buf) { ppach(buf,v0,zero,v0);      })},    // ppach      v0,zero,v0
                { "prot3w",   { 0xc9, 0x47, 0x08, 0x70}, ([](std::ostringstream& buf) { prot3w(buf,t0,t0);          })},    // prot3w     t0,t0
                { "psllw",    { 0xbc, 0x58, 0x0d, 0x70}, ([](std::ostringstream& buf) { psllw(buf,t3,t5,0x2);       })},    // psllw      t3,t5,0x2
                { "psraw",    { 0x3f, 0x64, 0x0e, 0x70}, ([](std::ostringstream& buf) { psraw(buf,t4,t6,0x10);      })},    // psraw      t4,t6,0x10
                { "psrlw",    { 0x3e, 0x4c, 0x0a, 0x70}, ([](std::ostringstream& buf) { psrlw(buf,t1,t2,0x10);      })},    // psrlw      t1,t2,0x10
                { "sq",       { 0x00, 0x00, 0x40, 0x7c}, ([](std::ostringstream& buf) { sq(buf,zero,v0,0x0);        })},    // sq         zero,0x0(v0)
                { "sub",      { 0x22, 0x08, 0x28, 0x00}, ([](std::ostringstream& buf) { sub(buf,at,at,t0);          })},    // sub        at,at,t0
                { "tge",      { 0x30, 0xbb, 0x1a, 0x00}, ([](std::ostringstream& buf) { tge(buf,zero,k0);           })},    // tge        zero,k0
                { "tlbwi",    { 0x02, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { tlbwi(buf);                 })},    // tlbwi
            };

            for (auto var : testDataMain)
            {
                checkInstr(var);
            }

            std::vector<MipsTest> testDataAdditional = {
                { "abss",   { 0x05, 0x60, 0x00, 0x46}, ([](std::ostringstream& buf) { abss(buf,f0,f12);          })},    // abss     f0,f12
                { "add",    { 0x20, 0x40, 0x03, 0x01}, ([](std::ostringstream& buf) { add(buf,t0,t0,v1);         })},    // add        t0,t0,v1
                { "addas",  { 0x18, 0x00, 0x01, 0x46}, ([](std::ostringstream& buf) { addas(buf,f0,f1);          })},    // addas     f0,f1
                { "addi",   { 0xff, 0xff, 0xe7, 0x20}, ([](std::ostringstream& buf) { addi(buf,a3,a3,-0x1);      })},    // addi       a3,a3,-0x1
                { "addiu",  { 0x80, 0x2e, 0x42, 0x24}, ([](std::ostringstream& buf) { addiu(buf,v0,v0,0x2e80);   })},    // addiu      v0,v0,0x2e80
                { "adds",   { 0x00, 0x00, 0x00, 0x46}, ([](std::ostringstream& buf) { adds(buf,f0,f0,f0);        })},    // adds      f0,f0,f0
                { "addu",   { 0x21, 0x38, 0x07, 0x02}, ([](std::ostringstream& buf) { addu(buf,a3,s0,a3);        })},    // addu       a3,s0,a3
                { "and",    { 0x24, 0x20, 0x64, 0x00}, ([](std::ostringstream& buf) { _and(buf,a0,v1,a0);        })},    // and        a0,v1,a0
                { "andi",   { 0x0f, 0x00, 0x44, 0x30}, ([](std::ostringstream& buf) { andi(buf,a0,v0,0xf);       })},    // andi       a0,v0,0xf
                { "b",      { 0x08, 0x00, 0x00, 0x10}, ([](std::ostringstream& buf) { b(buf,0x08);               })},    // b          lab001002b0
                { "bc0f",   { 0xf9, 0xff, 0x00, 0x41}, ([](std::ostringstream& buf) { bc0f(buf,0xf9);            })},    // bc0f       lab00298f0c
                { "bc1f",   { 0x05, 0x00, 0x00, 0x45}, ([](std::ostringstream& buf) { bc1f(buf,0x05);            })},    // bc1f       lab001193bc
                { "bc1fl",  { 0x05, 0x00, 0x02, 0x45}, ([](std::ostringstream& buf) { bc1fl(buf,0x05);           })},    // bc1fl      lab001243ec
                { "bc1tl",  { 0x01, 0x00, 0x03, 0x45}, ([](std::ostringstream& buf) { bc1tl(buf,0x01);           })},    // bc1tl      lab00126b04
                { "beq",    { 0x06, 0x00, 0x80, 0x10}, ([](std::ostringstream& buf) { beq(buf,a0,zero,0x06);     })},    // beq        a0,zero,lab0010014c
                { "bne",    { 0x03, 0x00, 0x60, 0x14}, ([](std::ostringstream& buf) { bne(buf,v1,zero,0x03);     })},    // bne        v1,zero,lab00100260
                { "ceqs",   { 0x32, 0x60, 0x00, 0x46}, ([](std::ostringstream& buf) { ceqs(buf,f12,f0);          })},    // ceqs     f12,f0
                { "cles",   { 0x36, 0x38, 0x00, 0x46}, ([](std::ostringstream& buf) { cles(buf,f7,f0);           })},    // cles     f7,f0
                { "clts",   { 0x34, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { clts(buf,f1,f0);           })},    // clts     f1,f0
                { "cvtsw",  { 0x60, 0x29, 0x80, 0x46}, ([](std::ostringstream& buf) { cvtsw(buf,f5,f5);          })},    // cvtsw    f5,f5
                { "cvtws",  { 0xe4, 0x00, 0x00, 0x46}, ([](std::ostringstream& buf) { cvtws(buf,f3,f0);          })},    // cvtws    f3,f0
                { "daddi",  { 0x00, 0x80, 0x0e, 0x60}, ([](std::ostringstream& buf) { daddi(buf,t6,zero,-0x8000); })},    // daddi      t6,zero,-0x8000
                { "daddiu", { 0x01, 0x00, 0x10, 0x66}, ([](std::ostringstream& buf) { daddiu(buf,s0,s0,0x1);     })},    // daddiu     s0,s0,0x1
                { "daddu",  { 0x2d, 0x18, 0x82, 0x00}, ([](std::ostringstream& buf) { daddu(buf,v1,a0,v0);       })},    // daddu      v1,a0,v0
                { "di",     { 0x39, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { di(buf);                   })},    // di
                { "div",    { 0x1a, 0x00, 0x85, 0x00}, ([](std::ostringstream& buf) { div(buf,a0,a1);            })},    // div        a0,a1
                { "div1",   { 0x1a, 0x00, 0x2d, 0x72}, ([](std::ostringstream& buf) { div1(buf,s1,t5);           })},    // div1       s1,t5
                { "divs",   { 0x03, 0x00, 0x01, 0x46}, ([](std::ostringstream& buf) { divs(buf,f0,f0,f1);        })},    // divs      f0,f0,f1
                { "divu",   { 0x1b, 0x00, 0x52, 0x00}, ([](std::ostringstream& buf) { divu(buf,v0,s2);           })},    // divu       v0,s2
                { "dsll",   { 0xb8, 0x28, 0x13, 0x00}, ([](std::ostringstream& buf) { dsll(buf,a1,s3,0x2);       })},    // dsll      a1,s3,0x2
                { "dsll32", { 0x3c, 0x68, 0x0d, 0x00}, ([](std::ostringstream& buf) { dsll32(buf,t5,t5,0x0);     })},    // dsll32     t5,t5,0x0
                { "dsllv",  { 0x14, 0x10, 0xa2, 0x00}, ([](std::ostringstream& buf) { dsllv(buf,v0,v0,a1);       })},    // dsllv      v0,v0,a1
                { "dsra",   { 0x7b, 0x13, 0x02, 0x00}, ([](std::ostringstream& buf) { dsra(buf,v0,v0,0xd);       })},    // dsra       v0,v0,0xd
                { "dsra32", { 0x3f, 0x68, 0x0d, 0x00}, ([](std::ostringstream& buf) { dsra32(buf,t5,t5,0x0);     })},    // dsra32     t5,t5,0x0
                { "dsrl",   { 0x7a, 0x18, 0x04, 0x00}, ([](std::ostringstream& buf) { dsrl(buf,v1,a0,0x1);       })},    // dsrl       v1,a0,0x1
                { "dsrl32", { 0x3e, 0x68, 0x0d, 0x00}, ([](std::ostringstream& buf) { dsrl32(buf,t5,t5,0x0);     })},    // dsrl32     t5,t5,0x0
                { "dsrlv",  { 0x16, 0x28, 0x65, 0x00}, ([](std::ostringstream& buf) { dsrlv(buf,a1,a1,v1);       })},    // dsrlv      a1,a1,v1
                { "dsubu",  { 0x2f, 0x68, 0x0d, 0x00}, ([](std::ostringstream& buf) { dsubu(buf,t5,zero,t5);     })},    // dsubu      t5,zero,t5
                { "ei",     { 0x38, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { ei(buf);                   })},    // ei
                { "eret",   { 0x18, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { eret(buf);                 })},    // eret
                { "jalr",   { 0x09, 0xf8, 0x40, 0x00}, ([](std::ostringstream& buf) { jalr(buf,v0);              })},    // jalr       v0
                { "jr",     { 0x08, 0x00, 0xe0, 0x03}, ([](std::ostringstream& buf) { jr(buf,ra);                })},    // jr         ra
                { "lb",     { 0xa8, 0x39, 0x42, 0x80}, ([](std::ostringstream& buf) { lb(buf,v0,v0,0x39a8);      })},    // lb         v0,0x39a8(v0)
                { "lbu",    { 0x00, 0x00, 0x84, 0x90}, ([](std::ostringstream& buf) { lbu(buf,a0,a0,0x0);        })},    // lbu        a0,0x0(a0)
                { "ldl",    { 0x17, 0x00, 0xa2, 0x6b}, ([](std::ostringstream& buf) { ldl(buf,v0,sp,0x17);       })},    // ldl        v0,0x17(sp)
                { "ldr",    { 0x10, 0x00, 0xa2, 0x6f}, ([](std::ostringstream& buf) { ldr(buf,v0,sp,0x10);       })},    // ldr        v0,0x10(sp)
                { "lh",     { 0x0c, 0x00, 0x23, 0x86}, ([](std::ostringstream& buf) { lh(buf,v1,s1,0xc);         })},    // lh        v1,0xc(s1)
                { "lhu",    { 0x38, 0x00, 0x62, 0x94}, ([](std::ostringstream& buf) { lhu(buf,v0,v1,0x38);       })},    // lhu        v0,0x38(v1)
                { "li",     { 0x3c, 0x00, 0x03, 0x24}, ([](std::ostringstream& buf) { li(buf,v1,0x3c);           })},    // li         v1,0x3c
                { "lui",    { 0x88, 0x00, 0x02, 0x3c}, ([](std::ostringstream& buf) { lui(buf,v0,0x88);          })},    // lui        v0,0x88
                { "lw",     { 0x00, 0x00, 0x44, 0x8c}, ([](std::ostringstream& buf) { lw(buf,a0,v0,0x0);         })},    // lw         a0,0x0(v0)
                { "lwl",    { 0x23, 0x00, 0x43, 0x88}, ([](std::ostringstream& buf) { lwl(buf,v1,v0,0x23);       })},    // lwl        v1,0x23(v0)
                { "lwr",    { 0x20, 0x00, 0x43, 0x98}, ([](std::ostringstream& buf) { lwr(buf,v1,v0,0x20);       })},    // lwr        v1,0x20(v0)
                { "lwu",    { 0x00, 0x00, 0xa6, 0x9f}, ([](std::ostringstream& buf) { lwu(buf,a2,sp,0x0);        })},    // lwu       a2,0x0(sp)=>local10
                { "maddas", { 0x1e, 0x18, 0x02, 0x46}, ([](std::ostringstream& buf) { maddas(buf,f3,f2);         })},    // maddas    f3,f2
                { "madds",  { 0x5c, 0x20, 0x03, 0x46}, ([](std::ostringstream& buf) { madds(buf,f1,f4,f3);       })},    // madds     f1,f4,f3
                { "maddu",  { 0x01, 0x28, 0xa5, 0x70}, ([](std::ostringstream& buf) { maddu(buf,a1,a1,a1);       })},    // maddu      a1,a1,a1
                { "maxs",   { 0x28, 0x60, 0x0d, 0x46}, ([](std::ostringstream& buf) { maxs(buf,f0,f12,f13);      })},    // maxs     f0,f12,f13
                { "mfc1",   { 0x00, 0x60, 0x06, 0x44}, ([](std::ostringstream& buf) { mfc1(buf,a2,f12);          })},    // mfc1       a2,f12
                { "mfhi",   { 0x10, 0x10, 0x00, 0x00}, ([](std::ostringstream& buf) { mfhi(buf,v0);              })},    // mfhi       v0
                { "mfhi1",  { 0x10, 0x10, 0x00, 0x70}, ([](std::ostringstream& buf) { mfhi1(buf,v0);             })},    // mfhi1      v0
                { "mflo",   { 0x12, 0x10, 0x00, 0x00}, ([](std::ostringstream& buf) { mflo(buf,v0);              })},    // mflo       v0
                { "mflo1",  { 0x12, 0x10, 0x00, 0x70}, ([](std::ostringstream& buf) { mflo1(buf,v0);             })},    // mflo1      v0
                { "mfsa",   { 0x28, 0x10, 0x00, 0x00}, ([](std::ostringstream& buf) { mfsa(buf,v0);              })},    // mfsa       v0
                { "mins",   { 0x29, 0x60, 0x0d, 0x46}, ([](std::ostringstream& buf) { mins(buf,f0,f12,f13);      })},    // mins     f0,f12,f13
                { "movs",   { 0x06, 0x60, 0x00, 0x46}, ([](std::ostringstream& buf) { movs(buf,f0,f12);          })},    // movs      f0,f12
                { "movz",   { 0x0a, 0x30, 0x82, 0x00}, ([](std::ostringstream& buf) { movz(buf,a2,a0,v0);        })},    // movz       a2,a0,v0
                { "msubas", { 0x1f, 0xa0, 0x14, 0x46}, ([](std::ostringstream& buf) { msubas(buf,f20,f20);       })},    // msubas    f20,f20
                { "msubs",  { 0x5d, 0x0b, 0x0d, 0x46}, ([](std::ostringstream& buf) { msubs(buf,f13,f1,f13);     })},    // msubs    f13,f1,f13
                { "mtc1",   { 0x00, 0x00, 0x82, 0x44}, ([](std::ostringstream& buf) { mtc1(buf,v0,f0);           })},    // mtc1       v0,f0
                { "mthi",   { 0x11, 0x00, 0x40, 0x00}, ([](std::ostringstream& buf) { mthi(buf,v0);              })},    // mthi       v0
                { "mthi1",  { 0x11, 0x00, 0x40, 0x70}, ([](std::ostringstream& buf) { mthi1(buf,v0);             })},    // mthi1      v0
                { "mtlo",   { 0x13, 0x00, 0x40, 0x00}, ([](std::ostringstream& buf) { mtlo(buf,v0);              })},    // mtlo       v0
                { "mtlo1",  { 0x13, 0x00, 0x40, 0x70}, ([](std::ostringstream& buf) { mtlo1(buf,v0);             })},    // mtlo1      v0
                { "mtsa",   { 0x29, 0x00, 0x40, 0x00}, ([](std::ostringstream& buf) { mtsa(buf,v0);              })},    // mtsa       v0
                { "mulas",  { 0x1a, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { mulas(buf,f1,f0);          })},    // mulas     f1,f0
                { "muls",   { 0x02, 0x00, 0x14, 0x46}, ([](std::ostringstream& buf) { muls(buf,f0,f0,f20);       })},    // muls      f0,f0,f20
                { "mult",   { 0x18, 0x28, 0xa6, 0x00}, ([](std::ostringstream& buf) { mult(buf,a1,a1,a2);        })},    // mult      a1,a1,a2
                { "mult1",  { 0x18, 0x18, 0x69, 0x70}, ([](std::ostringstream& buf) { mult1(buf,v1,v1,t1);       })},    // mult1      v1,v1,t1
                { "multu",  { 0x19, 0x00, 0x43, 0x00}, ([](std::ostringstream& buf) { multu(buf,v0,v1);          })},    // multu      v0,v1
                { "multu1", { 0x19, 0x00, 0x02, 0x72}, ([](std::ostringstream& buf) { multu1(buf,s0,v0);         })},    // multu1     s0,v0
                { "negs",   { 0x07, 0xa0, 0x00, 0x46}, ([](std::ostringstream& buf) { negs(buf,f0,f20);          })},    // negs      f0,f20
                { "nop",    { 0x00, 0x00, 0x00, 0x00}, ([](std::ostringstream& buf) { nop(buf);                  })},    // nop
                { "nor",    { 0x27, 0x10, 0x20, 0x01}, ([](std::ostringstream& buf) { nor(buf,v0,t1,zero);       })},    // nor        v0,t1,zero
                { "or",     { 0x25, 0x38, 0xf1, 0x00}, ([](std::ostringstream& buf) { _or(buf,a3,a3,s1);         })},    // or         a3,a3,s1
                { "ori",    { 0xf0, 0xff, 0x84, 0x34}, ([](std::ostringstream& buf) { ori(buf,a0,a0,0xfff0);     })},    // ori        a0,a0,0xfff0
                { "paddsw", { 0x08, 0x14, 0x48, 0x70}, ([](std::ostringstream& buf) { paddsw(buf,v0,v0,t0);      })},    // paddsw     v0,v0,t0
                { "paddw",  { 0x08, 0x60, 0x8b, 0x70}, ([](std::ostringstream& buf) { paddw(buf,t4,a0,t3);       })},    // paddw      t4,a0,t3
                { "pand",   { 0x89, 0x14, 0x43, 0x70}, ([](std::ostringstream& buf) { pand(buf,v0,v0,v1);        })},    // pand       v0,v0,v1
                { "pcpyh",  { 0xe9, 0x46, 0x0a, 0x70}, ([](std::ostringstream& buf) { pcpyh(buf,t0,t2);          })},    // pcpyh      t0,t2
                { "pcpyld", { 0x89, 0x4b, 0x08, 0x71}, ([](std::ostringstream& buf) { pcpyld(buf,t1,t0,t0);      })},    // pcpyld     t1,t0,t0
                { "pcpyud", { 0xa9, 0x1b, 0x49, 0x70}, ([](std::ostringstream& buf) { pcpyud(buf,v1,v0,t1);      })},    // pcpyud     v1,v0,t1
                { "pexch",  { 0xa9, 0x36, 0x06, 0x70}, ([](std::ostringstream& buf) { pexch(buf,a2,a2);          })},    // pexch      a2,a2
                { "pexew",  { 0x89, 0x4f, 0x08, 0x70}, ([](std::ostringstream& buf) { pexew(buf,t1,t0);          })},    // pexew      t1,t0
                { "pextlb", { 0x88, 0x16, 0x04, 0x70}, ([](std::ostringstream& buf) { pextlb(buf,v0,zero,a0);    })},    // pextlb     v0,zero,a0
                { "pextlh", { 0x88, 0x45, 0x08, 0x70}, ([](std::ostringstream& buf) { pextlh(buf,t0,zero,t0);    })},    // pextlh     t0,zero,t0
                { "pextlw", { 0x88, 0x64, 0x28, 0x71}, ([](std::ostringstream& buf) { pextlw(buf,t4,t1,t0);      })},    // pextlw     t4,t1,t0
                { "pextuw", { 0xa8, 0x6c, 0x28, 0x71}, ([](std::ostringstream& buf) { pextuw(buf,t5,t1,t0);      })},    // pextuw     t5,t1,t0
                { "pmaxh",  { 0xc8, 0x29, 0xa0, 0x70}, ([](std::ostringstream& buf) { pmaxh(buf,a1,a1,zero);     })},    // pmaxh      a1,a1,zero
                { "pmaxw",  { 0xc8, 0x88, 0x2f, 0x72}, ([](std::ostringstream& buf) { pmaxw(buf,s1,s1,t7);       })},    // pmaxw      s1,s1,t7
                { "pmfhi",  { 0x09, 0x32, 0x00, 0x70}, ([](std::ostringstream& buf) { pmfhi(buf,a2);             })},    // pmfhi      a2
                { "pmflo",  { 0x49, 0x42, 0x00, 0x70}, ([](std::ostringstream& buf) { pmflo(buf,t0);             })},    // pmflo      t0
                { "pminh",  { 0xe8, 0x29, 0xb9, 0x70}, ([](std::ostringstream& buf) { pminh(buf,a1,a1,t9);       })},    // pminh      a1,a1,t9
                { "pminw",  { 0xe8, 0x88, 0x38, 0x72}, ([](std::ostringstream& buf) { pminw(buf,s1,s1,t8);       })},    // pminw      s1,s1,t8
                { "pmulth", { 0x09, 0x37, 0xc9, 0x70}, ([](std::ostringstream& buf) { pmulth(buf,a2,a2,t1);      })},    // pmulth     a2,a2,t1
                { "pnor",   { 0xe9, 0x1c, 0x02, 0x70}, ([](std::ostringstream& buf) { pnor(buf,v1,zero,v0);      })},    // pnor       v1,zero,v0
                { "ppacb",  { 0xc8, 0x16, 0x42, 0x70}, ([](std::ostringstream& buf) { ppacb(buf,v0,v0,v0);       })},    // ppacb     v0,v0,v0
                { "ppach",  { 0xc8, 0x15, 0x02, 0x70}, ([](std::ostringstream& buf) { ppach(buf,v0,zero,v0);     })},    // ppach      v0,zero,v0
                { "prot3w", { 0xc9, 0x47, 0x08, 0x70}, ([](std::ostringstream& buf) { prot3w(buf,t0,t0);         })},    // prot3w     t0,t0
                { "psllw",  { 0xbc, 0x58, 0x0d, 0x70}, ([](std::ostringstream& buf) { psllw(buf,t3,t5,0x2);      })},    // psllw      t3,t5,0x2
                { "psraw",  { 0x3f, 0x64, 0x0e, 0x70}, ([](std::ostringstream& buf) { psraw(buf,t4,t6,0x10);     })},    // psraw      t4,t6,0x10
                { "psrlh",  { 0x76, 0x10, 0x02, 0x70}, ([](std::ostringstream& buf) { psrlh(buf,v0,v0,0x1);      })},    // psrlh      v0,v0,0x1
                { "psrlw",  { 0x3e, 0x4c, 0x0a, 0x70}, ([](std::ostringstream& buf) { psrlw(buf,t1,t2,0x10);     })},    // psrlw      t1,t2,0x10
                { "psubb",  { 0x48, 0x12, 0x4a, 0x70}, ([](std::ostringstream& buf) { psubb(buf,v0,v0,t2);       })},    // psubb      v0,v0,t2
                { "psubw",  { 0x48, 0x38, 0x43, 0x70}, ([](std::ostringstream& buf) { psubw(buf,a3,v0,v1);       })},    // psubw      a3,v0,v1
                { "pxor",   { 0xc9, 0x14, 0x49, 0x70}, ([](std::ostringstream& buf) { pxor(buf,v0,v0,t1);        })},    // pxor       v0,v0,t1
                { "rsqrts", { 0x16, 0x00, 0x0c, 0x46}, ([](std::ostringstream& buf) { rsqrts(buf,f0,f0,f12);     })},    // rsqrts    f0,f0,f12
                { "sb",     { 0x00, 0x00, 0x40, 0xa0}, ([](std::ostringstream& buf) { sb(buf,zero,v0,0x0);       })},    // sb         zero,0x0(v0)
                { "sh",     { 0x38, 0x00, 0x83, 0xa4}, ([](std::ostringstream& buf) { sh(buf,v1,a0,0x38);        })},    // sh         v1,0x38(a0)
                { "sll",    { 0x80, 0x38, 0x0b, 0x00}, ([](std::ostringstream& buf) { sll(buf,a3,t3,0x2);        })},    // sll        a3,t3,0x2
                { "sllv",   { 0x04, 0x20, 0x83, 0x00}, ([](std::ostringstream& buf) { sllv(buf,a0,v1,a0);        })},    // sllv       a0,v1,a0
                { "slt",    { 0x2a, 0x08, 0x41, 0x00}, ([](std::ostringstream& buf) { slt(buf,at,v0,at);         })},    // slt        at,v0,at
                { "slti",   { 0x60, 0x00, 0xe3, 0x28}, ([](std::ostringstream& buf) { slti(buf,v1,a3,0x60);      })},    // slti       v1,a3,0x60
                { "sltiu",  { 0x0a, 0x00, 0x67, 0x2d}, ([](std::ostringstream& buf) { sltiu(buf,a3,t3,0xa);      })},    // sltiu      a3,t3,0xa
                { "sltu",   { 0x2b, 0x08, 0x67, 0x00}, ([](std::ostringstream& buf) { sltu(buf,at,v1,a3);        })},    // sltu       at,v1,a3
                { "sq",     { 0x00, 0x00, 0x40, 0x7c}, ([](std::ostringstream& buf) { sq(buf,zero,v0,0x0);       })},    // sq         zero,0x0(v0)
                { "sqrts",  { 0x04, 0x00, 0x0c, 0x46}, ([](std::ostringstream& buf) { sqrts(buf,f0,f12);         })},    // sqrts     f0,f12
                { "sra",    { 0x43, 0x11, 0x02, 0x00}, ([](std::ostringstream& buf) { sra(buf,v0,v0,0x5);        })},    // sra        v0,v0,0x5
                { "srav",   { 0x07, 0x10, 0x82, 0x00}, ([](std::ostringstream& buf) { srav(buf,v0,v0,a0);        })},    // srav       v0,v0,a0
                { "srl",    { 0x82, 0x20, 0x06, 0x00}, ([](std::ostringstream& buf) { srl(buf,a0,a2,0x2);        })},    // srl        a0,a2,0x2
                { "srlv",   { 0x06, 0x10, 0x44, 0x00}, ([](std::ostringstream& buf) { srlv(buf,v0,a0,v0);        })},    // srlv       v0,a0,v0
                { "sub",    { 0x22, 0x08, 0x28, 0x00}, ([](std::ostringstream& buf) { sub(buf,at,at,t0);         })},    // sub        at,at,t0
                { "subs",   { 0x01, 0x58, 0x0b, 0x46}, ([](std::ostringstream& buf) { subs(buf,f0,f11,f11);      })},    // subs      f0,f11,f11
                { "subu",   { 0x23, 0x30, 0x0d, 0x01}, ([](std::ostringstream& buf) { subu(buf,a2,t0,t5);        })},    // subu       a2,t0,t5
                { "syscall",{ 0x0c, 0x00, 0x00, 0x00}, ([](std::ostringstream& buf) { syscall(buf);              })},    // syscall
                { "tge",    { 0x30, 0xbb, 0x1a, 0x00}, ([](std::ostringstream& buf) { tge(buf,zero,k0);          })},    // tge        zero,k0
                { "tlbwi",  { 0x02, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { tlbwi(buf);                })},    // tlbwi
                { "xor",    { 0x26, 0x10, 0x62, 0x00}, ([](std::ostringstream& buf) { _xor(buf,v0,v1,v0);        })},    // xor        v0,v1,v0
                { "xori",   { 0x01, 0x00, 0x42, 0x38}, ([](std::ostringstream& buf) { xori(buf,v0,v0,0x1);       })},    // xori       v0,v0,0x1
                { "abss",   { 0x45, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { abss(buf,f1,f1);           })},    // abss      f1,f1
                { "add",    { 0x20, 0x40, 0x05, 0x00}, ([](std::ostringstream& buf) { add(buf,t0,zero,a1);       })},    // add        t0,zero,a1
                { "addas",  { 0x18, 0x00, 0x01, 0x46}, ([](std::ostringstream& buf) { addas(buf,f0,f1);          })},    // addas     f0,f1
                { "addi",   { 0x11, 0x00, 0x03, 0x20}, ([](std::ostringstream& buf) { addi(buf,v1,zero,0x11);    })},    // addi       v1,zero,0x11
                { "addiu",  { 0x00, 0x60, 0x42, 0x24}, ([](std::ostringstream& buf) { addiu(buf,v0,v0,0x6000);   })},    // addiu      v0,v0,0x6000
                { "adds",   { 0x80, 0x10, 0x00, 0x46}, ([](std::ostringstream& buf) { adds(buf,f2,f2,f0);        })},    // adds      f2,f2,f0
                { "addu",   { 0x21, 0x40, 0x0a, 0x01}, ([](std::ostringstream& buf) { addu(buf,t0,t0,t2);        })},    // addu       t0,t0,t2
                { "and",    { 0x24, 0x20, 0x64, 0x00}, ([](std::ostringstream& buf) { _and(buf,a0,v1,a0);        })},    // and        a0,v1,a0
                { "andi",   { 0x0f, 0x00, 0x44, 0x30}, ([](std::ostringstream& buf) { andi(buf,a0,v0,0xf);       })},    // andi       a0,v0,0xf
                { "b",      { 0xb9, 0x01, 0x00, 0x10}, ([](std::ostringstream& buf) { b(buf,0xb9);               })},    // b          lab00100a70
                { "bc1f",   { 0x02, 0x00, 0x00, 0x45}, ([](std::ostringstream& buf) { bc1f(buf,0x02);            })},    // bc1f       lab00107094
                { "bc1fl",  { 0x0f, 0x00, 0x02, 0x45}, ([](std::ostringstream& buf) { bc1fl(buf,0x0f);           })},    // bc1fl      lab002d50a0
                { "beq",    { 0x06, 0x00, 0x80, 0x10}, ([](std::ostringstream& buf) { beq(buf,a0,zero,0x06);     })},    // beq        a0,zero,lab0010014c
                { "bne",    { 0x04, 0x00, 0x40, 0x14}, ([](std::ostringstream& buf) { bne(buf,v0,zero,0x04);     })},    // bne        v0,zero,lab00100390
                { "ceqs",   { 0x32, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { ceqs(buf,f1,f0);           })},    // ceqs     f1,f0
                { "cles",   { 0x36, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { cles(buf,f1,f0);           })},    // cles     f1,f0
                { "clts",   { 0x34, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { clts(buf,f1,f0);           })},    // clts     f1,f0
                { "cvtsw",  { 0xa0, 0x10, 0x80, 0x46}, ([](std::ostringstream& buf) { cvtsw(buf,f2,f2);          })},    // cvtsw    f2,f2
                { "cvtws",  { 0xa4, 0x07, 0x00, 0x46}, ([](std::ostringstream& buf) { cvtws(buf,f30,f0);         })},    // cvtws    f30,f0
                { "daddi",  { 0x00, 0x80, 0x0b, 0x60}, ([](std::ostringstream& buf) { daddi(buf,t3,zero,-0x8000); })},    // daddi      t3,zero,-0x8000
                { "daddiu", { 0x01, 0x00, 0x04, 0x64}, ([](std::ostringstream& buf) { daddiu(buf,a0,zero,0x1);   })},    // daddiu    a0,zero,0x1
                { "daddu",  { 0x2d, 0x48, 0x28, 0x01}, ([](std::ostringstream& buf) { daddu(buf,t1,t1,t0);       })},    // daddu     t1,t1,t0
                { "di",     { 0x39, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { di(buf);                   })},    // di
                { "div",    { 0x1a, 0x00, 0x51, 0x00}, ([](std::ostringstream& buf) { div(buf,v0,s1);            })},    // div        v0,s1
                { "div1",   { 0x1a, 0x00, 0x82, 0x70}, ([](std::ostringstream& buf) { div1(buf,a0,v0);           })},    // div1       a0,v0
                { "divs",   { 0x43, 0x08, 0x02, 0x46}, ([](std::ostringstream& buf) { divs(buf,f1,f1,f2);        })},    // divs      f1,f1,f2
                { "divu",   { 0x1b, 0x00, 0x62, 0x00}, ([](std::ostringstream& buf) { divu(buf,v1,v0);           })},    // divu       v1,v0
                { "dsll",   { 0x38, 0x1c, 0x03, 0x00}, ([](std::ostringstream& buf) { dsll(buf,v1,v1,0x10);      })},    // dsll       v1,v1,0x10
                { "dsll32", { 0x7c, 0x18, 0x05, 0x00}, ([](std::ostringstream& buf) { dsll32(buf,v1,a1,0x1);     })},    // dsll32     v1,a1,0x1
                { "dsllv",  { 0x14, 0x18, 0x62, 0x00}, ([](std::ostringstream& buf) { dsllv(buf,v1,v0,v1);       })},    // dsllv      v1,v0,v1
                { "dsra",   { 0x3b, 0x22, 0x04, 0x00}, ([](std::ostringstream& buf) { dsra(buf,a0,a0,0x8);       })},    // dsra       a0,a0,0x8
                { "dsra32", { 0x3f, 0x14, 0x02, 0x00}, ([](std::ostringstream& buf) { dsra32(buf,v0,v0,0x10);    })},    // dsra32     v0,v0,0x10
                { "dsrl",   { 0x7a, 0x18, 0x04, 0x00}, ([](std::ostringstream& buf) { dsrl(buf,v1,a0,0x1);       })},    // dsrl       v1,a0,0x1
                { "dsrl32", { 0x7e, 0x18, 0x03, 0x00}, ([](std::ostringstream& buf) { dsrl32(buf,v1,v1,0x1);     })},    // dsrl32     v1,v1,0x1
                { "dsrlv",  { 0x16, 0x28, 0x65, 0x00}, ([](std::ostringstream& buf) { dsrlv(buf,a1,a1,v1);       })},    // dsrlv      a1,a1,v1
                { "dsubu",  { 0x2f, 0x48, 0x28, 0x01}, ([](std::ostringstream& buf) { dsubu(buf,t1,t1,t0);       })},    // dsubu      t1,t1,t0
                { "ei",     { 0x38, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { ei(buf);                   })},    // ei
                { "eret",   { 0x18, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { eret(buf);                 })},    // eret
                { "j",      { 0x4b, 0x00, 0x04, 0x08}, ([](std::ostringstream& buf) { j(buf,0x4b);               })},    // j          lab0010012c
                { "jalr",   { 0x09, 0xf8, 0x20, 0x03}, ([](std::ostringstream& buf) { jalr(buf,t9);              })},    // jalr       t9
                { "jr",     { 0x08, 0x00, 0xe0, 0x03}, ([](std::ostringstream& buf) { jr(buf,ra);                })},    // jr         ra
                { "lb",     { 0x00, 0x00, 0x22, 0x82}, ([](std::ostringstream& buf) { lb(buf,v0,s1,0x0);         })},    // lb         v0,0x0(s1)
                { "lbu",    { 0x00, 0x00, 0x0a, 0x91}, ([](std::ostringstream& buf) { lbu(buf,t2,t0,0x0);        })},    // lbu        t2,0x0(t0)
                { "ld",     { 0x90, 0x9d, 0x83, 0xdf}, ([](std::ostringstream& buf) { ld(buf,v1,gp,-0x6270);     })},    // ld         v1,-0x6270(gp)=>dat004f1980                     = ffffffff00000000h
                { "ldl",    { 0x07, 0x00, 0x49, 0x68}, ([](std::ostringstream& buf) { ldl(buf,t1,v0,0x7);        })},    // ldl        t1,0x7(v0)
                { "ldr",    { 0x00, 0x00, 0x49, 0x6c}, ([](std::ostringstream& buf) { ldr(buf,t1,v0,0x0);        })},    // ldr        t1,0x0(v0)
                { "lh",     { 0x32, 0x00, 0xa5, 0x84}, ([](std::ostringstream& buf) { lh(buf,a1,a1,0x32);        })},    // lh         a1,0x32(a1)
                { "lhu",    { 0x6c, 0x00, 0x02, 0x96}, ([](std::ostringstream& buf) { lhu(buf,v0,s0,0x6c);       })},    // lhu        v0,0x6c(s0)
                { "li",     { 0x3c, 0x00, 0x03, 0x24}, ([](std::ostringstream& buf) { li(buf,v1,0x3c);           })},    // li         v1,0x3c
                { "lui",    { 0x4f, 0x00, 0x02, 0x3c}, ([](std::ostringstream& buf) { lui(buf,v0,0x4f);          })},    // lui        v0,0x4f
                { "lw",     { 0x00, 0x00, 0x44, 0x8c}, ([](std::ostringstream& buf) { lw(buf,a0,v0,0x0);         })},    // lw         a0,0x0(v0)
                { "lwc1",   { 0x98, 0x9d, 0x80, 0xc7}, ([](std::ostringstream& buf) { lwc1(buf,f0,gp,-0x6268);   })},    // lwc1       f0,-0x6268(gp)=>->fun00100280                   = 00100280
                { "lwl",    { 0x03, 0x00, 0x50, 0x88}, ([](std::ostringstream& buf) { lwl(buf,s0,v0,0x3);        })},    // lwl        s0,0x3(v0)
                { "lwr",    { 0x00, 0x00, 0x50, 0x98}, ([](std::ostringstream& buf) { lwr(buf,s0,v0,0x0);        })},    // lwr        s0,0x0(v0)
                { "lwu",    { 0xac, 0x00, 0xa7, 0x9f}, ([](std::ostringstream& buf) { lwu(buf,a3,sp,0xac);       })},    // lwu        a3,0xac(sp)
                { "madd",   { 0x00, 0x00, 0x44, 0x70}, ([](std::ostringstream& buf) { madd(buf,v0,a0);           })},    // madd       v0,a0
                { "madds",  { 0x1c, 0x10, 0x02, 0x46}, ([](std::ostringstream& buf) { madds(buf,f0,f2,f2);       })},    // madds     f0,f2,f2
                { "maxs",   { 0x28, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { maxs(buf,f0,f1,f0);        })},    // maxs      f0,f1,f0
                { "mfc1",   { 0x00, 0xf0, 0x02, 0x44}, ([](std::ostringstream& buf) { mfc1(buf,v0,f30);          })},    // mfc1       v0,f30
                { "mfhi",   { 0x10, 0x10, 0x00, 0x00}, ([](std::ostringstream& buf) { mfhi(buf,v0);              })},    // mfhi       v0
                { "mfhi1",  { 0x10, 0x10, 0x00, 0x70}, ([](std::ostringstream& buf) { mfhi1(buf,v0);             })},    // mfhi1      v0
                { "mflo1",  { 0x12, 0x10, 0x00, 0x70}, ([](std::ostringstream& buf) { mflo1(buf,v0);             })},    // mflo1      v0
                { "mfsa",   { 0x28, 0x10, 0x00, 0x00}, ([](std::ostringstream& buf) { mfsa(buf,v0);              })},    // mfsa       v0
                { "mins",   { 0x29, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { mins(buf,f0,f1,f0);        })},    // mins      f0,f1,f0
                { "movn",   { 0x0b, 0x20, 0x41, 0x00}, ([](std::ostringstream& buf) { movn(buf,a0,v0,at);        })},    // movn       a0,v0,at
                { "movs",   { 0x86, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { movs(buf,f2,f1);           })},    // movs      f2,f1
                { "movz",   { 0x0a, 0x20, 0x02, 0x00}, ([](std::ostringstream& buf) { movz(buf,a0,zero,v0);      })},    // movz       a0,zero,v0
                { "msubs",  { 0xdd, 0x1a, 0x03, 0x46}, ([](std::ostringstream& buf) { msubs(buf,f11,f3,f3);      })},    // msubs     f11,f3,f3
                { "mtc1",   { 0x00, 0xb8, 0x82, 0x44}, ([](std::ostringstream& buf) { mtc1(buf,v0,f23);          })},    // mtc1       v0,f23
                { "mthi",   { 0x11, 0x00, 0x40, 0x00}, ([](std::ostringstream& buf) { mthi(buf,v0);              })},    // mthi       v0
                { "mthi1",  { 0x11, 0x00, 0x40, 0x70}, ([](std::ostringstream& buf) { mthi1(buf,v0);             })},    // mthi1      v0
                { "mtlo",   { 0x13, 0x00, 0x40, 0x00}, ([](std::ostringstream& buf) { mtlo(buf,v0);              })},    // mtlo       v0
                { "mtlo1",  { 0x13, 0x00, 0x40, 0x70}, ([](std::ostringstream& buf) { mtlo1(buf,v0);             })},    // mtlo1      v0
                { "mtsa",   { 0x29, 0x00, 0x40, 0x00}, ([](std::ostringstream& buf) { mtsa(buf,v0);              })},    // mtsa       v0
                { "mulas",  { 0x1a, 0x00, 0x00, 0x46}, ([](std::ostringstream& buf) { mulas(buf,f0,f0);          })},    // mulas     f0,f0
                { "muls",   { 0x02, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { muls(buf,f0,f1,f0);        })},    // muls      f0,f1,f0
                { "mult",   { 0x18, 0x20, 0x85, 0x00}, ([](std::ostringstream& buf) { mult(buf,a0,a0,a1);        })},    // mult       a0,a0,a1
                { "mult1",  { 0x18, 0x18, 0x63, 0x72}, ([](std::ostringstream& buf) { mult1(buf,v1,s3,v1);       })},    // mult1      v1,s3,v1
                { "multu",  { 0x19, 0x00, 0x43, 0x00}, ([](std::ostringstream& buf) { multu(buf,v0,v1);          })},    // multu      v0,v1
                { "multu1", { 0x19, 0x00, 0x02, 0x72}, ([](std::ostringstream& buf) { multu1(buf,s0,v0);         })},    // multu1     s0,v0
                { "negs",   { 0x47, 0xb5, 0x00, 0x46}, ([](std::ostringstream& buf) { negs(buf,f21,f22);         })},    // negs     f21,f22
                { "nop",    { 0x00, 0x00, 0x00, 0x00}, ([](std::ostringstream& buf) { nop(buf);                  })},    // nop
                { "nor",    { 0x27, 0x20, 0x00, 0x02}, ([](std::ostringstream& buf) { nor(buf,a0,s0,zero);       })},    // nor        a0,s0,zero
                { "or",     { 0x25, 0x18, 0x62, 0x00}, ([](std::ostringstream& buf) { _or(buf,v1,v1,v0);         })},    // or         v1,v1,v0
                { "ori",    { 0xf0, 0xff, 0x84, 0x34}, ([](std::ostringstream& buf) { ori(buf,a0,a0,0xfff0);     })},    // ori        a0,a0,0xfff0
                { "paddsw", { 0x08, 0x24, 0x83, 0x70}, ([](std::ostringstream& buf) { paddsw(buf,a0,a0,v1);      })},    // paddsw     a0,a0,v1
                { "paddw",  { 0x08, 0x48, 0x66, 0x72}, ([](std::ostringstream& buf) { paddw(buf,t1,s3,a2);       })},    // paddw      t1,s3,a2
                { "pand",   { 0x89, 0x14, 0x43, 0x70}, ([](std::ostringstream& buf) { pand(buf,v0,v0,v1);        })},    // pand       v0,v0,v1
                { "pcpyh",  { 0xe9, 0x46, 0x0a, 0x70}, ([](std::ostringstream& buf) { pcpyh(buf,t0,t2);          })},    // pcpyh      t0,t2
                { "pcpyld", { 0x89, 0x4b, 0x29, 0x71}, ([](std::ostringstream& buf) { pcpyld(buf,t1,t1,t1);      })},    // pcpyld     t1,t1,t1
                { "pcpyud", { 0xa9, 0x1b, 0x49, 0x70}, ([](std::ostringstream& buf) { pcpyud(buf,v1,v0,t1);      })},    // pcpyud     v1,v0,t1
                { "pexch",  { 0xa9, 0x26, 0x04, 0x70}, ([](std::ostringstream& buf) { pexch(buf,a0,a0);          })},    // pexch      a0,a0
                { "pextlb", { 0x88, 0x46, 0x08, 0x70}, ([](std::ostringstream& buf) { pextlb(buf,t0,zero,t0);    })},    // pextlb     t0,zero,t0
                { "pextlh", { 0x88, 0x45, 0x06, 0x70}, ([](std::ostringstream& buf) { pextlh(buf,t0,zero,a2);    })},    // pextlh     t0,zero,a2
                { "pextlw", { 0x88, 0x4c, 0x29, 0x71}, ([](std::ostringstream& buf) { pextlw(buf,t1,t1,t1);      })},    // pextlw     t1,t1,t1
                { "pextuw", { 0xa8, 0x6c, 0x28, 0x71}, ([](std::ostringstream& buf) { pextuw(buf,t5,t1,t0);      })},    // pextuw     t5,t1,t0
                { "pmaxh",  { 0xc8, 0xa1, 0x80, 0x72}, ([](std::ostringstream& buf) { pmaxh(buf,s4,s4,zero);     })},    // pmaxh      s4,s4,zero
                { "pmaxw",  { 0xc8, 0x70, 0xc9, 0x71}, ([](std::ostringstream& buf) { pmaxw(buf,t6,t6,t1);       })},    // pmaxw      t6,t6,t1
                { "pmfhi",  { 0x09, 0x22, 0x00, 0x70}, ([](std::ostringstream& buf) { pmfhi(buf,a0);             })},    // pmfhi      a0
                { "pmflo",  { 0x49, 0x32, 0x00, 0x70}, ([](std::ostringstream& buf) { pmflo(buf,a2);             })},    // pmflo      a2
                { "pminh",  { 0xe8, 0xa1, 0x90, 0x72}, ([](std::ostringstream& buf) { pminh(buf,s4,s4,s0);       })},    // pminh      s4,s4,s0
                { "pminw",  { 0xe8, 0x58, 0x6a, 0x71}, ([](std::ostringstream& buf) { pminw(buf,t3,t3,t2);       })},    // pminw      t3,t3,t2
                { "pmulth", { 0x09, 0x27, 0x87, 0x70}, ([](std::ostringstream& buf) { pmulth(buf,a0,a0,a3);      })},    // pmulth     a0,a0,a3
                { "pnor",   { 0xe9, 0x1c, 0x02, 0x70}, ([](std::ostringstream& buf) { pnor(buf,v1,zero,v0);      })},    // pnor       v1,zero,v0
                { "ppach",  { 0xc8, 0x4d, 0x09, 0x71}, ([](std::ostringstream& buf) { ppach(buf,t1,t0,t1);       })},    // ppach      t1,t0,t1
                { "psllw",  { 0xbc, 0x30, 0x0a, 0x70}, ([](std::ostringstream& buf) { psllw(buf,a2,t2,0x2);      })},    // psllw      a2,t2,0x2
                { "psraw",  { 0xbf, 0x2b, 0x05, 0x70}, ([](std::ostringstream& buf) { psraw(buf,a1,a1,0xe);      })},    // psraw      a1,a1,0xe
                { "psrlw",  { 0x3e, 0x44, 0x09, 0x70}, ([](std::ostringstream& buf) { psrlw(buf,t0,t1,0x10);     })},    // psrlw      t0,t1,0x10
                { "psubb",  { 0x48, 0x12, 0x4a, 0x70}, ([](std::ostringstream& buf) { psubb(buf,v0,v0,t2);       })},    // psubb      v0,v0,t2
                { "psubw",  { 0x48, 0x38, 0x43, 0x70}, ([](std::ostringstream& buf) { psubw(buf,a3,v0,v1);       })},    // psubw      a3,v0,v1
                { "pxor",   { 0xc9, 0x14, 0x49, 0x70}, ([](std::ostringstream& buf) { pxor(buf,v0,v0,t1);        })},    // pxor       v0,v0,t1
                { "rsqrts", { 0x96, 0x28, 0x00, 0x46}, ([](std::ostringstream& buf) { rsqrts(buf,f2,f5,f0);      })},    // rsqrts    f2,f5,f0
                { "sb",     { 0x00, 0x00, 0x40, 0xa0}, ([](std::ostringstream& buf) { sb(buf,zero,v0,0x0);       })},    // sb         zero,0x0(v0)
                { "sh",     { 0x16, 0x00, 0x03, 0xa6}, ([](std::ostringstream& buf) { sh(buf,v1,s0,0x16);        })},    // sh         v1,0x16(s0)
                { "sll",    { 0x40, 0x40, 0x0a, 0x00}, ([](std::ostringstream& buf) { sll(buf,t0,t2,0x1);        })},    // sll        t0,t2,0x1
                { "sllv",   { 0x04, 0x20, 0x83, 0x00}, ([](std::ostringstream& buf) { sllv(buf,a0,v1,a0);        })},    // sllv       a0,v1,a0
                { "slt",    { 0x2a, 0x08, 0x44, 0x00}, ([](std::ostringstream& buf) { slt(buf,at,v0,a0);         })},    // slt        at,v0,a0
                { "slti",   { 0x61, 0x00, 0x62, 0x28}, ([](std::ostringstream& buf) { slti(buf,v0,v1,0x61);      })},    // slti       v0,v1,0x61
                { "sltiu",  { 0x01, 0x00, 0x42, 0x2c}, ([](std::ostringstream& buf) { sltiu(buf,v0,v0,0x1);      })},    // sltiu      v0,v0,0x1
                { "sltu",   { 0x2b, 0x10, 0x02, 0x00}, ([](std::ostringstream& buf) { sltu(buf,v0,zero,v0);      })},    // sltu       v0,zero,v0
                { "sq",     { 0x00, 0x00, 0x40, 0x7c}, ([](std::ostringstream& buf) { sq(buf,zero,v0,0x0);       })},    // sq         zero,0x0(v0)
                { "sqrts",  { 0x04, 0x00, 0x00, 0x46}, ([](std::ostringstream& buf) { sqrts(buf,f0,f0);          })},    // sqrts     f0,f0
                { "sra",    { 0xc3, 0x10, 0x03, 0x00}, ([](std::ostringstream& buf) { sra(buf,v0,v1,0x3);        })},    // sra       v0,v1,0x3
                { "srav",   { 0x07, 0x60, 0x68, 0x01}, ([](std::ostringstream& buf) { srav(buf,t4,t0,t3);        })},    // srav      t4,t0,t3
                { "srl",    { 0x02, 0x1c, 0x02, 0x00}, ([](std::ostringstream& buf) { srl(buf,v1,v0,0x10);       })},    // srl        v1,v0,0x10
                { "srlv",   { 0x06, 0x18, 0x64, 0x00}, ([](std::ostringstream& buf) { srlv(buf,v1,a0,v1);        })},    // srlv       v1,a0,v1
                { "sub",    { 0x22, 0x10, 0x04, 0x00}, ([](std::ostringstream& buf) { sub(buf,v0,zero,a0);       })},    // sub        v0,zero,a0
                { "subs",   { 0x41, 0x10, 0x01, 0x46}, ([](std::ostringstream& buf) { subs(buf,f1,f2,f1);        })},    // subs      f1,f2,f1
                { "subu",   { 0x23, 0x10, 0x54, 0x02}, ([](std::ostringstream& buf) { subu(buf,v0,s2,s4);        })},    // subu      v0,s2,s4
                { "sw",     { 0x08, 0x00, 0x89, 0xac}, ([](std::ostringstream& buf) { sw(buf,t1,a0,0x8);         })},    // sw         t1,0x8(a0)
                { "syscall",{ 0x0c, 0x00, 0x00, 0x00}, ([](std::ostringstream& buf) { syscall(buf);              })},    // syscall
                { "tlbwi",  { 0x02, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { tlbwi(buf);                })},    // tlbwi
                { "xor",    { 0x26, 0x10, 0x62, 0x00}, ([](std::ostringstream& buf) { _xor(buf,v0,v1,v0);        })},    // xor        v0,v1,v0
                { "xori",   { 0x01, 0x00, 0x42, 0x38}, ([](std::ostringstream& buf) { xori(buf,v0,v0,0x1);       })},    // xori       v0,v0,0x1
                { "abss",   { 0x05, 0xa3, 0x00, 0x46}, ([](std::ostringstream& buf) { abss(buf,f12,f20);         })},    // abss     f12,f20
                { "add",    { 0x20, 0x10, 0x43, 0x00}, ([](std::ostringstream& buf) { add(buf,v0,v0,v1);         })},    // add        v0,v0,v1
                { "addas",  { 0x18, 0x00, 0x01, 0x46}, ([](std::ostringstream& buf) { addas(buf,f0,f1);          })},    // addas     f0,f1
                { "addi",   { 0xff, 0xff, 0xe7, 0x20}, ([](std::ostringstream& buf) { addi(buf,a3,a3,-0x1);      })},    // addi       a3,a3,-0x1
                { "addiu",  { 0x80, 0x26, 0x42, 0x24}, ([](std::ostringstream& buf) { addiu(buf,v0,v0,0x2680);   })},    // addiu      v0,v0,0x2680
                { "adds",   { 0x40, 0x00, 0x00, 0x46}, ([](std::ostringstream& buf) { adds(buf,f1,f0,f0);        })},    // adds      f1,f0,f0
                { "addu",   { 0x21, 0x90, 0x64, 0x00}, ([](std::ostringstream& buf) { addu(buf,s2,v1,a0);        })},    // addu      s2,v1,a0
                { "and",    { 0x24, 0x20, 0x64, 0x00}, ([](std::ostringstream& buf) { _and(buf,a0,v1,a0);        })},    // and        a0,v1,a0
                { "andi",   { 0x0f, 0x00, 0x44, 0x30}, ([](std::ostringstream& buf) { andi(buf,a0,v0,0xf);       })},    // andi       a0,v0,0xf
                { "b",      { 0x0c, 0x00, 0x00, 0x10}, ([](std::ostringstream& buf) { b(buf,0x0c);               })},    // b          lab0010036c
                { "bc0f",   { 0xfa, 0xff, 0x00, 0x41}, ([](std::ostringstream& buf) { bc0f(buf,0xfa);            })},    // bc0f       lab001f21b8
                { "bc1f",   { 0x07, 0x00, 0x00, 0x45}, ([](std::ostringstream& buf) { bc1f(buf,0x07);            })},    // bc1f       lab0010debc
                { "bc1fl",  { 0x0f, 0x00, 0x02, 0x45}, ([](std::ostringstream& buf) { bc1fl(buf,0x0f);           })},    // bc1fl      lab0011a6b8
                { "bc1tl",  { 0x01, 0x00, 0x03, 0x45}, ([](std::ostringstream& buf) { bc1tl(buf,0x01);           })},    // bc1tl      lab0011c90c
                { "beq",    { 0x06, 0x00, 0x80, 0x10}, ([](std::ostringstream& buf) { beq(buf,a0,zero,0x06);     })},    // beq        a0,zero,lab0010014c
                { "bne",    { 0x1b, 0x00, 0x60, 0x14}, ([](std::ostringstream& buf) { bne(buf,v1,zero,0x1b);     })},    // bne        v1,zero,lab00100340
                { "ceqs",   { 0x32, 0x60, 0x00, 0x46}, ([](std::ostringstream& buf) { ceqs(buf,f12,f0);          })},    // ceqs     f12,f0
                { "cles",   { 0x36, 0x38, 0x00, 0x46}, ([](std::ostringstream& buf) { cles(buf,f7,f0);           })},    // cles     f7,f0
                { "clts",   { 0x34, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { clts(buf,f1,f0);           })},    // clts     f1,f0
                { "cvtsw",  { 0x60, 0x4a, 0x80, 0x46}, ([](std::ostringstream& buf) { cvtsw(buf,f9,f9);          })},    // cvtsw    f9,f9
                { "cvtws",  { 0xa4, 0x00, 0x00, 0x46}, ([](std::ostringstream& buf) { cvtws(buf,f2,f0);          })},    // cvtws    f2,f0
                { "daddiu", { 0xd0, 0xff, 0x68, 0x64}, ([](std::ostringstream& buf) { daddiu(buf,t0,v1,-0x30);   })},    // daddiu    t0,v1,-0x30
                { "daddu",  { 0x2d, 0x18, 0x68, 0x00}, ([](std::ostringstream& buf) { daddu(buf,v1,v1,t0);       })},    // daddu      v1,v1,t0
                { "di",     { 0x39, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { di(buf);                   })},    // di
                { "div",    { 0x1a, 0x00, 0x70, 0x00}, ([](std::ostringstream& buf) { div(buf,v1,s0);            })},    // div        v1,s0
                { "div1",   { 0x1a, 0x00, 0xc3, 0x70}, ([](std::ostringstream& buf) { div1(buf,a2,v1);           })},    // div1       a2,v1
                { "divs",   { 0x03, 0x00, 0x01, 0x46}, ([](std::ostringstream& buf) { divs(buf,f0,f0,f1);        })},    // divs      f0,f0,f1
                { "divu",   { 0x1b, 0x00, 0x47, 0x00}, ([](std::ostringstream& buf) { divu(buf,v0,a3);           })},    // divu       v0,a3
                { "dsll",   { 0xb8, 0x18, 0x08, 0x00}, ([](std::ostringstream& buf) { dsll(buf,v1,t0,0x2);       })},    // dsll       v1,t0,0x2
                { "dsll32", { 0x3c, 0x14, 0x05, 0x00}, ([](std::ostringstream& buf) { dsll32(buf,v0,a1,0x10);    })},    // dsll32     v0,a1,0x10
                { "dsllv",  { 0x14, 0x28, 0x45, 0x00}, ([](std::ostringstream& buf) { dsllv(buf,a1,a1,v0);       })},    // dsllv      a1,a1,v0
                { "dsra",   { 0x7b, 0x13, 0x02, 0x00}, ([](std::ostringstream& buf) { dsra(buf,v0,v0,0xd);       })},    // dsra       v0,v0,0xd
                { "dsra32", { 0x3f, 0x14, 0x02, 0x00}, ([](std::ostringstream& buf) { dsra32(buf,v0,v0,0x10);    })},    // dsra32     v0,v0,0x10
                { "dsrl",   { 0x3a, 0x14, 0x02, 0x00}, ([](std::ostringstream& buf) { dsrl(buf,v0,v0,0x10);      })},    // dsrl       v0,v0,0x10
                { "dsrl32", { 0x3e, 0x48, 0x09, 0x00}, ([](std::ostringstream& buf) { dsrl32(buf,t1,t1,0x0);     })},    // dsrl32     t1,t1,0x0
                { "dsrlv",  { 0x16, 0x28, 0x65, 0x00}, ([](std::ostringstream& buf) { dsrlv(buf,a1,a1,v1);       })},    // dsrlv      a1,a1,v1
                { "dsubu",  { 0x2f, 0x10, 0x82, 0x00}, ([](std::ostringstream& buf) { dsubu(buf,v0,a0,v0);       })},    // dsubu      v0,a0,v0
                { "ei",     { 0x38, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { ei(buf);                   })},    // ei
                { "eret",   { 0x18, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { eret(buf);                 })},    // eret
                { "j",      { 0x4b, 0x00, 0x04, 0x08}, ([](std::ostringstream& buf) { j(buf,0x4b);               })},    // j          lab0010012c
                { "jalr",   { 0x09, 0xf8, 0x40, 0x00}, ([](std::ostringstream& buf) { jalr(buf,v0);              })},    // jalr       v0
                { "jr",     { 0x08, 0x00, 0xe0, 0x03}, ([](std::ostringstream& buf) { jr(buf,ra);                })},    // jr         ra
                { "lb",     { 0x00, 0x00, 0xa7, 0x80}, ([](std::ostringstream& buf) { lb(buf,a3,a1,0x0);         })},    // lb         a3,0x0(a1)
                { "lbu",    { 0x00, 0x00, 0x83, 0x90}, ([](std::ostringstream& buf) { lbu(buf,v1,a0,0x0);        })},    // lbu        v1,0x0(a0)
                { "ldl",    { 0x07, 0x00, 0x22, 0x69}, ([](std::ostringstream& buf) { ldl(buf,v0,t1,0x7);        })},    // ldl        v0,0x7(t1)
                { "ldr",    { 0x00, 0x00, 0x22, 0x6d}, ([](std::ostringstream& buf) { ldr(buf,v0,t1,0x0);        })},    // ldr        v0,0x0(t1)
                { "lh",     { 0x02, 0x00, 0x24, 0x86}, ([](std::ostringstream& buf) { lh(buf,a0,s1,0x2);         })},    // lh         a0,0x2(s1)
                { "lhu",    { 0x02, 0x00, 0x24, 0x96}, ([](std::ostringstream& buf) { lhu(buf,a0,s1,0x2);        })},    // lhu        a0,0x2(s1)
                { "li",     { 0x3c, 0x00, 0x03, 0x24}, ([](std::ostringstream& buf) { li(buf,v1,0x3c);           })},    // li         v1,0x3c
                { "lui",    { 0x4e, 0x00, 0x02, 0x3c}, ([](std::ostringstream& buf) { lui(buf,v0,0x4e);          })},    // lui        v0,0x4e
                { "lw",     { 0x00, 0x00, 0x44, 0x8c}, ([](std::ostringstream& buf) { lw(buf,a0,v0,0x0);         })},    // lw         a0,0x0(v0)
                { "lwc1",   { 0x14, 0x00, 0xa0, 0xc4}, ([](std::ostringstream& buf) { lwc1(buf,f0,a1,0x14);      })},    // lwc1       f0,0x14(a1)
                { "lwl",    { 0x03, 0x00, 0x02, 0x8a}, ([](std::ostringstream& buf) { lwl(buf,v0,s0,0x3);        })},    // lwl        v0,0x3(s0)
                { "lwr",    { 0x00, 0x00, 0x02, 0x9a}, ([](std::ostringstream& buf) { lwr(buf,v0,s0,0x0);        })},    // lwr        v0,0x0(s0)
                { "lwu",    { 0x20, 0x02, 0x85, 0x9c}, ([](std::ostringstream& buf) { lwu(buf,a1,a0,0x220);      })},    // lwu        a1,0x220(a0)
                { "madd",   { 0x00, 0x00, 0x43, 0x70}, ([](std::ostringstream& buf) { madd(buf,v0,v1);           })},    // madd       v0,v1
                { "maddas", { 0x1e, 0x08, 0x02, 0x46}, ([](std::ostringstream& buf) { maddas(buf,f1,f2);         })},    // maddas    f1,f2
                { "madds",  { 0x9c, 0x10, 0x00, 0x46}, ([](std::ostringstream& buf) { madds(buf,f2,f2,f0);       })},    // madds     f2,f2,f0
                { "maddu",  { 0x01, 0x78, 0xa3, 0x71}, ([](std::ostringstream& buf) { maddu(buf,t7,t5,v1);       })},    // maddu      t7,t5,v1
                { "maxs",   { 0x68, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { maxs(buf,f1,f1,f0);        })},    // maxs      f1,f1,f0
                { "mfc1",   { 0x00, 0x58, 0x04, 0x44}, ([](std::ostringstream& buf) { mfc1(buf,a0,f11);          })},    // mfc1       a0,f11
                { "mfhi",   { 0x10, 0x28, 0x00, 0x00}, ([](std::ostringstream& buf) { mfhi(buf,a1);              })},    // mfhi       a1
                { "mfhi1",  { 0x10, 0x10, 0x00, 0x70}, ([](std::ostringstream& buf) { mfhi1(buf,v0);             })},    // mfhi1      v0
                { "mflo",   { 0x12, 0x18, 0x00, 0x00}, ([](std::ostringstream& buf) { mflo(buf,v1);              })},    // mflo       v1
                { "mflo1",  { 0x12, 0x10, 0x00, 0x70}, ([](std::ostringstream& buf) { mflo1(buf,v0);             })},    // mflo1      v0
                { "mfsa",   { 0x28, 0x10, 0x00, 0x00}, ([](std::ostringstream& buf) { mfsa(buf,v0);              })},    // mfsa       v0
                { "mins",   { 0x69, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { mins(buf,f1,f1,f0);        })},    // mins      f1,f1,f0
                { "moveq",  { 0xa9, 0x44, 0x40, 0x71}, ([](std::ostringstream& buf) { moveq(buf,t0,t2);          })},    // moveq      t0,t2
                { "movn",   { 0x0b, 0x80, 0x62, 0x00}, ([](std::ostringstream& buf) { movn(buf,s0,v1,v0);        })},    // movn       s0,v1,v0
                { "movs",   { 0x06, 0x03, 0x00, 0x46}, ([](std::ostringstream& buf) { movs(buf,f12,f0);          })},    // movs      f12,f0
                { "movz",   { 0x0a, 0x98, 0x02, 0x00}, ([](std::ostringstream& buf) { movz(buf,s3,zero,v0);      })},    // movz      s3,zero,v0
                { "msubas", { 0x1f, 0x48, 0x03, 0x46}, ([](std::ostringstream& buf) { msubas(buf,f9,f3);         })},    // msubas    f9,f3
                { "msubs",  { 0xdd, 0x08, 0x00, 0x46}, ([](std::ostringstream& buf) { msubs(buf,f3,f1,f0);       })},    // msubs    f3,f1,f0
                { "mtc1",   { 0x00, 0x00, 0x81, 0x44}, ([](std::ostringstream& buf) { mtc1(buf,at,f0);           })},    // mtc1       at,f0
                { "mthi",   { 0x11, 0x00, 0x40, 0x00}, ([](std::ostringstream& buf) { mthi(buf,v0);              })},    // mthi       v0
                { "mthi1",  { 0x11, 0x00, 0x40, 0x70}, ([](std::ostringstream& buf) { mthi1(buf,v0);             })},    // mthi1      v0
                { "mtlo",   { 0x13, 0x00, 0xe0, 0x00}, ([](std::ostringstream& buf) { mtlo(buf,a3);              })},    // mtlo       a3
                { "mtlo1",  { 0x13, 0x00, 0x40, 0x70}, ([](std::ostringstream& buf) { mtlo1(buf,v0);             })},    // mtlo1      v0
                { "mtsa",   { 0x29, 0x00, 0x40, 0x00}, ([](std::ostringstream& buf) { mtsa(buf,v0);              })},    // mtsa       v0
                { "mtsab",  { 0x00, 0x00, 0xb8, 0x05}, ([](std::ostringstream& buf) { mtsab(buf,t5,0x0);         })},    // mtsab      t5,0x0
                { "mulas",  { 0x1a, 0x20, 0x03, 0x46}, ([](std::ostringstream& buf) { mulas(buf,f4,f3);          })},    // mulas     f4,f3
                { "muls",   { 0x42, 0x5d, 0x0b, 0x46}, ([](std::ostringstream& buf) { muls(buf,f21,f11,f11);     })},    // muls      f21,f11,f11
                { "mult",   { 0x18, 0x20, 0x85, 0x00}, ([](std::ostringstream& buf) { mult(buf,a0,a0,a1);        })},    // mult       a0,a0,a1
                { "mult1",  { 0x18, 0x10, 0xa3, 0x72}, ([](std::ostringstream& buf) { mult1(buf,v0,s5,v1);       })},    // mult1      v0,s5,v1
                { "multu",  { 0x19, 0x00, 0x43, 0x00}, ([](std::ostringstream& buf) { multu(buf,v0,v1);          })},    // multu      v0,v1
                { "multu1", { 0x19, 0x00, 0x02, 0x72}, ([](std::ostringstream& buf) { multu1(buf,s0,v0);         })},    // multu1     s0,v0
                { "negs",   { 0x07, 0xa0, 0x00, 0x46}, ([](std::ostringstream& buf) { negs(buf,f0,f20);          })},    // negs      f0,f20
                { "nop",    { 0x00, 0x00, 0x00, 0x00}, ([](std::ostringstream& buf) { nop(buf);                  })},    // nop
                { "nor",    { 0x27, 0x10, 0x06, 0x00}, ([](std::ostringstream& buf) { nor(buf,v0,zero,a2);       })},    // nor        v0,zero,a2
                { "or",     { 0x25, 0x18, 0xc3, 0x00}, ([](std::ostringstream& buf) { _or(buf,v1,a2,v1);         })},    // or         v1,a2,v1
                { "ori",    { 0xf0, 0xff, 0x84, 0x34}, ([](std::ostringstream& buf) { ori(buf,a0,a0,0xfff0);     })},    // ori        a0,a0,0xfff0
                { "paddh",  { 0x08, 0x11, 0x0a, 0x71}, ([](std::ostringstream& buf) { paddh(buf,v0,t0,t2);       })},    // paddh      v0,t0,t2
                { "paddw",  { 0x08, 0x90, 0x12, 0x71}, ([](std::ostringstream& buf) { paddw(buf,s2,t0,s2);       })},    // paddw      s2,t0,s2
                { "pand",   { 0x89, 0x14, 0x43, 0x70}, ([](std::ostringstream& buf) { pand(buf,v0,v0,v1);        })},    // pand       v0,v0,v1
                { "pcgth",  { 0x88, 0x49, 0x40, 0x71}, ([](std::ostringstream& buf) { pcgth(buf,t1,t2,zero);     })},    // pcgth      t1,t2,zero
                { "pcpyh",  { 0xe9, 0x46, 0x0a, 0x70}, ([](std::ostringstream& buf) { pcpyh(buf,t0,t2);          })},    // pcpyh      t0,t2
                { "pcpyld", { 0x89, 0x33, 0x02, 0x70}, ([](std::ostringstream& buf) { pcpyld(buf,a2,zero,v0);    })},    // pcpyld     a2,zero,v0
                { "pcpyud", { 0xa9, 0x1b, 0x49, 0x70}, ([](std::ostringstream& buf) { pcpyud(buf,v1,v0,t1);      })},    // pcpyud     v1,v0,t1
                { "pextlb", { 0x88, 0x46, 0x0a, 0x70}, ([](std::ostringstream& buf) { pextlb(buf,t0,zero,t2);    })},    // pextlb     t0,zero,t2
                { "pextlh", { 0x88, 0x55, 0x03, 0x70}, ([](std::ostringstream& buf) { pextlh(buf,t2,zero,v1);    })},    // pextlh     t2,zero,v1
                { "pextlw", { 0x88, 0x24, 0x03, 0x70}, ([](std::ostringstream& buf) { pextlw(buf,a0,zero,v1);    })},    // pextlw     a0,zero,v1
                { "pextub", { 0xa8, 0x4e, 0x0a, 0x70}, ([](std::ostringstream& buf) { pextub(buf,t1,zero,t2);    })},    // pextub     t1,zero,t2
                { "pmaxh",  { 0xc8, 0x41, 0x00, 0x71}, ([](std::ostringstream& buf) { pmaxh(buf,t0,t0,zero);     })},    // pmaxh      t0,t0,zero
                { "pmaxw",  { 0xc8, 0x20, 0x86, 0x70}, ([](std::ostringstream& buf) { pmaxw(buf,a0,a0,a2);       })},    // pmaxw      a0,a0,a2
                { "pminh",  { 0xe8, 0x41, 0x0b, 0x71}, ([](std::ostringstream& buf) { pminh(buf,t0,t0,t3);       })},    // pminh      t0,t0,t3
                { "pminw",  { 0xe8, 0x18, 0x83, 0x70}, ([](std::ostringstream& buf) { pminw(buf,v1,a0,v1);       })},    // pminw      v1,a0,v1
                { "pmulth", { 0x09, 0x47, 0x12, 0x71}, ([](std::ostringstream& buf) { pmulth(buf,t0,t0,s2);      })},    // pmulth     t0,t0,s2
                { "pnor",   { 0xe9, 0xcc, 0x00, 0x70}, ([](std::ostringstream& buf) { pnor(buf,t9,zero,zero);    })},    // pnor       t9,zero,zero
                { "por",    { 0xa9, 0x44, 0x04, 0x71}, ([](std::ostringstream& buf) { por(buf,t0,t0,a0);         })},    // por        t0,t0,a0
                { "ppacb",  { 0xc8, 0x56, 0x28, 0x71}, ([](std::ostringstream& buf) { ppacb(buf,t2,t1,t0);       })},    // ppacb      t2,t1,t0
                { "ppach",  { 0xc8, 0x1d, 0x03, 0x70}, ([](std::ostringstream& buf) { ppach(buf,v1,zero,v1);     })},    // ppach      v1,zero,v1
                { "psllh",  { 0x74, 0xc8, 0x19, 0x70}, ([](std::ostringstream& buf) { psllh(buf,t9,t9,0x1);      })},    // psllh      t9,t9,0x1
                { "psrah",  { 0xf7, 0x53, 0x0b, 0x70}, ([](std::ostringstream& buf) { psrah(buf,t2,t3,0xf);      })},    // psrah      t2,t3,0xf
                { "psraw",  { 0xff, 0x27, 0x03, 0x70}, ([](std::ostringstream& buf) { psraw(buf,a0,v1,0x1f);     })},    // psraw      a0,v1,0x1f
                { "psrlh",  { 0xf6, 0xcb, 0x19, 0x70}, ([](std::ostringstream& buf) { psrlh(buf,t9,t9,0xf);      })},    // psrlh      t9,t9,0xf
                { "psubb",  { 0x48, 0x12, 0x4a, 0x70}, ([](std::ostringstream& buf) { psubb(buf,v0,v0,t2);       })},    // psubb      v0,v0,t2
                { "psubw",  { 0x48, 0x38, 0x43, 0x70}, ([](std::ostringstream& buf) { psubw(buf,a3,v0,v1);       })},    // psubw      a3,v0,v1
                { "pxor",   { 0xc9, 0x14, 0x49, 0x70}, ([](std::ostringstream& buf) { pxor(buf,v0,v0,t1);        })},    // pxor       v0,v0,t1
                { "qfsrv",  { 0xe8, 0x56, 0x28, 0x71}, ([](std::ostringstream& buf) { qfsrv(buf,t2,t1,t0);       })},    // qfsrv      t2,t1,t0
                { "rsqrts", { 0x56, 0x08, 0x02, 0x46}, ([](std::ostringstream& buf) { rsqrts(buf,f1,f1,f2);      })},    // rsqrts    f1,f1,f2
                { "sb",     { 0x00, 0x00, 0x40, 0xa0}, ([](std::ostringstream& buf) { sb(buf,zero,v0,0x0);       })},    // sb         zero,0x0(v0)
                { "sh",     { 0x00, 0x00, 0x11, 0xa6}, ([](std::ostringstream& buf) { sh(buf,s1,s0,0x0);         })},    // sh         s1,0x0(s0)
                { "sll",    { 0x00, 0x1a, 0x06, 0x00}, ([](std::ostringstream& buf) { sll(buf,v1,a2,0x8);        })},    // sll        v1,a2,0x8
                { "sllv",   { 0x04, 0x10, 0xae, 0x01}, ([](std::ostringstream& buf) { sllv(buf,v0,t6,t5);        })},    // sllv       v0,t6,t5
                { "slt",    { 0x2a, 0x08, 0x20, 0x01}, ([](std::ostringstream& buf) { slt(buf,at,t1,zero);       })},    // slt        at,t1,zero
                { "slti",   { 0x02, 0x00, 0x82, 0x28}, ([](std::ostringstream& buf) { slti(buf,v0,a0,0x2);       })},    // slti       v0,a0,0x2
                { "sltiu",  { 0x10, 0x00, 0x41, 0x2c}, ([](std::ostringstream& buf) { sltiu(buf,at,v0,0x10);     })},    // sltiu      at,v0,0x10
                { "sltu",   { 0x2b, 0x18, 0x73, 0x00}, ([](std::ostringstream& buf) { sltu(buf,v1,v1,s3);        })},    // sltu       v1,v1,s3
                { "sq",     { 0x00, 0x00, 0x40, 0x7c}, ([](std::ostringstream& buf) { sq(buf,zero,v0,0x0);       })},    // sq         zero,0x0(v0)
                { "sqrts",  { 0x04, 0x00, 0x00, 0x46}, ([](std::ostringstream& buf) { sqrts(buf,f0,f0);          })},    // sqrts     f0,f0
                { "sra",    { 0xc3, 0x10, 0x03, 0x00}, ([](std::ostringstream& buf) { sra(buf,v0,v1,0x3);        })},    // sra        v0,v1,0x3
                { "srl",    { 0xc2, 0x10, 0x03, 0x00}, ([](std::ostringstream& buf) { srl(buf,v0,v1,0x3);        })},    // srl        v0,v1,0x3
                { "srlv",   { 0x06, 0x10, 0x43, 0x00}, ([](std::ostringstream& buf) { srlv(buf,v0,v1,v0);        })},    // srlv       v0,v1,v0
                { "sub",    { 0x22, 0xe8, 0xa2, 0x03}, ([](std::ostringstream& buf) { sub(buf,sp,sp,v0);         })},    // sub       sp,sp,v0
                { "subs",   { 0x01, 0x58, 0x0b, 0x46}, ([](std::ostringstream& buf) { subs(buf,f0,f11,f11);      })},    // subs      f0,f11,f11
                { "subu",   { 0x23, 0x90, 0x43, 0x02}, ([](std::ostringstream& buf) { subu(buf,s2,s2,v1);        })},    // subu       s2,s2,v1
                { "syscall",{ 0x0c, 0x00, 0x00, 0x00}, ([](std::ostringstream& buf) { syscall(buf);              })},    // syscall
                { "tlbwi",  { 0x02, 0x00, 0x00, 0x42}, ([](std::ostringstream& buf) { tlbwi(buf);                })},    // tlbwi
                { "xor",    { 0x26, 0x10, 0xe2, 0x02}, ([](std::ostringstream& buf) { _xor(buf,v0,s7,v0);        })},    // xor        v0,s7,v0
                { "xori",   { 0x01, 0x00, 0x42, 0x38}, ([](std::ostringstream& buf) { xori(buf,v0,v0,0x1);       })},    // xori       v0,v0,0x1
            };

            for (auto var : testDataAdditional)
            {
                checkInstr(var);
            }
        }
    } test;
#endif
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
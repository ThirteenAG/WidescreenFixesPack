/**
 * Copyright (c) 2009-2016 Andrew J. Paroski (https://github.com/paroski)
 *
 * This source file is subject to the OSI MIT license, that is bundled
 * with this package in the file LICENSE.
 *
 * Copyright (c) 2010-2016 Facebook, Inc. (http://www.facebook.com)
 *
 * This source file is subject to version 3.01 of the PHP license,
 * that is bundled with this package in the file LICENSE.PHP, and
 * is available through the world-wide-web at the following url:
 *   http://www.php.net/license/3_01.txt
 *
 * If you did not receive a copy of the PHP license and are unable
 * to obtain it through the world-wide-web, please send a note to
 * license@php.net so we can mail you a copy immediately.
 */

#ifndef ASM_X64_H__
#define ASM_X64_H__

#include <cassert>
#include <cstring>
#include <cstdint>
#include <vector>

#ifndef _MSC_VER
#ifndef LIKELY
#define LIKELY(x)     __builtin_expect(x, 1)
#endif

#ifndef UNLIKELY
#define UNLIKELY(x)   __builtin_expect(x, 0)
#endif

#define __forceinline __attribute__((always_inline))
#define __declspec(noinline) __attribute__((noinline))

#else
#ifndef LIKELY
#define LIKELY(x) x
#endif

#ifndef UNLIKELY
#define UNLIKELY(x) x
#endif
#endif

struct MemoryRef;
struct SimpleMemoryRef;
struct IndexedMemoryRef;
struct IndexedBaselessMemoryRef;
struct RIPRelativeRef;
struct BaselessMemoryRef;

struct ScaledIndex;
struct ScaledIndexDisp;
struct DispReg;
struct DispBaseless;

//////////////////////////////////////////////////////////////////////

namespace sz
{
    constexpr int nosize = 0;
    constexpr int byte = 1;
    constexpr int word = 2;
    constexpr int dword = 4;
    constexpr int qword = 8;
} // namespace

//////////////////////////////////////////////////////////////////////

/*
 * Type for register numbers, independent of the size we're going to
 * be using it as.  Also, the same register number may mean different
 * physical registers for different instructions (e.g. xmm0 and rax
 * are both 0).
 *
 * This type is mainly published for backward compatability with the
 * APIs that look like store_reg##_disp_reg##, which predate the
 * size-specific types.  (Some day it may become internal to this
 * module.)
 */
enum class RegNumber : int {};

struct Reg64
{
    explicit constexpr Reg64(int rn) : rn(rn) {}

    constexpr /* implicit */ operator RegNumber() const { return RegNumber(rn); }

    /**
     * Integer conversion is allowed, but only explicitly.
     */
    explicit constexpr operator int() const { return rn; }
    SimpleMemoryRef operator[](intptr_t disp) const;
    IndexedMemoryRef operator[](Reg64) const;
    IndexedMemoryRef operator[](ScaledIndex) const;
    IndexedMemoryRef operator[](ScaledIndexDisp) const;
    IndexedMemoryRef operator[](DispReg) const;

    constexpr bool operator==(Reg64 o) const { return rn == o.rn; }
    constexpr bool operator!=(Reg64 o) const { return rn != o.rn; }

private:
    int rn;
};

#define SIMPLE_REGTYPE(What)                                        \
  struct What                                                       \
  {                                                                 \
    explicit constexpr What(int rn) : rn(rn) {}                     \
    constexpr /* implicit */ operator RegNumber() const {           \
      return RegNumber(rn);                                         \
    }                                                               \
    explicit constexpr operator int() const { return rn; }          \
    constexpr bool operator==(What o) const { return rn == o.rn; }  \
    constexpr bool operator!=(What o) const { return rn != o.rn; }  \
  private:                                                          \
    int rn;                                                         \
  }

SIMPLE_REGTYPE(Reg32);
SIMPLE_REGTYPE(Reg16);
SIMPLE_REGTYPE(Reg8);
SIMPLE_REGTYPE(RegXMM);
SIMPLE_REGTYPE(RegMMX);

#undef SIMPLE_REGTYPE

struct RegRIP
{
    RIPRelativeRef operator[](intptr_t disp) const;
};

//////////////////////////////////////////////////////////////////////

namespace reg
{
    const RegNumber noreg = RegNumber(0x40);

    constexpr Reg64 rax(0);
    constexpr Reg64 rcx(1);
    constexpr Reg64 rdx(2);
    constexpr Reg64 rbx(3);
    constexpr Reg64 rsp(4);
    constexpr Reg64 rbp(5);
    constexpr Reg64 rsi(6);
    constexpr Reg64 rdi(7);

    constexpr Reg64 r8(8);
    constexpr Reg64 r9(9);
    constexpr Reg64 r10(10);
    constexpr Reg64 r11(11);
    constexpr Reg64 r12(12);
    constexpr Reg64 r13(13);
    constexpr Reg64 r14(14);
    constexpr Reg64 r15(15);

    constexpr RegRIP rip = RegRIP();

    constexpr Reg32 eax(0);
    constexpr Reg32 ecx(1);
    constexpr Reg32 edx(2);
    constexpr Reg32 ebx(3);
    constexpr Reg32 esp(4);
    constexpr Reg32 ebp(5);
    constexpr Reg32 esi(6);
    constexpr Reg32 edi(7);
    constexpr Reg32 r8d(8);
    constexpr Reg32 r9d(9);
    constexpr Reg32 r10d(10);
    constexpr Reg32 r11d(11);
    constexpr Reg32 r12d(12);
    constexpr Reg32 r13d(13);
    constexpr Reg32 r14d(14);
    constexpr Reg32 r15d(15);

    constexpr Reg16 ax(0);
    constexpr Reg16 cx(1);
    constexpr Reg16 dx(2);
    constexpr Reg16 bx(3);
    constexpr Reg16 sp(4);
    constexpr Reg16 bp(5);
    constexpr Reg16 si(6);
    constexpr Reg16 di(7);
    constexpr Reg16 r8w(8);
    constexpr Reg16 r9w(9);
    constexpr Reg16 r10w(10);
    constexpr Reg16 r11w(11);
    constexpr Reg16 r12w(12);
    constexpr Reg16 r13w(13);
    constexpr Reg16 r14w(14);
    constexpr Reg16 r15w(15);

    constexpr Reg8 al(0);
    constexpr Reg8 cl(1);
    constexpr Reg8 dl(2);
    constexpr Reg8 bl(3);
    constexpr Reg8 spl(4); // Instructions that use spl, bpl, sil, or dil must
    constexpr Reg8 bpl(5); // have a REX prefix even if the rex.W, rex.R, rex.X,
    constexpr Reg8 sil(6); // and rex.B bits are all zero. See the comment below
    constexpr Reg8 dil(7); // for details.
    constexpr Reg8 r8b(8);
    constexpr Reg8 r9b(9);
    constexpr Reg8 r10b(10);
    constexpr Reg8 r11b(11);
    constexpr Reg8 r12b(12);
    constexpr Reg8 r13b(13);
    constexpr Reg8 r14b(14);
    constexpr Reg8 r15b(15);

    /**
     * The reg numbers that we use internally for high-byte regs (ah, ch, dh, bh)
     * are "tagged" so that we can distinguish them from the reg numbers for spl,
     * bpl, sil, and dil. The tag (bit 7) is only used for internal purposes and
     * it gets masked out when we encode reg numbers during instruction emission.
     * In 64-bit mode, the CPU's decoder relies on the absence or presence of the
     * REX prefix to distinguish between ah/ch/dh/bh vs. spl/bpl/sil/dil. Thus,
     * x64 does not allow instructions to mix high-byte regs together with spl,
     * bpl, sil, dil, r8b-r15b, r8w-r15w, r8d-r15d, r8-r15, or any other feature
     * that requires a REX prefix.
     */
    constexpr Reg8 ah(0x80 | 4);
    constexpr Reg8 ch(0x80 | 5);
    constexpr Reg8 dh(0x80 | 6);
    constexpr Reg8 bh(0x80 | 7);

    constexpr RegXMM xmm0(0);
    constexpr RegXMM xmm1(1);
    constexpr RegXMM xmm2(2);
    constexpr RegXMM xmm3(3);
    constexpr RegXMM xmm4(4);
    constexpr RegXMM xmm5(5);
    constexpr RegXMM xmm6(6);
    constexpr RegXMM xmm7(7);
    constexpr RegXMM xmm8(8);
    constexpr RegXMM xmm9(9);
    constexpr RegXMM xmm10(10);
    constexpr RegXMM xmm11(11);
    constexpr RegXMM xmm12(12);
    constexpr RegXMM xmm13(13);
    constexpr RegXMM xmm14(14);
    constexpr RegXMM xmm15(15);

    constexpr RegMMX mmx0(0);
    constexpr RegMMX mmx1(1);
    constexpr RegMMX mmx2(2);
    constexpr RegMMX mmx3(3);
    constexpr RegMMX mmx4(4);
    constexpr RegMMX mmx5(5);
    constexpr RegMMX mmx6(6);
    constexpr RegMMX mmx7(7);

    /**
     * Returns true if encoding the specified register requires setting a bit in
     * the REX prefix (either rex.R, rex.X, or rex.B, depending on which operand
     * is being encoded).
     */
    inline __forceinline
        bool usesRexBit(int rn)
    {
        static_assert(!(int(reg::noreg) & 8),
            "usesRexBit(reg::noreg) must return false");
        return (bool)(rn & 8);
    }

    /**
     * Returns true if the specified reg number corresponds to a high-byte
     * register (ah, ch, dh, bh).
     */
    inline __forceinline
        bool isHighByteReg(int rn)
    {
        return (rn & 0x80);
    }

    /**
     * Returns true if the specified byte reg number corresponds to spl, bpl,
     * sil, or dil. Instructions that use spl/bpl/sil/dil must have a REX prefix
     * even if the rex.W, rex.R, rex.X, and rex.B bits are all zero.
     */
    inline __forceinline
        bool byteRegRequiresRexPrefix(int rn)
    {
        bool requiresRexPrefix = (rn >= 4 && rn <= 7);
        // If rn is a to high-byte register, this function should return false.
        assert(!isHighByteReg(rn) || !requiresRexPrefix);
        return requiresRexPrefix;
    }

    inline void highByteRegMisuse()
    {
        // In 64-bit mode, the use of high-byte registers is restricted to
        // instructions that don't need a REX prefix.
        assert(!"High byte registers can't be used with new x64 registers, or "
            "with any mode or feature that requires a REX prefix");
    }
} // namespace

//////////////////////////////////////////////////////////////////////

typedef uint8_t* Address;
typedef uint8_t* CodeAddress;

constexpr inline bool IsPowerOf2(size_t x)
{
    return x && ((x & (x - 1)) == 0);
}

/**
 * CodeBlock is a simple bump-allocating wrapper around a block of executable
 * memory. CodeBlock does not allocate this block itself, but rather attaches
 * to an an existing block of memory.
 */
struct CodeBlock
{
    CodeBlock() : m_base(nullptr), m_frontier(nullptr), m_size(0) {}

    CodeBlock(const CodeBlock& other) = delete;
    CodeBlock& operator=(const CodeBlock& other) = delete;

    CodeBlock(CodeBlock&& other)
        : m_base(other.m_base), m_frontier(other.m_frontier), m_size(other.m_size)
    {
        other.m_base = other.m_frontier = nullptr;
        other.m_size = 0;
    }

    CodeBlock& operator=(CodeBlock&& other)
    {
        m_base = other.m_base;
        m_frontier = other.m_frontier;
        m_size = other.m_size;
        other.m_base = other.m_frontier = nullptr;
        other.m_size = 0;
        return *this;
    }

    /**
     * Attach to an existing block of memory. It is the caller's responsibility
     * to mark this block of memory with "executable" permission before trying
     * to invoke code emitted into the block of memory.
     */
    inline __forceinline
        void init(Address start, size_t sz)
    {
        m_base = m_frontier = start;
        m_size = sz;
    }

    inline __forceinline
        bool canEmit(size_t nBytes)
    {
        assert(m_frontier >= m_base);
        assert(m_frontier <= m_base + m_size);
        return m_frontier + nBytes <= m_base + m_size;
    }

#define BLOCK_EMISSION_ASSERT(canEmitCheck) \
  assert((canEmitCheck) && \
         "Data block does not have enough room to emit instruction");

    void byte(const uint8_t byte)
    {
        BLOCK_EMISSION_ASSERT(canEmit(sz::byte));
        *m_frontier = byte;
        m_frontier += sz::byte;
    }
    void word(const uint16_t word)
    {
        BLOCK_EMISSION_ASSERT(canEmit(sz::word));
        *(uint16_t*)m_frontier = word;
        m_frontier += sz::word;
    }
    void dword(const uint32_t dword)
    {
        BLOCK_EMISSION_ASSERT(canEmit(sz::dword));
        *(uint32_t*)m_frontier = dword;
        m_frontier += sz::dword;
    }
    void qword(const uint64_t qword)
    {
        BLOCK_EMISSION_ASSERT(canEmit(sz::qword));
        *(uint64_t*)m_frontier = qword;
        m_frontier += sz::qword;
    }
    inline __forceinline
        void bytes(size_t n, const uint8_t *bs)
    {
        BLOCK_EMISSION_ASSERT(canEmit(n));
        if (n <= 8) {
            // If it is a modest number of bytes, try executing in one machine
            // store. This allows control-flow edges, including nop, to be
            // appear idempotent on other CPUs.
            union {
                uint64_t qword;
                uint8_t bytes[8];
            } u;
            u.qword = *(uint64_t*)m_frontier;
            for (size_t i = 0; i < n; ++i) {
                u.bytes[i] = bs[i];
            }

            // If this address doesn't span cache lines, on x64 this is an
            // atomic store.  We're not using atomic_release_store() because
            // this code path occurs even when it may span cache lines, and
            // that function asserts about this.
            *reinterpret_cast<uint64_t*>(m_frontier) = u.qword;
        }
        else {
            memcpy(m_frontier, bs, n);
        }
        m_frontier += n;
    }

    void skip(size_t n)
    {
        BLOCK_EMISSION_ASSERT(canEmit(n));
        m_frontier += n;
    }

#undef BLOCK_EMISSION_ASSERT

    inline __forceinline
        Address base() const { return m_base; }

    inline __forceinline
        Address frontier() const { return m_frontier; }

    inline __forceinline
        void setFrontier(Address addr)
    {
        m_frontier = addr;
    }

    inline __forceinline
        size_t capacity() const
    {
        return m_size;
    }

    inline __forceinline
        size_t used() const
    {
        return m_frontier - m_base;
    }

    inline __forceinline
        size_t available() const
    {
        return m_size - (m_frontier - m_base);
    }

    inline __forceinline
        bool contains(CodeAddress addr) const
    {
        return addr >= m_base && addr < (m_base + m_size);
    }

    inline __forceinline
        bool empty() const
    {
        return m_base == m_frontier;
    }

    inline __forceinline
        void clear()
    {
        m_frontier = m_base;
    }

    void zero()
    {
        memset(m_base, 0, m_frontier - m_base);
        clear();
    }

private:
    Address m_base;
    Address m_frontier;
    size_t m_size;
};

const uint8_t kOpsizePrefix = 0x66;

// Go from a RegNumber to the same physical register of a given
// size.

inline Reg8 rbyte(Reg32 r) { return Reg8(int(r)); }
inline Reg8 rbyte(RegNumber r) { return Reg8(int(r)); }
inline Reg16 r16(Reg8 r) { return Reg16(int(r)); }
inline Reg16 r16(RegNumber r) { return Reg16(int(r)); }
inline Reg32 r32(Reg8 r) { return Reg32(int(r)); }
inline Reg32 r32(Reg16 r) { return Reg32(int(r)); }
inline Reg32 r32(Reg32 r) { return r; }
inline Reg32 r32(RegNumber r) { return Reg32(int(r)); }
inline Reg64 r64(RegNumber r) { return Reg64(int(r)); }

//////////////////////////////////////////////////////////////////////

inline bool signedValueFits(int64_t val, int s)
{
    assert(s != sz::nosize);
    if (s == sz::qword) {
        return true;
    }
    int64_t bits = s * 8;
    return val < (1ll << (bits - 1)) && val >= -(1ll << (bits - 1));
}

inline bool unsignedValueFits(uint64_t val, int s)
{
    assert(s != sz::nosize);
    if (s == sz::qword) {
        return true;
    }
    uint64_t bits = s * 8;
    return (val & ((1ull << bits) - 1)) == val;
}

/**
 * The following structures define intermediate types for various
 * addressing modes.  They overload some operators to allow using
 * registers to look somewhat like pointers.
 *
 * E.g. rax[rbx*2 + 3] or *(rax + rbx*2 + 3).
 *
 * These operators are not defined commutatively; the thought is it
 * mandates the order you normally write them in a .S, but it could be
 * changed if this proves undesirable.
 */

 // reg*x
struct ScaledIndex
{
    explicit ScaledIndex(Reg64 index, intptr_t scale)
        : index(index)
        , scale(scale)
    {
        assert((scale == sz::byte || scale == sz::word ||
            scale == sz::dword || scale == sz::qword) &&
            "Invalid index register scaling (must be 1, 2, 4, or 8).");
        assert(int(index) != int(reg::noreg) && "invalid register");
        assert(int(index) != int(reg::rsp));
    }

    ScaledIndexDisp operator+(intptr_t x) const;
    ScaledIndexDisp operator-(intptr_t x) const;

    IndexedBaselessMemoryRef operator*() const;
    IndexedBaselessMemoryRef operator[](intptr_t) const;

    Reg64 index;
    intptr_t scale;
};

// reg*x + disp
struct ScaledIndexDisp
{
    explicit ScaledIndexDisp(ScaledIndex si, intptr_t disp)
        : si(si)
        , disp(disp)
    { }

    ScaledIndexDisp operator+(intptr_t x) const
    {
        return ScaledIndexDisp(si, disp + x);
    }

    ScaledIndexDisp operator-(intptr_t x) const
    {
        return ScaledIndexDisp(si, disp - x);
    }

    IndexedBaselessMemoryRef operator*() const;
    IndexedBaselessMemoryRef operator[](intptr_t) const;

    ScaledIndex si;
    intptr_t disp;
};

// reg+x
struct DispReg
{
    explicit DispReg(Reg64 base, intptr_t disp = 0)
        : base(base)
        , disp(disp)
    {
        assert(int(base) != int(reg::noreg) && "invalid register");
    }

    SimpleMemoryRef operator*() const;
    SimpleMemoryRef operator[](intptr_t) const;

    DispReg operator+(intptr_t x) const
    {
        return DispReg(base, disp + x);
    }

    DispReg operator-(intptr_t x) const
    {
        return DispReg(base, disp - x);
    }

    Reg64 base;
    intptr_t disp;
};

// reg + reg*x + y
struct IndexedDispReg
{
    explicit IndexedDispReg(Reg64 base, ScaledIndex sr)
        : base(base)
        , index(sr.index)
        , scale(sr.scale)
        , disp(0)
    {
        assert(int(base) != int(reg::noreg) && "invalid register");
        assert(int(index) != int(reg::noreg) && "invalid register");
        assert((scale == sz::byte || scale == sz::word ||
            scale == sz::dword || scale == sz::qword) &&
            "Invalid index register scaling (must be 1, 2, 4, or 8).");
    }

    explicit IndexedDispReg(Reg64 base, ScaledIndexDisp sid)
        : base(base)
        , index(sid.si.index)
        , scale(sid.si.scale)
        , disp(sid.disp)
    {
        assert(int(base) != int(reg::noreg) && "invalid register");
        assert(int(index) != int(reg::noreg) && "invalid register");
        assert((scale == sz::byte || scale == sz::word ||
            scale == sz::dword || scale == sz::qword) &&
            "Invalid index register scaling (must be 1, 2, 4, or 8).");
    }

    IndexedMemoryRef operator*() const;
    IndexedMemoryRef operator[](intptr_t disp) const;

    IndexedDispReg operator+(intptr_t disp) const
    {
        auto ret = *this;
        ret.disp += disp;
        return ret;
    }

    IndexedDispReg operator-(intptr_t disp) const
    {
        auto ret = *this;
        ret.disp -= disp;
        return ret;
    }

    Reg64 base;
    Reg64 index;
    int scale;
    intptr_t disp;
};

// rip+x
struct DispRIP
{
    explicit DispRIP(intptr_t disp) : disp(disp) {}

    RIPRelativeRef operator*() const;
    RIPRelativeRef operator[](intptr_t x) const;

    DispRIP operator+(intptr_t x) const
    {
        return DispRIP(disp + x);
    }

    DispRIP operator-(intptr_t x) const
    {
        return DispRIP(disp - x);
    }

    intptr_t disp;
};

// x (displacement with no base)
struct DispBaseless
{
    explicit DispBaseless(intptr_t disp) : disp(disp) {}

    BaselessMemoryRef operator*() const;
    BaselessMemoryRef operator[](intptr_t x) const;

    DispBaseless operator+(intptr_t x) const
    {
        return DispBaseless(disp + x);
    }

    DispBaseless operator-(intptr_t x) const
    {
        return DispBaseless(disp - x);
    }

    intptr_t disp;
};

// *(reg + x)
struct SimpleMemoryRef
{
    explicit SimpleMemoryRef(DispReg r) : r(r) {}

    DispReg r;
};

// *(reg + reg*x + y)
struct IndexedMemoryRef
{
    explicit IndexedMemoryRef(IndexedDispReg r) : r(r) {}

    IndexedDispReg r;
};

// *(reg*x + y)
struct IndexedBaselessMemoryRef
{
    explicit IndexedBaselessMemoryRef(ScaledIndex si) : sid(si, 0) {}
    explicit IndexedBaselessMemoryRef(ScaledIndexDisp sid) : sid(sid) {}

    ScaledIndexDisp sid;
};

// *(rip + x)
struct RIPRelativeRef
{
    explicit RIPRelativeRef(DispRIP r) : r(r) {}

    DispRIP r;
};

// *x (dereferenced displacement without a base)
struct BaselessMemoryRef
{
    explicit BaselessMemoryRef(DispBaseless db) : db(db) {}

    DispBaseless db;
};

struct MemoryRef
{
    const Reg64 noreg64 = Reg64(int(reg::noreg));

    /* implicit */ MemoryRef(const SimpleMemoryRef& smr)
        : disp(smr.r.disp), base(smr.r.base), index(noreg64), scale(sz::byte),
        ripRelative(false)
    { }

    /* implicit */ MemoryRef(const IndexedMemoryRef& imr)
        : disp(imr.r.disp), base(imr.r.base), index(imr.r.index),
        scale(imr.r.scale), ripRelative(false)
    {
        assert(index != reg::rsp);

        swapRegsIfValidShorterEncoding();
    }

    /* implicit */ MemoryRef(const IndexedBaselessMemoryRef& ibmr)
        : disp(ibmr.sid.disp), base(noreg64), index(ibmr.sid.si.index),
        scale(ibmr.sid.si.scale), ripRelative(false)
    {
        assert(index != reg::rsp);

        swapRegsIfValidShorterEncoding();
    }

    /* implicit */ MemoryRef(const RIPRelativeRef& rrr)
        : disp(rrr.r.disp), base(noreg64), index(noreg64), scale(sz::byte),
        ripRelative(true)
    { }

    /* implicit */ MemoryRef(const BaselessMemoryRef& bmr)
        : disp(bmr.db.disp), base(noreg64), index(noreg64), scale(sz::byte),
        ripRelative(false)
    { }

    void swapRegsIfValidShorterEncoding()
    {
        // When scale is sz::byte, we swap `base` and `index` when it produces
        // a shorter encoding.
        if (scale == sz::byte && base != reg::rsp && index != noreg64 &&
            (base == noreg64 && signedValueFits(disp, sz::byte) ||
                base == noreg64 && index != reg::r12 ||
                disp == 0 && index != reg::rbp && index != reg::r13 &&
                (base == reg::rbp || base == reg::r13))) {
            std::swap(base, index);
        }
    }

    intptr_t disp;
    Reg64 base;
    Reg64 index;
    int scale;
    bool ripRelative;
};

inline IndexedMemoryRef IndexedDispReg::operator*() const
{
    return IndexedMemoryRef(*this);
}

inline IndexedMemoryRef IndexedDispReg::operator[](intptr_t x) const
{
    return *(*this + x);
}

inline ScaledIndexDisp ScaledIndex::operator+(intptr_t x) const
{
    return ScaledIndexDisp(*this, x);
}

inline ScaledIndexDisp ScaledIndex::operator-(intptr_t x) const
{
    return ScaledIndexDisp(*this, -x);
}

inline IndexedBaselessMemoryRef ScaledIndex::operator*() const
{
    return IndexedBaselessMemoryRef(*this);
}

inline IndexedBaselessMemoryRef ScaledIndex::operator[](intptr_t x) const
{
    return *(ScaledIndexDisp(*this, x));
}

inline IndexedBaselessMemoryRef ScaledIndexDisp::operator*() const
{
    return IndexedBaselessMemoryRef(*this);
}

inline IndexedBaselessMemoryRef ScaledIndexDisp::operator[](intptr_t x) const
{
    return *(*this + x);
}

inline SimpleMemoryRef DispReg::operator*() const
{
    return SimpleMemoryRef(*this);
}

inline SimpleMemoryRef DispReg::operator[](intptr_t x) const
{
    return *(*this + x);
}

inline RIPRelativeRef DispRIP::operator*() const
{
    return RIPRelativeRef(*this);
}

inline RIPRelativeRef DispRIP::operator[](intptr_t x) const
{
    return *(*this + x);
}

inline BaselessMemoryRef DispBaseless::operator*() const
{
    return BaselessMemoryRef(*this);
}

inline BaselessMemoryRef DispBaseless::operator[](intptr_t x) const
{
    return *(*this + x);
}

inline DispReg operator+(Reg64 r, intptr_t d) { return DispReg(r, d); }
inline DispReg operator-(Reg64 r, intptr_t d) { return DispReg(r, -d); }
inline DispRIP operator+(RegRIP r, intptr_t d) { return DispRIP(d); }
inline DispRIP operator-(RegRIP r, intptr_t d) { return DispRIP(-d); }

inline ScaledIndex operator*(Reg64 r, int scale)
{
    return ScaledIndex(r, scale);
}
inline IndexedDispReg operator+(Reg64 base, ScaledIndex sr)
{
    return IndexedDispReg(base, sr);
}
inline ScaledIndexDisp operator+(ScaledIndex si, intptr_t disp)
{
    return ScaledIndexDisp(si, disp);
}
inline IndexedDispReg operator+(Reg64 b, Reg64 i)
{
    return b + ScaledIndex(i, sz::byte);
}

inline SimpleMemoryRef operator*(Reg64 r) { return SimpleMemoryRef(DispReg(r)); }
inline DispRIP operator*(RegRIP r) { return DispRIP(0); }

inline SimpleMemoryRef Reg64::operator[](intptr_t disp) const
{
    return *(*this + disp);
}

inline IndexedMemoryRef Reg64::operator[](Reg64 idx) const
{
    return *(*this + idx * 1);
}

inline IndexedMemoryRef Reg64::operator[](ScaledIndex si) const
{
    return *(*this + si);
}

inline IndexedMemoryRef Reg64::operator[](DispReg dr) const
{
    return *(*this + ScaledIndex(dr.base, sz::byte) + dr.disp);
}

inline IndexedMemoryRef Reg64::operator[](ScaledIndexDisp sid) const
{
    return *(*this + sid.si + sid.disp);
}

inline RIPRelativeRef RegRIP::operator[](intptr_t disp) const
{
    return *(*this + disp);
}

//////////////////////////////////////////////////////////////////////

namespace reg
{

#define X(x) if (r == x) return "%"#x
    inline const char* regname(Reg64 r)
    {
        X(rax); X(rbx); X(rcx); X(rdx); X(rsp); X(rbp); X(rsi); X(rdi);
        X(r8); X(r9); X(r10); X(r11); X(r12); X(r13); X(r14); X(r15);
        return nullptr;
    }
    inline const char* regname(Reg32 r)
    {
        X(eax); X(ecx); X(edx); X(ebx); X(esp); X(ebp); X(esi); X(edi);
        X(r8d); X(r9d); X(r10d); X(r11d); X(r12d); X(r13d); X(r14d); X(r15d);
        return nullptr;
    }
    inline const char* regname(Reg16 r)
    {
        X(ax); X(cx); X(dx); X(bx); X(sp); X(bp); X(si); X(di);
        X(r8w); X(r9w); X(r10w); X(r11w); X(r12w); X(r13w); X(r14w); X(r15w);
        return nullptr;
    }
    inline const char* regname(Reg8 r)
    {
        X(al); X(cl); X(dl); X(bl); X(spl); X(bpl); X(sil); X(dil);
        X(r8b); X(r9b); X(r10b); X(r11b); X(r12b); X(r13b); X(r14b); X(r15b);
        X(ah); X(ch); X(dh); X(bh);
        return nullptr;
    }
    inline const char* regname(RegXMM r)
    {
        X(xmm0); X(xmm1); X(xmm2); X(xmm3); X(xmm4); X(xmm5); X(xmm6);
        X(xmm7); X(xmm8); X(xmm9); X(xmm10); X(xmm11); X(xmm12); X(xmm13);
        X(xmm14); X(xmm15);
        return nullptr;
    }
    inline const char* regname(RegMMX r)
    {
        X(mmx0); X(mmx1); X(mmx2); X(mmx3); X(mmx4); X(mmx5); X(mmx6); X(mmx7);
        return nullptr;
    }
#undef X

}

//////////////////////////////////////////////////////////////////////

enum instrFlags
{
    // Instruction requires a 0x66 prefix. Cannot be used with IF_PREFIX_F2 or
    // IF_PREFIX_F3.
    IF_PREFIX_66 = 0x000001,

    // Instruction requires a 0xF2 prefix. Cannot be used with IF_PREFIX_66 or
    // IF_PREFIX_F3.
    IF_PREFIX_F2 = 0x000002,

    // Instruction requires a 0xF3 prefix. Cannot be used with IF_PREFIX_66 and
    // IF_PREFIX_F2.
    IF_PREFIX_F3 = 0x000004,

    // For qword address modes, this instruction does not require the rex.W bit
    // to be set to 1.
    IF_NO_REXW = 0x000008,

    // Instruction's opcode is 2 bytes long and the first opcode byte is 0x0F.
    // Cannot be used with IF_ESCAPE_0F3A.
    IF_ESCAPE_0F = 0x000010,

    // Instruction's opcode is 3 bytes long and the first two opcode bytes
    // are 0x0F 0x3A. Cannot be used with IF_ESCAPE_0F.
    IF_ESCAPE_0F3A = 0x000020,

    // When IF_REVERSE is set, it means that the instruction's "forward" encoding
    // uses r as the destination and the instruction's "backward" encoding uses
    // rm as the destination.
    IF_REVERSE = 0x000040,

    // For address modes that take an immediate operand, this instruction only
    // supports 8-bit immediates. Cannot be used with IF_WORD_IMM_ONLY or
    // IF_HAS_SHORTER_IMM.
    IF_BYTE_IMM_ONLY = 0x000080,

    // For address modes that take an immediate operand, this instruction only
    // supports 16-bit immediates. Cannot be used with IF_BYTE_IMM_ONLY or
    // IF_HAS_SHORTER_IMM.
    IF_WORD_IMM_ONLY = 0x000100,

    // For word/dword/qword address modes that take an immediate operand, this
    // instruction supports a special "short" encoding that takes a byte-size
    // immediate (in addition to the instruction's usual encoding that takes
    // either a word-, dword-, or qword-sized immediate). Cannot be used with
    // IF_BYTE_IMM_ONLY or IF_WORD_IMM_ONLY.
    IF_HAS_SHORTER_IMM = 0x000200,

    // Instruction supports R-compact encoding. Cannot be used with
    // IF_RAX_COMPACT, IF_1_COMPACT, IF_MOV, or IF_XCHG.
    IF_R_COMPACT = 0x000400,

    // Instruction supports rax-compact encoding. Cannot be used with
    // IF_R_COMPACT, IF_1_COMPACT, IF_MOV, or IF_XCHG.
    IF_RAX_COMPACT = 0x000800,

    // Instruction supports 1-compact encoding. Cannot be used with
    // IF_R_COMPACT, IF_RAX_COMPACT, IF_MOV, or IF_XCHG.
    IF_1_COMPACT = 0x001000,

    // Instruction is mov. The mov instruction uses a special encoding when
    // opSz == immSize, and because it is the only instruction that supports
    // 64-bit immediates. Cannot be used with IF_R_COMPACT, IF_RAX_COMPACT,
    // IF_1_COMPACT, or IF_XCHG.
    IF_MOV = 0x002000,

    // Instruction is xchg. The xchg instruction has a special encoding when
    // r is rax. Cannot be used with IF_R_COMPACT, IF_RAX_COMPACT, IF_1_COMPACT,
    // or IF_MOV.
    IF_XCHG = 0x004000,

    // Instruction is movzxb or movsxb. For these instructions, the "rm"
    // operand is always byte-size regardless of whether opSz == sz::byte.
    IF_MOVXB = 0x008000,

    // Instruction is setcc. Unlike other instructions, setcc only supports
    // opSz == sz::byte and its operand is always byte size.
    IF_SETCC = 0x010000,

    // These flags are for optimization purposes, and for supporting immediates
    // between 2^31 and 2^32-1 for certain instructions.
    IF_AND = 0x020000, // Instruction is and.
    IF_XOR = 0x040000, // Instruction is xor.
    IF_TEST = 0x080000, // Instruction is test.
    IF_MOVZX = 0x100000, // Instruction is movzxb or movzxw.
};

/*
  Address mode to index map:
    Index 0 - "Forward" encoding: R / RR / R_M (normal) / MR (reverse)
    Index 1 - "Backward" encoding: MR (normal) / RM (reverse)
    Index 2 - "Immediate" encoding: I / IR / IM / IRR / IRM / IMR
    Index 3 - "Compact" encoding: R (compact) / IR (compact) / noargs
    Index 4 - "/digit" value used for by some address modes

  "Reverse" instructions are instructions where IF_REVERSE is set. "Normal"
  instructions are instructions where IF_REVERSE is not set.

  The term "prefix" is used to refer to any byte or sequence of bytes in an
  instruction that come before the "opcode" bytes.

  The REX prefix is a special prefix in x64 code that is used to extend the
  x86 instruction set so that it can refer to r8-r15, r8d-r15d, r8w-r15w,
  spl, bpl, sil, dil, r8b-r15b, and xmm8-xmm15. The REX prefix is also used
  by some instructions to distinguish between dword and qword address modes.
  If an instruction has a REX prefix, all other prefixes (if any) must come
  before the REX prefix.

  0xF1 is used to indicate invalid opcodes.
*/

struct X64Instr
{
    unsigned char table[5];
    unsigned flags : 24;
};

// inc supports R and M modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_inc = { { 0xFF,0xF1,0xF1,0xF1,0x00 }, 0 };

// dec supports R and M modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_dec = { { 0xFF,0xF1,0xF1,0xF1,0x01 }, 0 };

// not supports R and M modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_not = { { 0xF7,0xF1,0xF1,0xF1,0x02 }, 0 };

// neg supports R and M modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_neg = { { 0xF7,0xF1,0xF1,0xF1,0x03 }, 0 };

// lea supports MR mode (where R is gpr).
// Supports qword, dword, word.
//                                    0    1    2    3    4
const X64Instr instr_lea = { { 0xF1,0x8D,0xF1,0xF1,0x00 }, 0 };

// xchg supports RR, RM, and MR modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_xchg = { { 0x87,0x87,0xF1,0x90,0x00 },
                                 (IF_XCHG) };

// mul supports R and M modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_mul = { { 0xF7,0xF1,0xF1,0xF1,0x04 }, 0 };

// div supports R and M modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_div = { { 0xF7,0xF1,0xF1,0xF1,0x06 }, 0 };

// idiv supports R and M modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_idiv = { { 0xF7,0xF1,0xF1,0xF1,0x07 }, 0 };

// add supports RR, RM, MR, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_add = { { 0x01,0x03,0x81,0x05,0x00 },
                                 (IF_HAS_SHORTER_IMM | IF_RAX_COMPACT) };

// sub supports RR, RM, MR, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_sub = { { 0x29,0x2B,0x81,0x2D,0x05 },
                                 (IF_HAS_SHORTER_IMM | IF_RAX_COMPACT) };

// and supports RR, RM, MR, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_and = { { 0x21,0x23,0x81,0x25,0x04 },
                                 (IF_HAS_SHORTER_IMM | IF_RAX_COMPACT | IF_AND) };

// or supports RR, RM, MR, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_or = { { 0x09,0x0B,0x81,0x0D,0x01 },
                                 (IF_HAS_SHORTER_IMM | IF_RAX_COMPACT) };

// xor supports RR, RM, MR, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_xor = { { 0x31,0x33,0x81,0x35,0x06 },
                                 (IF_HAS_SHORTER_IMM | IF_RAX_COMPACT | IF_XOR) };

// cmp supports RR, RM, MR, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_cmp = { { 0x39,0x3B,0x81,0x3D,0x07 },
                                 (IF_HAS_SHORTER_IMM | IF_RAX_COMPACT) };

// adc supports RR, RM, MR, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_adc = { { 0x11,0x13,0x81,0x15,0x02 },
                                 (IF_HAS_SHORTER_IMM | IF_RAX_COMPACT) };

// sbb supports RR, RM, MR, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_sbb = { { 0x19,0x1B,0x81,0x1D,0x03 },
                                 (IF_HAS_SHORTER_IMM | IF_RAX_COMPACT) };

// mov supports RR, RM, MR, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_mov = { { 0x89,0x8B,0xC7,0xB8,0x00 },
                                 (IF_MOV) };

// test supports RR, RM, MR, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_test = { { 0x85,0x85,0xF7,0xA9,0x00 },
                                 (IF_RAX_COMPACT | IF_TEST) };

// imul supports R and M modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_imul = { { 0xF7,0xF1,0xF1,0xF1,0x05 }, 0 };

// imul2 supports RR and MR modes (where R is gpr).
// Supports qword, dword, word.
//                                    0    1    2    3    4
const X64Instr instr_imul2 = { { 0xAF,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F) };

// imul3 supports IRR and IMR modes (where R is gpr).
// Supports qword, dword, word.
//                                    0    1    2    3    4
const X64Instr instr_imul3 = { { 0xF1,0xF1,0x69,0xF1,0x00 },
                                 (IF_REVERSE | IF_HAS_SHORTER_IMM) };

// jmp supports I, R, and M modes (where R is gpr).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_jmp = { { 0xFF,0xF1,0xE9,0xF1,0x04 },
                                 (IF_NO_REXW) };

// jmp8 supports I mode.
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_jmp8 = { { 0xF1,0xF1,0xEB,0xF1,0x00 },
                                 (IF_NO_REXW | IF_BYTE_IMM_ONLY) };

// call supports I, R, and M modes (where R is gpr).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_call = { { 0xFF,0xF1,0xE8,0xF1,0x02 },
                                 (IF_NO_REXW) };

// push supports I, R, and M modes (where R is gpr).
// Supports qword and word only.
//                                    0    1    2    3    4
const X64Instr instr_push = { { 0xFF,0xF1,0x68,0x50,0x06 },
                                 (IF_HAS_SHORTER_IMM | IF_NO_REXW | IF_R_COMPACT) };

// pop supports R and M modes (where R is gpr).
// Supports qword and word only.
//                                    0    1    2    3    4
const X64Instr instr_pop = { { 0x8F,0xF1,0xF1,0x58,0x00 },
                                 (IF_NO_REXW | IF_R_COMPACT) };

// ret supports I and "noargs" modes.
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_ret = { { 0xF1,0xF1,0xC2,0xC3,0x00 },
                                 (IF_WORD_IMM_ONLY | IF_NO_REXW) };

// jcc supports CI mode.
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_jcc = { { 0xF1,0xF1,0x80,0xF1,0x00 },
                                 (IF_ESCAPE_0F | IF_NO_REXW) };

// jcc8 supports CI mode.
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_jcc8 = { { 0xF1,0xF1,0x70,0xF1,0x00 },
                                 (IF_NO_REXW | IF_BYTE_IMM_ONLY) };

// rol supports R, M, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_rol = { { 0xD3,0xF1,0xC1,0xD1,0x00 },
                                 (IF_1_COMPACT | IF_BYTE_IMM_ONLY) };

// ror supports R, M, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_ror = { { 0xD3,0xF1,0xC1,0xD1,0x01 },
                                 (IF_1_COMPACT | IF_BYTE_IMM_ONLY) };

// rcl supports R, M, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_rcl = { { 0xD3,0xF1,0xC1,0xD1,0x02 },
                                 (IF_1_COMPACT | IF_BYTE_IMM_ONLY) };

// rcr supports R, M, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_rcr = { { 0xD3,0xF1,0xC1,0xD1,0x03 },
                                 (IF_1_COMPACT | IF_BYTE_IMM_ONLY) };

// shl supports R, M, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_shl = { { 0xD3,0xF1,0xC1,0xD1,0x04 },
                                 (IF_1_COMPACT | IF_BYTE_IMM_ONLY) };

// shr supports R, M, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_shr = { { 0xD3,0xF1,0xC1,0xD1,0x05 },
                                 (IF_1_COMPACT | IF_BYTE_IMM_ONLY) };

// sar supports R, M, IR, and IM modes (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_sar = { { 0xD3,0xF1,0xC1,0xD1,0x07 },
                                 (IF_1_COMPACT | IF_BYTE_IMM_ONLY) };

// cqo supports the "noargs" address mode.
//   cqo: RAX sign extend to RDX:RAX (opSz == qword)
//   cdq: EAX sign extend to EDX:EAX (opSz == dword)
//   cwd: AX sign extend to DX:AX (opSz == word)
// Supports qword, dword, word.
//                                    0    1    2    3    4
const X64Instr instr_cqo = { { 0xF1,0xF1,0xF1,0x99,0x00 }, 0 };

// cdqe supports the "noargs" address mode.
//   cdqe: EAX sign extend to RAX (opSz == qword)
//   cwde: AX sign extend to EAX (opSz == dword)
//   cbw: AL sign extend to AX (opSz == word)
// Supports qword, dword, word.
//                                    0    1    2    3    4
const X64Instr instr_cdqe = { { 0xF1,0xF1,0xF1,0x98,0x00 }, 0 };

// nop supports the "noargs" address mode.
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_nop = { { 0xF1,0xF1,0xF1,0x90,0x00 },
                                 (IF_NO_REXW) };

// int3 supports the "noargs" address mode.
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_int3 = { { 0xF1,0xF1,0xF1,0xCC,0x00 },
                                 (IF_NO_REXW) };

// ud2 supports the "noargs" address mode.
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_ud2 = { { 0xF1,0xF1,0xF1,0x0B,0x00 },
                                 (IF_NO_REXW | IF_ESCAPE_0F) };

// pushf supports the "noargs" address mode.
// Supports qword and word only.
const X64Instr instr_pushf = { { 0xF1,0xF1,0xF1,0x9C,0x00 },
                                 (IF_NO_REXW) };

// popf supports the "noargs" address mode.
// Supports qword and word only.
const X64Instr instr_popf = { { 0xF1,0xF1,0xF1,0x9D,0x00 },
                                 (IF_NO_REXW) };

// xadd supports RR and RM (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_xadd = { { 0xC1,0xF1,0xF1,0xF1,0x00 },
                                 (IF_ESCAPE_0F) };

// cmpxchg supports RR and RM (where R is gpr).
// Supports qword, dword, word, byte.
//                                    0    1    2    3    4
const X64Instr instr_cmpxchg = { { 0xB1,0xF1,0xF1,0xF1,0x00 },
                                 (IF_ESCAPE_0F) };

// cmpxchg16b supports M.
// Supports qword and dword only.
// Not supported by early AMD x64 processors.
const X64Instr instr_cmpxchg16b = { { 0xC7,0xF1,0xF1,0xF1,0x01 },
                                    (IF_ESCAPE_0F) };

// movzxb supports RR and MR (where R is gpr). Source is always byte size.
//   movzxbq: R/M8 zero extend to R64 (opSz == qword)
//   movzxbd: R/M8 zero extend to R32 (opSz == dword)
//   movzxbw: R/M8 zero extend to R16 (opSz == word)
// Supports qword, dword, word.
//                                    0    1    2    3    4
const X64Instr instr_movzxb = { { 0xB6,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_MOVXB |
                                  IF_MOVZX) };

// movzxw supports RR and MR (where R is gpr). Source is always word size.
//   movzxwq: R/M16 zero extend to R64 (opSz == qword)
//   movzxwd: R/M16 zero extend to R32 (opSz == dword)
// Supports qword and dword only.
//                                    0    1    2    3    4
const X64Instr instr_movzxw = { { 0xB7,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_MOVZX) };

// movsxb supports RR and MR (where R is gpr). Source is always byte size.
//   movsxbq: R/M8 sign extend to R64 (opSz == qword)
//   movsxbd: R/M8 sign extend to R32 (opSz == dword)
//   movsxbw: R/M8 sign extend to R16 (opSz == word)
// Supports qword, dword, word.
//                                    0    1    2    3    4
const X64Instr instr_movsxb = { { 0xBE,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_MOVXB) };

// movsxw supports RR and MR (where R is gpr). Source is always word size.
//   movsxwq: R/M16 sign extend to R64 (opSz == qword)
//   movsxwd: R/M16 sign extend to R32 (opSz == dword)
// Supports qword and dword only.
//                                    0    1    2    3    4
const X64Instr instr_movsxw = { { 0xBF,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F) };

// movsxdq supports RR and MR (where R is gpr). Source is always dword size.
//   movsxdq: R/M32 sign extend to R64 (opSz == qword)
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_movsxdq = { { 0x63,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE) };

// shld supports RR, RM, IRR, IRM (where R is gpr).
// Supports qword, dword, word.
//                                    0    1    2    3    4
const X64Instr instr_shld = { { 0xA5,0xF1,0xA4,0xF1,0x00 },
                                 (IF_ESCAPE_0F | IF_BYTE_IMM_ONLY) };

// shrd supports RR, RM, IRR, IRM (where R is gpr).
// Supports qword, dword, word.
//                                    0    1    2    3    4
const X64Instr instr_shrd = { { 0xAD,0xF1,0xAC,0xF1,0x00 },
                                 (IF_ESCAPE_0F | IF_BYTE_IMM_ONLY) };

// setcc supports CR and CM modes (where R is gpr).
// Supports byte only.
//                                    0    1    2    3    4
const X64Instr instr_setcc = { { 0x90,0xF1,0xF1,0xF1,0x00 },
                                 (IF_ESCAPE_0F | IF_NO_REXW | IF_SETCC) };

// cmovcc supports CRR and CMR modes (where R is gpr).
// Supports qword, dword, word.
//                                    0    1    2    3    4
const X64Instr instr_cmovcc = { { 0x40,0x40,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F) };

// popcnt only suports RR and MR modes (where R is gpr).
// Supports qword, dword, and word.
// This instruction is not supported on all x64 processors.
//                                    0    1    2    3    4
const X64Instr instr_popcnt = { { 0xB8,0xF1,0xF1,0xF1,0x00 },
                                 (IF_ESCAPE_0F | IF_PREFIX_F3) };

// bswap supports R mode (where R is gpr).
// Supports qword and dword only.
const X64Instr instr_bswap = { { 0xF1,0xF1,0xF1,0xC8,0x00 },
                                 (IF_ESCAPE_0F | IF_R_COMPACT) };

// cmc supports "noargs" address mode.
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_cmc = { { 0xF1,0xF1,0xF1,0xF5,0x00 },
                                 (IF_NO_REXW) };

// clc supports "noargs" address mode.
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_clc = { { 0xF1,0xF1,0xF1,0xF8,0x00 },
                                 (IF_NO_REXW) };

// stc supports "noargs" address mode.
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_stc = { { 0xF1,0xF1,0xF1,0xF9,0x00 },
                                 (IF_NO_REXW) };

// xlat supports "noargs" address mode.
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_xlat = { { 0xF1,0xF1,0xF1,0xD7,0x00 },
                                 (IF_NO_REXW) };

// mov (gpr->xmm) supports RR and MR (where dest R is xmm, source R is gpr).
// Supports qword and dword only.
//                                    0    1    2    3    4
const X64Instr instr_gpr2xmm = { { 0x6E,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_PREFIX_66) };

// mov (xmm->gpr) supports RR and RM (where dest R is gpr, source R is xmm).
// Supports qword and dword only.
//                                    0    1    2    3    4
const X64Instr instr_xmm2gpr = { { 0x7E,0xF1,0xF1,0xF1,0x00 },
                                 (IF_ESCAPE_0F | IF_PREFIX_66) };

// cvtsi2sd supports RR and MR (where dest R is xmm, source R is gpr).
// Supports qword and dword only.
//                                    0    1    2    3    4
const X64Instr instr_cvtsi2sd = { { 0x2A,0xF1,0xF1,0xF1,0x00 },
                                  (IF_REVERSE | IF_ESCAPE_0F | IF_PREFIX_F2) };

// cvttsd2si supports RR and MR (where dest R is gpr, and source R is xmm).
// Supports qword and dword only.
//                                    0    1    2    3    4
const X64Instr instr_cvttsd2si = { { 0x2C,0xF1,0xF1,0xF1,0x00 },
                                   (IF_REVERSE | IF_ESCAPE_0F | IF_PREFIX_F2) };

// movsd supports RR, MR, and RM (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_movsd = { { 0x11,0x10,0xF1,0xF1,0x00 },
                                 (IF_ESCAPE_0F | IF_NO_REXW | IF_PREFIX_F2) };

// addsd supports RR and MR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_addsd = { { 0x58,0xF1,0xF1,0xF1,0x00 },
                                  (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                   IF_PREFIX_F2) };

// subsd supports RR and MR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_subsd = { { 0x5C,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_F2) };

// mulsd supports RR and MR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_mulsd = { { 0x59,0xF1,0xF1,0xF1,0x00 },
                                  (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                   IF_PREFIX_F2) };

// divsd supports RR and MR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_divsd = { { 0x5E,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_F2) };

// sqrtsd supports RR and MR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_sqrtsd = { { 0x51,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_F2) };

// movss supports RR, MR, and RM (where R is xmm).
// Supports dword only.
//                                    0    1    2    3    4
const X64Instr instr_movss = { { 0x11,0x10,0xF1,0xF1,0x00 },
                                 (IF_ESCAPE_0F | IF_NO_REXW | IF_PREFIX_F3) };

// addss supports RR and MR (where R is xmm).
// Supports dword only.
//                                    0    1    2    3    4
const X64Instr instr_addss = { { 0x58,0xF1,0xF1,0xF1,0x00 },
                                  (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                   IF_PREFIX_F3) };

// subss supports RR and MR (where R is xmm).
// Supports dword only.
//                                    0    1    2    3    4
const X64Instr instr_subss = { { 0x5C,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_F3) };

// mulss supports RR and MR (where R is xmm).
// Supports dword only.
//                                    0    1    2    3    4
const X64Instr instr_mulss = { { 0x59,0xF1,0xF1,0xF1,0x00 },
                                  (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                   IF_PREFIX_F3) };

// divss supports RR and MR (where R is xmm).
// Supports dword only.
//                                    0    1    2    3    4
const X64Instr instr_divss = { { 0x5E,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_F3) };

// sqrtss supports RR and MR (where R is xmm).
// Supports dword only.
//                                    0    1    2    3    4
const X64Instr instr_sqrtss = { { 0x51,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_F3) };

// ucomisd supports RR and MR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_ucomisd = { { 0x2E,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_66) };

// pxor supports RR and MR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_pxor = { { 0xEF,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_66) };

// psllq supports RR, MR, and IR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_psllq = { { 0xF3,0xF1,0x73,0xF1,0x06 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_66 | IF_BYTE_IMM_ONLY) };

// psrlq supports RR, MR, and IR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_psrlq = { { 0xD3,0xF1,0x73,0xF1,0x02 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_66 | IF_BYTE_IMM_ONLY) };

// movdqa only suports MR, RM, and RR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_movdqa = { { 0x6F,0x7F,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_66) };

// movdqu only suports MR, RM, and RR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_movdqu = { { 0x6F,0x7F,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_F3) };

// movdqa only suports MR, RM, and RR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_movaps = { { 0x28,0x29,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW) };

// movdqu only suports MR, RM, and RR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_movups = { { 0x10,0x11,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW) };

// movdqa only suports MR, RM, and RR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_movapd = { { 0x28,0x29,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_66) };

// movdqu only suports MR, RM, and RR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_movupd = { { 0x10,0x11,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_66) };

// lddqu supports MR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_lddqu = { { 0xF0,0xF1,0xF1,0xF1,0x00 },
                                 (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                  IF_PREFIX_F2) };

// unpcklpd supports address modes RR and MR (where R is xmm).
// Supports qword only.
//                                    0    1    2    3    4
const X64Instr instr_unpcklpd = { { 0x14,0xF1,0xF1,0xF1,0x00 },
                                  (IF_REVERSE | IF_ESCAPE_0F | IF_NO_REXW |
                                   IF_PREFIX_66) };

// roundsd supports address modes IRR and IMR (where R is xmm).
// Supports qword only.
// Not supported on all x64 processors.
//                                    0    1    2    3    4
const X64Instr instr_roundsd = { { 0xF1,0xF1,0x0B,0xF1,0x00 },
                                 (IF_REVERSE | IF_NO_REXW | IF_PREFIX_66 |
                                  IF_ESCAPE_0F3A | IF_BYTE_IMM_ONLY) };

enum class RoundDirection : int64_t
{
    nearest = 0,
    floor = 1,
    ceil = 2,
    truncate = 3,
};

enum ConditionCode
{
    CC_None = -1,

    CC_O = 0x00,
    CC_NO = 0x01,

    CC_B = 0x02,
    CC_NAE = 0x02,
    CC_AE = 0x03,
    CC_NB = 0x03,
    CC_NC = 0x03,

    CC_E = 0x04,
    CC_Z = 0x04,
    CC_NE = 0x05,
    CC_NZ = 0x05,

    CC_BE = 0x06,
    CC_NA = 0x06,
    CC_A = 0x07,
    CC_NBE = 0x07,

    CC_S = 0x08,
    CC_NS = 0x09,

    CC_P = 0x0A,
    CC_NP = 0x0B,

    CC_L = 0x0C,
    CC_NGE = 0x0C,
    CC_GE = 0x0D,
    CC_NL = 0x0D,

    CC_LE = 0x0E,
    CC_NG = 0x0E,
    CC_G = 0x0F,
    CC_NLE = 0x0F,
};

inline ConditionCode ccNegate(ConditionCode c)
{
    return ConditionCode(int(c) ^ 1); // And you thought x86 was irregular!
}

///////////////////////////////////////////////////////////////////////////////

class X64Assembler
{
public:
    explicit X64Assembler(CodeBlock& cb) : codeBlock(cb) {}

    X64Assembler(const X64Assembler&) = delete;
    X64Assembler& operator=(const X64Assembler&) = delete;

    CodeBlock& code() const { return codeBlock; }

    CodeAddress base() const { return codeBlock.base(); }

    CodeAddress frontier() const { return codeBlock.frontier(); }

    void setFrontier(CodeAddress newFrontier)
    {
        codeBlock.setFrontier(newFrontier);
    }

    size_t capacity() const { return codeBlock.capacity(); }

    size_t used() const { return codeBlock.used(); }

    size_t available() const { return codeBlock.available(); }

    bool contains(CodeAddress addr) const { return codeBlock.contains(addr); }

    bool empty() const { return codeBlock.empty(); }

    void clear()
    {
        codeBlock.clear();
    }

    bool canEmit(size_t nBytes) const
    {
        assert(capacity() >= used());
        return nBytes < (capacity() - used());
    }

    /**
     * This function will emit the "int3" instruction repeatedly to fill the
     * desired number of bytes.
     */
    void int3(size_t numBytes)
    {
        for (size_t i = 0; i < numBytes; ++i) {
            byte(0xCC);
        }
    }

    /**
     * This function will emit one or more "nop" instructions to fill the desired
     * number of bytes, taking advantage of the multi-byte nop encodings.
     */
    void nop(size_t numBytes)
    {
        static const uint8_t nops[17][16] = {
          { },
          { 0x90 },
          { 0x66, 0x90 },
          { 0x0F, 0x1F, 0x00 },
          { 0x0F, 0x1F, 0x40, 0x00 },
          { 0x0F, 0x1F, 0x44, 0x00, 0x00 },
          { 0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00 },
          { 0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00 },
          { 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 },
          { 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 },
          { 0x66, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 },
          { 0x66, 0x66, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 },
          { 0x0F, 0x1F, 0x40, 0x00,
            0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 },
          { 0x0F, 0x1F, 0x44, 0x00, 0x00,
            0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 },
          { 0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00,
            0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00 },
          { 0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00,
            0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 },
          { 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 },
        };
        if (numBytes <= 10) {
            bytes(numBytes, nops[numBytes]);
            return;
        }
        CodeAddress next16ByteBoundary =
            (CodeAddress)(((intptr_t)frontier() + 15) % 16);
        if (frontier() + numBytes > next16ByteBoundary) {
            size_t n = next16ByteBoundary - frontier();
            bytes(n, nops[n]);
            numBytes -= n;
        }
        while (numBytes >= 16) {
            bytes(16, nops[16]);
            numBytes -= 16;
        }
        bytes(numBytes, nops[numBytes]);
    }

    void byte(uint8_t b) { codeBlock.byte(b); }
    void word(uint16_t w) { codeBlock.word(w); }
    void dword(uint32_t dw) { codeBlock.dword(dw); }
    void qword(uint64_t qw) { codeBlock.qword(qw); }
    void bytes(size_t n, const uint8_t* bs) { codeBlock.bytes(n, bs); }

    /**
     * op<cc> %r
     *
     * Restrictions:
     *   r cannot be set to reg::noreg
     */
    inline __forceinline
        void emitCR(X64Instr op, int cc, RegNumber regN, int opSz = sz::qword)
    {
        assert(regN != reg::noreg);
        int rm = int(regN);
        emitPrefixBytes(op.flags, opSz);
        emitRex(op.flags, opSz, rm);
        // If op supports the R-compact encoding, use it. This produces a shorter
        // encoding, but it is not necessary for correctness.
        if (op.flags & IF_R_COMPACT) {
            // Instructions that support the R-compact encoding do not have a
            // condition code.
            assert(cc == 0);
            emitOpcodeCompact(op, opSz, rm);
            return;
        }
        emitOpcodeForwardEnc(op, opSz, cc);
        emitModrm(3, op.table[4], rm);
    }

    /**
     * op %r
     *
     * Restrictions:
     *   r cannot be set to reg::noreg
     */
    inline __forceinline
        void emitR(X64Instr op, RegNumber r, int opSz = sz::qword)
    {
        emitCR(op, 0, r, opSz);
    }

    /**
     * op<cc> %rsrc, %rdest
     *
     * Restrictions:
     *   rsrc cannot be set to reg::noreg
     *   rdest cannot be set to reg::noreg
     */
    inline __forceinline
        void emitCRR(X64Instr op, int cc, RegNumber rsrc, RegNumber rdest,
            int opSz = sz::qword)
    {
        assert(rsrc != reg::noreg && rdest != reg::noreg);
        bool reverse = (op.flags & IF_REVERSE);
        // For normal instructions, rm is the destintation. For reverse
        // instructions, r is the destination.
        int r = int(reverse ? rdest : rsrc);
        int rm = int(reverse ? rsrc : rdest);
        if ((op.flags & IF_XCHG) && rm == int(reg::rax) && r != int(reg::rax) &&
            opSz != sz::byte) {
            // For the "xchg" instruction, if r != rax and rm == rax we swap the
            // order of the operands so that r == rax. This allows us to take
            // advantage of a special encoding for "xchg" below.
            std::swap(r, rm);
        }
        else if ((op.flags & IF_XOR) && r == rm && !reg::usesRexBit(r) &&
            opSz == sz::qword) {
            // For the "xor" instruction, if r == rm and r doesn't use rex, we
            // convert "xor r64, r64" to "xor r32, r32" because the encoding is
            // strictly shorter.
            opSz = sz::dword;
        }
        else if ((op.flags & IF_MOVZX) && !reg::usesRexBit(r) &&
            !reg::usesRexBit(rm) && opSz == sz::qword &&
            (!(op.flags & IF_MOVXB) || !reg::byteRegRequiresRexPrefix(rm))) {
            // For the "movzx" instructions, if r and rm don't use rex, we convert
            // "movzx r, r64" to "movzx r, r32" because the encoding is strictly
            // shorter.
            opSz = sz::dword;
        }
        emitPrefixBytes(op.flags, opSz);
        emitRex(op.flags, opSz, r, rm);
        // The xchg instruction supports a special encoding when r == rax which
        // is strictly shorter.
        if ((op.flags & IF_XCHG) && r == int(reg::rax) && opSz != sz::byte) {
            // xchg does not have a condition code.
            assert(cc == 0);
            emitOpcodeCompact(op, opSz, rm);
            return;
        }
        emitOpcodeForwardEnc(op, opSz, cc);
        emitModrm(3, r, rm);
    }

    /**
     * op %rsrc, %rdest
     *
     * Restrictions:
     *   rsrc cannot be set to reg::noreg
     *   rdest cannot be set to reg::noreg
     */
    inline __forceinline
        void emitRR(X64Instr op, RegNumber r1, RegNumber r2, int opSz = sz::qword)
    {
        emitCRR(op, 0, r1, r2, opSz);
    }

    /**
     * op $imm, %r
     *
     * Restrictions:
     *   r cannot be set to reg::noreg
     */
    inline __forceinline
        void emitIR(X64Instr op, int64_t imm, RegNumber rname,
            int opSz = sz::qword)
    {
        assert(rname != reg::noreg);
        int rm = int(rname);
        if (unsignedValueFits(imm, sz::dword) &&
            (op.flags & (IF_MOV | IF_AND | IF_TEST))) {
            if (opSz == sz::qword &&
                (!reg::usesRexBit(rm) || !signedValueFits(imm, sz::dword))) {
                // When imm can be expressed as an unsigned 32-bit value, the following
                // conversions are safe:
                //   and  imm, r64 -> and  imm, r32
                //   test imm, r64 -> test imm, r32
                //   mov  imm, r64 -> mov  imm, r32
                // We perform these conversions when they produce a shorter encoding
                // or when imm is within the range of unsigned 32-bit integers but
                // outside the range of signed 32-bit integers.
                opSz = sz::dword;
            }
            if (opSz == sz::dword && !signedValueFits(imm, sz::dword)) {
                // For "and imm, r32", "test imm, r32", and "mov imm, r32", we support
                // cases where 2^31 <= imm < 2^32. Do 32-bit -> 64-bit sign extension
                // to satisfy various asserts in the code.
                imm = static_cast<int32_t>(imm);
                assert(signedValueFits(imm, sz::dword));
            }
        }
        int immSize = computeImmediateSize(op.flags, opSz, imm, true /*isEmitIR*/);
        emitPrefixBytes(op.flags, opSz);
        emitRex(op.flags, opSz, rm);
        // Use the rax-compact encoding if the instruction supports it. This
        // produces a shorter encoding, but it is not necessary for correctness.
        if (rm == int(reg::rax) &&
            (op.flags & IF_RAX_COMPACT) &&
            immSize == (opSz == sz::qword ? sz::dword : opSz)) {
            emitOpcodeCompact(op, opSz);
            emitImmediate(op, imm, immSize);
            return;
        }
        // When op is "mov" and the operand size and immediate size are the same,
        // we use a special compact encoding. This produces shorter encodings, and
        // when opSz == sz::qword it is necessary for correctness.
        if ((op.flags & IF_MOV) && immSize == opSz) {
            emitOpcodeCompact(op, opSz, rm);
            emitImmediate(op, imm, immSize);
            return;
        }
        // If the instruction supports 1-compact mode, use that
        if ((op.flags & IF_1_COMPACT) != 0 && imm == 1) {
            emitOpcodeCompact(op, opSz);
            emitModrm(3, op.table[4], rm);
            return;
        }
        emitOpcodeImmEnc(op, opSz, immSize, 0);
        emitModrm(3, op.table[4], rm);
        emitImmediate(op, imm, immSize);
    }

    /**
     * op $imm, %rsrc, %rdest
     *
     * Restrictions:
     *   rsrc cannot be set to reg::noreg
     *   rdest cannot be set to reg::noreg
     */
    inline __forceinline
        void emitIRR(X64Instr op, int64_t imm, RegNumber rsrc, RegNumber rdest,
            int opSz = sz::qword)
    {
        assert(rsrc != reg::noreg && rdest != reg::noreg);
        bool reverse = (op.flags & IF_REVERSE);
        // For normal instructions, rm is the destintation. For reverse
        // instructions, r is the destination.
        int r = int(reverse ? rdest : rsrc);
        int rm = int(reverse ? rsrc : rdest);
        int immSize = computeImmediateSize(op.flags, opSz, imm);
        emitPrefixBytes(op.flags, opSz);
        emitRex(op.flags, opSz, r, rm);
        emitOpcodeImmEnc(op, opSz, immSize, 0);
        emitModrm(3, r, rm);
        emitImmediate(op, imm, immSize);
    }

    /**
     * op<cc> $imm
     */
    inline __forceinline
        void emitCI(X64Instr op, int cc, int64_t imm, int opSz = sz::qword)
    {
        int immSize = computeImmediateSize(op.flags, opSz, imm);
        emitPrefixBytes(op.flags, opSz);
        emitRex(op.flags, opSz);
        emitOpcodeImmEnc(op, opSz, immSize, cc);
        emitImmediate(op, imm, immSize);
    }

    /**
     * op $imm
     */
    inline __forceinline
        void emitI(X64Instr op, int64_t imm, int opSz = sz::qword)
    {
        emitCI(op, 0, imm, opSz);
    }

    /**
     * op disp(%br,%ir,s)            [M]
     * op<cc> disp(%br,%ir,s)        [CM]
     * op $imm, disp(%br,%ir,s)      [IM]
     * op %r, disp(%br,%ir,s)        [RM]
     * op disp(%br,%ir,s), %r        [MR]
     * op<cc> disp(%br,%ir,s), %r    [CMR]
     * op $imm, %r, disp(%br,%ir,s)  [IRM]
     * op $imm, disp(%br,%ir,s), %r  [IMR]
     *
     * Restrictions:
     *     ir cannot be set to reg::rsp
     */
    inline __forceinline
        void emitCMX(X64Instr op,
            int cc,
            const MemoryRef& mr,
            bool hasR,
            RegNumber rName,
            bool rIsDest = false,
            int64_t imm = 0,
            bool hasImm = false,
            int opSz = sz::qword)
    {
        assert(!hasR || rName != reg::noreg);
        int r = int(rName);
        if (UNLIKELY((op.flags & IF_MOVZX) &&
            !reg::usesRexBit(r) &&
            !reg::usesRexBit(int(mr.base)) &&
            !reg::usesRexBit(int(mr.index)) &&
            opSz == sz::qword)) {
            assert(hasR && rIsDest);
            assert(!hasImm);
            // For "movzxbq" and "movzxwq", if r, mr.base, and mr.index don't use
            // rex, we convert "movzxbq r8, r64" to "movzxbd r8, r32" and we convert
            // "movzxwq r16, r64" to "movzxwd r16, r32" because the encoding is
            // strictly shorter.
            opSz = sz::dword;
        }
        int immSize =
            hasImm ? computeImmediateSize(op.flags, opSz, imm) : sz::nosize;
        emitPrefixBytes(op.flags, opSz);
        emitRex(op.flags, opSz, hasR, r, mr);
        if (immSize != sz::nosize) {
            // If this instruction has an immediate, use the Immediate encoding
            emitOpcodeImmEnc(op, opSz, immSize, cc);
        }
        else if (!(hasR && rIsDest) == !(op.flags & IF_REVERSE)) {
            // If there is no immediate and this a normal (or reverse) instruction
            // and rm (or r) is the destination, use the Forward encoding.
            emitOpcodeForwardEnc(op, opSz, cc);
        }
        else {
            // If there is no immediate and this a normal (or reverse) instruction
            // and r (or rm) is the destination, use the Backward encoding.
            emitOpcodeBackwardEnc(op, opSz, cc);
        }
        // Emit modr/m, and also emit the SIB byte and the displacement if needed.
        // If there is no register operand, use the instruction's "/digit" value
        // for r's reg number.
        emitModrmSibAndDisp(hasR ? r : op.table[4], mr);
        if (immSize != sz::nosize) {
            emitImmediate(op, imm, immSize);
        }
    }

    /**
     * op $imm, disp(%br,%ir,s)
     */
    inline __forceinline
        void emitIM(X64Instr op, int64_t imm, const MemoryRef& mr, int opSz = sz::qword)
    {
        emitCMX(op, 0, mr, false, reg::noreg, false, imm, true, opSz);
    }

    /**
     * op %r, disp(%br,%ir,s)
     */
    inline __forceinline
        void emitRM(X64Instr op, RegNumber r, const MemoryRef& mr, int opSz = sz::qword)
    {
        emitCMX(op, 0, mr, true, r, false, 0, false, opSz);
    }

    /**
     * op<cc> %r, disp(%br,%ir,s)
     */
    inline __forceinline
        void emitCMR(X64Instr op, int cc, const MemoryRef& mr, RegNumber r,
            int opSz = sz::qword)
    {
        emitCMX(op, cc, mr, true, r, true, 0, false, opSz);
    }

    /**
     * op disp(%br,%ir,s), %r
     */
    inline __forceinline
        void emitMR(X64Instr op, const MemoryRef& mr, RegNumber r, int opSz = sz::qword)
    {
        emitCMX(op, 0, mr, true, r, true, 0, false, opSz);
    }

    /**
     * op $imm, %r, disp(%br,%ir,s)
     */
    inline __forceinline
        void emitIRM(X64Instr op, int64_t imm, RegNumber r, const MemoryRef& mr,
            int opSz = sz::qword)
    {
        emitCMX(op, 0, mr, true, r, false, imm, true, opSz);
    }

    /**
     * op $imm, disp(%br,%ir,s), %r
     */
    inline __forceinline
        void emitIMR(X64Instr op, int64_t imm, const MemoryRef& mr, RegNumber r,
            int opSz = sz::qword)
    {
        emitCMX(op, 0, mr, true, r, true, imm, true, opSz);
    }

    /**
     * op disp(%br,%ir,s)
     */
    inline __forceinline
        void emitM(X64Instr op, const MemoryRef& mr, int opSz = sz::qword)
    {
        emitCMX(op, 0, mr, false, reg::noreg, false, 0, false, opSz);
    }

    /**
     * op<cc> disp(%br,%ir,s)
     */
    inline __forceinline
        void emitCM(X64Instr op, int cc, const MemoryRef& mr,
            int opSz = sz::qword)
    {
        emitCMX(op, cc, mr, false, reg::noreg, false, 0, false, opSz);
    }

    /**
     * op (with no arguments)
     */
    inline __forceinline
        void emitNoArgs(X64Instr op, int opSz = sz::qword)
    {
        // Emit any prefixes needed by op (not including the REX prefix)
        emitPrefixBytes(op.flags, opSz);
        // Emit the REX prefix if needed
        if (opSz == sz::qword && !(op.flags & IF_NO_REXW)) {
            byte(0x48);
        }
        // Emit the opcode
        emitOpcodeCompact(op, opSz);
    }

    /**
     * Segment override prefixes
     */
    inline __forceinline
        void fs()
    {
        byte(0x64);
    }

    inline __forceinline
        void gs()
    {
        byte(0x65);
    }

    /**
     * Lock prefix
     */
    inline __forceinline
        void lock()
    {
        byte(0xF0);
    }

    /**
     * Helper functions for patching an already-emitted "jcc rel32", "jcc rel8",
     * "jmp rel32", "jmp rel8", or "call rel32" instruction to point to the
     * desired target address. The `instrAddr` parameter is the absolute address
     * of the first byte of the jcc/jmp/call instruction to be patched, and the
     * `targetAddr` parameter is the absolute address of the desired target.
     * These helper functions will take care of converting `targetAddr` into a
     * relative offset.
     *
     * This allows users to emit a jcc/jmp/call instruction before the target
     * address is known and then update the instruction at a later time once
     * the target address is known.
     */
    static void patchJcc(CodeAddress instrAddr, CodeAddress targetAddr) {
        assert(instrAddr[0] == 0x0F && (instrAddr[1] & 0xF0) == 0x80);
        int64_t delta = targetAddr - (instrAddr + 6); // 2 for opcode, 4 for offset
        assert(signedValueFits(delta, sz::dword));
        *(int32_t*)(instrAddr + 2) = static_cast<int32_t>(delta);
    }

    static void patchJcc8(CodeAddress instrAddr, CodeAddress targetAddr) {
        assert((instrAddr[0] & 0xF0) == 0x70);
        int64_t delta = targetAddr - (instrAddr + 2); // 1 for opcode, 4 for offset
        assert(signedValueFits(delta, sz::byte));
        *(int8_t*)(instrAddr + 1) = static_cast<int8_t>(delta);
    }

    static void patchJmp(CodeAddress instrAddr, CodeAddress targetAddr) {
        assert(instrAddr[0] == 0xE9);
        int64_t delta = targetAddr - (instrAddr + 5); // 1 for opcode, 4 for offset
        assert(signedValueFits(delta, sz::dword));
        *(int32_t*)(instrAddr + 1) = static_cast<int32_t>(delta);
    }

    static void patchJmp8(CodeAddress instrAddr, CodeAddress targetAddr) {
        assert(instrAddr[0] == 0xEB);
        int64_t delta = targetAddr - (instrAddr + 2); // 1 for opcode, 1 for offset
        assert(signedValueFits(delta, sz::byte));
        *(int8_t*)(instrAddr + 1) = static_cast<int8_t>(delta);
    }

    static void patchCall(CodeAddress instrAddr, CodeAddress targetAddr) {
        assert(instrAddr[0] == 0xE8);
        int64_t delta = targetAddr - (instrAddr + 5); // 1 for opcode, 4 for offset
        assert(signedValueFits(delta, sz::dword));
        *(int32_t*)(instrAddr + 1) = static_cast<int32_t>(delta);
    }

private:
    inline __forceinline
        void emitOpcodeForwardEnc(X64Instr op, int opSz, int cc = 0)
    {
        emitEscapeBytes(op.flags);
        unsigned char opcode = (op.table[0] | (cc & 0x0F));
        if (opSz == sz::byte && !(op.flags & IF_SETCC)) {
            assert(cc == 0);
            opcode &= ~1;
        }
        byte(opcode);
    }

    inline __forceinline
        void emitOpcodeBackwardEnc(X64Instr op, int opSz, int cc = 0)
    {
        emitEscapeBytes(op.flags);
        unsigned char opcode = (op.table[1] | (cc & 0x0F));
        if (opSz == sz::byte && !(op.flags & IF_SETCC)) {
            assert(cc == 0);
            opcode &= ~1;
        }
        byte(opcode);
    }

    inline __forceinline
        void emitOpcodeImmEnc(X64Instr op, int opSz, int immSize, int cc = 0)
    {
        assert(immSize != sz::nosize);
        emitEscapeBytes(op.flags);
        unsigned char opcode = (op.table[2] | (cc & 0x0F));
        if (opSz == sz::byte && !(op.flags & IF_SETCC)) {
            assert(cc == 0);
            opcode &= ~1;
        }
        if (opSz != sz::byte && immSize == sz::byte &&
            (op.flags & IF_HAS_SHORTER_IMM)) {
            assert(cc == 0);
            opcode |= 2;
        }
        byte(opcode);
    }

    inline __forceinline
        void emitOpcodeCompact(X64Instr op, int opSz)
    {
        emitEscapeBytes(op.flags);
        unsigned char opcode = op.table[3];
        if (opSz == sz::byte) opcode &= ~1;
        byte(opcode);
    }

    inline __forceinline
        void emitOpcodeCompact(X64Instr op, int opSz, int rm)
    {
        assert(op.flags & (IF_R_COMPACT | IF_MOV | IF_XCHG));
        assert(rm != int(reg::noreg));
        emitEscapeBytes(op.flags);
        unsigned char opcode = (op.table[3] | (rm & 7));
        if (opSz == sz::byte) opcode &= ~8;
        byte(opcode);
    }

    __declspec(noinline)
        void emitModrmSibAndDisp(int r, const MemoryRef& mr);

    inline __forceinline
        void emitPrefixBytes(unsigned int flags, int opSz)
    {
        // Emit the opsize prefix if necessary for word address modes
        if (opSz == sz::word) {
            byte(kOpsizePrefix);
        }
        // Emit op's mandatory prefix (if it has one)
        if (flags & IF_PREFIX_66) {
            byte(0x66);
        }
        else if (flags & IF_PREFIX_F2) {
            byte(0xF2);
        }
        else if (flags & IF_PREFIX_F3) {
            byte(0xF3);
        }
    }

    inline __forceinline
        void emitEscapeBytes(unsigned int flags)
    {
        if (flags & IF_ESCAPE_0F) {
            byte(0x0F);
        }
        else if (flags & IF_ESCAPE_0F3A) {
            byte(0x0F);
            byte(0x3A);
        }
    }

    /**
     * emitRex() emits the encoding for the REX prefix, or it emits nothing
     * if no REX prefix is needed.
     */
    inline __forceinline
        void emitRex(unsigned int flags, int opSz)
    {
        unsigned char rex =
            (!(flags & IF_NO_REXW) && opSz == sz::qword) << 3;  // rex.W
          // If rex.W is set, then we need to emit a REX prefix.
        if (rex) {
            byte(rex | 0x40);
        }
    }

    inline __forceinline
        void emitRex(unsigned int flags, int opSz, int rm)
    {
        emitRex(flags, opSz, false, int(reg::noreg), rm, int(reg::noreg),
            false /*isEmitCMX*/);
    }

    inline __forceinline
        void emitRex(unsigned int flags, int opSz, int r, int rm)
    {
        emitRex(flags, opSz, true, r, rm, int(reg::noreg), false /*isEmitCMX*/);
    }

    inline __forceinline
        void emitRex(unsigned int flags, int opSz, bool hasR, int r, const MemoryRef& mr)
    {
        emitRex(flags, opSz, hasR, r, int(mr.base), int(mr.index),
            true /*isEmitCMX*/);
    }

    inline __forceinline
        void emitRex(unsigned int flags, int opSz, bool hasR, int r, int rm, int x,
            bool isEmitCMX)
    {
        assert(isEmitCMX || rm != int(reg::noreg));
        assert(!hasR || r != int(reg::noreg));
        // The wily rex byte, a multipurpose extension to the opcode space for x64
        unsigned char rex =
            (!(flags & IF_NO_REXW) && opSz == sz::qword) << 3 |  // rex.W
            (hasR && reg::usesRexBit(r)) << 2 |                  // rex.R
            reg::usesRexBit(x) << 1 |                            // rex.X
            reg::usesRexBit(rm);                                 // rex.B
          // Determine which are byte registers.
        bool rIsByteReg = (hasR && opSz == sz::byte);
        bool rmIsByteReg =
            (!isEmitCMX && (opSz == sz::byte || (flags & (IF_MOVXB | IF_SETCC))));
        // If one or more of rex.W, rex.R, rex.X, and rex.B are set, or if this
        // instruction uses spl/bpl/sil/dil, then we need to emit a REX prefix.
        if (rex || rIsByteReg && reg::byteRegRequiresRexPrefix(r) ||
            rmIsByteReg && reg::byteRegRequiresRexPrefix(rm)) {
            if (rIsByteReg && reg::isHighByteReg(r) ||
                rmIsByteReg && reg::isHighByteReg(rm)) {
                // Instructions with a REX prefix are not allowed to use high-byte regs
                reg::highByteRegMisuse();
            }
            byte(rex | 0x40);
        }
    }

    inline __forceinline
        void emitModrm(int mod, int r, int rm)
    {
        byte((mod << 6) | ((r & 7) << 3) | (rm & 7));
    }

    inline __forceinline
        int computeImmediateSize(unsigned int flags,
            int opSz,
            int64_t imm,
            bool isEmitIR = false)
    {
        // Some instructions only support 8-bit immediates. Other instructions
        // only support 16-bit immediates.
        if (flags & IF_BYTE_IMM_ONLY) {
            assert(signedValueFits(imm, sz::byte));
            return sz::byte;
        }
        else if (flags & IF_WORD_IMM_ONLY) {
            assert(signedValueFits(imm, sz::word));
            return sz::word;
        }
        // Use an 8-bit immediate if the instruction supports it and
        // if the immediate value fits in a byte.
        if ((flags & IF_HAS_SHORTER_IMM) && signedValueFits(imm, sz::byte)) {
            return sz::byte;
        }
        // If opSz is sz::byte or sz::word, we use an 8-bit immediate or
        // 16-bit immediate, respectively.
        if (opSz == sz::byte || opSz == sz::word) {
            assert(signedValueFits(imm, opSz));
            return opSz;
        }
        // "movq imm, r64" is the only x64 instruction that supports
        // 64-bit immediates.
        if (isEmitIR && (flags & IF_MOV) && opSz == sz::qword &&
            !signedValueFits(imm, sz::dword)) {
            return sz::qword;
        }
        // Most instructions take a 32-bit immediate.
        assert(signedValueFits(imm, sz::dword));
        return sz::dword;
    }

    inline __forceinline
        void emitImmediate(X64Instr op, int64_t imm, int immSize)
    {
        assert(immSize == sz::byte || !(op.flags & IF_BYTE_IMM_ONLY));
        assert(immSize == sz::word || !(op.flags & IF_WORD_IMM_ONLY));
        switch (immSize) {
        case sz::byte: byte(imm); break;
        case sz::word: word(imm); break;
        case sz::dword: dword(imm); break;
        case sz::qword: qword(imm); break;
        default: assert(false); break;
        }
    }

private:
    inline __forceinline
        RegNumber rn(Reg8 r) { return RegNumber(r); }
    inline __forceinline
        RegNumber rn(Reg16 r) { return RegNumber(r); }
    inline __forceinline
        RegNumber rn(Reg32 r) { return RegNumber(r); }
    inline __forceinline
        RegNumber rn(Reg64 r) { return RegNumber(r); }
    inline __forceinline
        RegNumber rn(RegXMM x) { return RegNumber(x); }
    inline __forceinline
        RegNumber rn(RegMMX x) { return RegNumber(x); }

    CodeBlock& codeBlock;

    //////////////////////////////////////////////////////////////////////

#define X64_INSTR_R_QL(INSTR_NAME) \
  X64_INSTR_R_Q(INSTR_NAME) \
  X64_INSTR_R_L(INSTR_NAME)

#define X64_INSTR_R_QLW(INSTR_NAME) \
  X64_INSTR_R_QL(INSTR_NAME) \
  X64_INSTR_R_W(INSTR_NAME)

#define X64_INSTR_R_QLWB(INSTR_NAME) \
  X64_INSTR_R_QLW(INSTR_NAME) \
  X64_INSTR_R_B(INSTR_NAME)

#define X64_INSTR_R_QW(INSTR_NAME) \
  X64_INSTR_R_Q(INSTR_NAME) \
  X64_INSTR_R_W(INSTR_NAME)

#define X64_INSTR_M_QL(INSTR_NAME) \
  X64_INSTR_M_Q(INSTR_NAME) \
  X64_INSTR_M_L(INSTR_NAME)

#define X64_INSTR_M_QLW(INSTR_NAME) \
  X64_INSTR_M_QL(INSTR_NAME) \
  X64_INSTR_M_W(INSTR_NAME)

#define X64_INSTR_M_QLWB(INSTR_NAME) \
  X64_INSTR_M_QLW(INSTR_NAME) \
  X64_INSTR_M_B(INSTR_NAME)

#define X64_INSTR_M_QW(INSTR_NAME) \
  X64_INSTR_M_Q(INSTR_NAME) \
  X64_INSTR_M_W(INSTR_NAME)

#define X64_INSTR_RR_QLW(INSTR_NAME) \
  X64_INSTR_RR_Q(INSTR_NAME) \
  X64_INSTR_RR_L(INSTR_NAME) \
  X64_INSTR_RR_W(INSTR_NAME)

#define X64_INSTR_RR_QLWB(INSTR_NAME) \
  X64_INSTR_RR_QLW(INSTR_NAME) \
  X64_INSTR_RR_B(INSTR_NAME)

#define X64_INSTR_IR_QLW(INSTR_NAME) \
  X64_INSTR_IR_Q(INSTR_NAME) \
  X64_INSTR_IR_L(INSTR_NAME) \
  X64_INSTR_IR_W(INSTR_NAME)

#define X64_INSTR_IR_QLWB(INSTR_NAME) \
  X64_INSTR_IR_QLW(INSTR_NAME) \
  X64_INSTR_IR_B(INSTR_NAME)

#define X64_INSTR_IM_QLW(INSTR_NAME) \
  X64_INSTR_IM_Q(INSTR_NAME) \
  X64_INSTR_IM_L(INSTR_NAME) \
  X64_INSTR_IM_W(INSTR_NAME)

#define X64_INSTR_IM_QLWB(INSTR_NAME) \
  X64_INSTR_IM_QLW(INSTR_NAME) \
  X64_INSTR_IM_B(INSTR_NAME)

#define X64_INSTR_RM_QLW(INSTR_NAME) \
  X64_INSTR_RM_Q(INSTR_NAME) \
  X64_INSTR_RM_L(INSTR_NAME) \
  X64_INSTR_RM_W(INSTR_NAME)

#define X64_INSTR_RM_QLWB(INSTR_NAME) \
  X64_INSTR_RM_QLW(INSTR_NAME) \
  X64_INSTR_RM_B(INSTR_NAME)

#define X64_INSTR_MR_QLW(INSTR_NAME) \
  X64_INSTR_MR_Q(INSTR_NAME) \
  X64_INSTR_MR_L(INSTR_NAME) \
  X64_INSTR_MR_W(INSTR_NAME)

#define X64_INSTR_MR_QLWB(INSTR_NAME) \
  X64_INSTR_MR_QLW(INSTR_NAME) \
  X64_INSTR_MR_B(INSTR_NAME)

#define X64_INSTR_IRR_QLW(INSTR_NAME) \
  X64_INSTR_IRR_Q(INSTR_NAME) \
  X64_INSTR_IRR_L(INSTR_NAME) \
  X64_INSTR_IRR_W(INSTR_NAME)

#define X64_INSTR_IRM_QLW(INSTR_NAME) \
  X64_INSTR_IRM_Q(INSTR_NAME) \
  X64_INSTR_IRM_L(INSTR_NAME) \
  X64_INSTR_IRM_W(INSTR_NAME)

//////////////////////////////////////////////////////////////////////

#define X64_INSTRS() \
  X64_INSTR_NO_ARGS(ret) \
  X64_INSTR_NO_ARGS(nop) \
  X64_INSTR_NO_ARGS(int3) \
  X64_INSTR_NO_ARGS(cqo) \
  X64_INSTR_NO_ARGS(cdqe) \
  X64_INSTR_NO_ARGS(pushf) \
  X64_INSTR_NO_ARGS(popf) \
  X64_INSTR_NO_ARGS(xlat) \
  X64_INSTR_NO_ARGS(cmc) \
  X64_INSTR_NO_ARGS(clc) \
  X64_INSTR_NO_ARGS(stc) \
  X64_INSTR_I(jmp) \
  X64_INSTR_I(jmp8) \
  X64_INSTR_I(push) \
  X64_INSTR_I(ret) \
  X64_INSTR_I(call) \
  X64_INSTR_R_QLWB(inc) \
  X64_INSTR_R_QLWB(dec) \
  X64_INSTR_R_QLWB(not) \
  X64_INSTR_R_QLWB(neg) \
  X64_INSTR_R_QLWB(mul) \
  X64_INSTR_R_QLWB(div) \
  X64_INSTR_R_QLWB(idiv) \
  X64_INSTR_R_QLWB(imul) \
  X64_INSTR_R_QLWB(rol) \
  X64_INSTR_R_QLWB(ror) \
  X64_INSTR_R_QLWB(rcl) \
  X64_INSTR_R_QLWB(rcr) \
  X64_INSTR_R_QLWB(shl) \
  X64_INSTR_R_QLWB(shr) \
  X64_INSTR_R_QLWB(sar) \
  X64_INSTR_R_QL(bswap) \
  X64_INSTR_R_QW(push) \
  X64_INSTR_R_QW(pop) \
  X64_INSTR_R_Q(jmp) \
  X64_INSTR_R_Q(call) \
  X64_INSTR_M_QLWB(inc) \
  X64_INSTR_M_QLWB(dec) \
  X64_INSTR_M_QLWB(not) \
  X64_INSTR_M_QLWB(neg) \
  X64_INSTR_M_QLWB(mul) \
  X64_INSTR_M_QLWB(div) \
  X64_INSTR_M_QLWB(idiv) \
  X64_INSTR_M_QLWB(imul) \
  X64_INSTR_M_QLWB(rol) \
  X64_INSTR_M_QLWB(ror) \
  X64_INSTR_M_QLWB(rcl) \
  X64_INSTR_M_QLWB(rcr) \
  X64_INSTR_M_QLWB(shl) \
  X64_INSTR_M_QLWB(shr) \
  X64_INSTR_M_QLWB(sar) \
  X64_INSTR_M_QW(push) \
  X64_INSTR_M_QW(pop) \
  X64_INSTR_M_Q(jmp) \
  X64_INSTR_M_Q(call) \
  X64_INSTR_RR_QLWB(xchg) \
  X64_INSTR_RR_QLWB(add) \
  X64_INSTR_RR_QLWB(sub) \
  X64_INSTR_RR_QLWB(and) \
  X64_INSTR_RR_QLWB(or) \
  X64_INSTR_RR_QLWB(xor) \
  X64_INSTR_RR_QLWB(cmp) \
  X64_INSTR_RR_QLWB(sbb) \
  X64_INSTR_RR_QLWB(adc) \
  X64_INSTR_RR_QLWB(mov) \
  X64_INSTR_RR_QLWB(test) \
  X64_INSTR_RR_QLWB(xadd) \
  X64_INSTR_RR_QLWB(cmpxchg) \
  X64_INSTR_RR_QLW(shld) \
  X64_INSTR_RR_QLW(shrd) \
  X64_INSTR_RR_QLW(popcnt) \
  X64_INSTR_RR_XMM(movsd) \
  X64_INSTR_RR_XMM(addsd) \
  X64_INSTR_RR_XMM(subsd) \
  X64_INSTR_RR_XMM(mulsd) \
  X64_INSTR_RR_XMM(divsd) \
  X64_INSTR_RR_XMM(sqrtsd) \
  X64_INSTR_RR_XMM(movss) \
  X64_INSTR_RR_XMM(addss) \
  X64_INSTR_RR_XMM(subss) \
  X64_INSTR_RR_XMM(mulss) \
  X64_INSTR_RR_XMM(divss) \
  X64_INSTR_RR_XMM(sqrtss) \
  X64_INSTR_RR_XMM(ucomisd) \
  X64_INSTR_RR_XMM(pxor) \
  X64_INSTR_RR_XMM(psllq) \
  X64_INSTR_RR_XMM(psrlq) \
  X64_INSTR_RR_XMM(movdqa) \
  X64_INSTR_RR_XMM(movdqu) \
  X64_INSTR_RR_XMM(movaps) \
  X64_INSTR_RR_XMM(movups) \
  X64_INSTR_RR_XMM(movapd) \
  X64_INSTR_RR_XMM(movupd) \
  X64_INSTR_RR_XMM(unpcklpd) \
  X64_INSTR_IR_QLWB(add) \
  X64_INSTR_IR_QLWB(sub) \
  X64_INSTR_IR_QLWB(and) \
  X64_INSTR_IR_QLWB(or) \
  X64_INSTR_IR_QLWB(xor) \
  X64_INSTR_IR_QLWB(cmp) \
  X64_INSTR_IR_QLWB(adc) \
  X64_INSTR_IR_QLWB(sbb) \
  X64_INSTR_IR_QLWB(mov) \
  X64_INSTR_IR_QLWB(test) \
  X64_INSTR_IR_QLWB(rol) \
  X64_INSTR_IR_QLWB(ror) \
  X64_INSTR_IR_QLWB(rcl) \
  X64_INSTR_IR_QLWB(rcr) \
  X64_INSTR_IR_QLWB(shl) \
  X64_INSTR_IR_QLWB(shr) \
  X64_INSTR_IR_QLWB(sar) \
  X64_INSTR_IR_XMM(psllq) \
  X64_INSTR_IR_XMM(psrlq) \
  X64_INSTR_IM_QLWB(add) \
  X64_INSTR_IM_QLWB(sub) \
  X64_INSTR_IM_QLWB(and) \
  X64_INSTR_IM_QLWB(or) \
  X64_INSTR_IM_QLWB(xor) \
  X64_INSTR_IM_QLWB(cmp) \
  X64_INSTR_IM_QLWB(adc) \
  X64_INSTR_IM_QLWB(sbb) \
  X64_INSTR_IM_QLWB(mov) \
  X64_INSTR_IM_QLWB(test) \
  X64_INSTR_IM_QLWB(rol) \
  X64_INSTR_IM_QLWB(ror) \
  X64_INSTR_IM_QLWB(rcl) \
  X64_INSTR_IM_QLWB(rcr) \
  X64_INSTR_IM_QLWB(shl) \
  X64_INSTR_IM_QLWB(shr) \
  X64_INSTR_IM_QLWB(sar) \
  X64_INSTR_RM_QLWB(xchg) \
  X64_INSTR_RM_QLWB(add) \
  X64_INSTR_RM_QLWB(sub) \
  X64_INSTR_RM_QLWB(and) \
  X64_INSTR_RM_QLWB(or) \
  X64_INSTR_RM_QLWB(xor) \
  X64_INSTR_RM_QLWB(cmp) \
  X64_INSTR_RM_QLWB(adc) \
  X64_INSTR_RM_QLWB(sbb) \
  X64_INSTR_RM_QLWB(mov) \
  X64_INSTR_RM_QLWB(test) \
  X64_INSTR_RM_QLWB(xadd) \
  X64_INSTR_RM_QLWB(cmpxchg) \
  X64_INSTR_RM_QLW(shld) \
  X64_INSTR_RM_QLW(shrd) \
  X64_INSTR_RM_XMM(movsd) \
  X64_INSTR_RM_XMM(movss) \
  X64_INSTR_RM_XMM(movdqa) \
  X64_INSTR_RM_XMM(movdqu) \
  X64_INSTR_RM_XMM(movaps) \
  X64_INSTR_RM_XMM(movups) \
  X64_INSTR_RM_XMM(movapd) \
  X64_INSTR_RM_XMM(movupd) \
  X64_INSTR_MR_QLWB(xchg) \
  X64_INSTR_MR_QLWB(add) \
  X64_INSTR_MR_QLWB(sub) \
  X64_INSTR_MR_QLWB(and) \
  X64_INSTR_MR_QLWB(or) \
  X64_INSTR_MR_QLWB(xor) \
  X64_INSTR_MR_QLWB(cmp) \
  X64_INSTR_MR_QLWB(adc) \
  X64_INSTR_MR_QLWB(sbb) \
  X64_INSTR_MR_QLWB(mov) \
  X64_INSTR_MR_QLWB(test) \
  X64_INSTR_MR_QLW(lea) \
  X64_INSTR_MR_XMM(movsd) \
  X64_INSTR_MR_XMM(addsd) \
  X64_INSTR_MR_XMM(subsd) \
  X64_INSTR_MR_XMM(mulsd) \
  X64_INSTR_MR_XMM(divsd) \
  X64_INSTR_MR_XMM(sqrtsd) \
  X64_INSTR_MR_XMM(movss) \
  X64_INSTR_MR_XMM(addss) \
  X64_INSTR_MR_XMM(subss) \
  X64_INSTR_MR_XMM(mulss) \
  X64_INSTR_MR_XMM(divss) \
  X64_INSTR_MR_XMM(sqrtss) \
  X64_INSTR_MR_XMM(ucomisd) \
  X64_INSTR_MR_XMM(pxor) \
  X64_INSTR_MR_XMM(psllq) \
  X64_INSTR_MR_XMM(psrlq) \
  X64_INSTR_MR_XMM(movdqa) \
  X64_INSTR_MR_XMM(movdqu) \
  X64_INSTR_MR_XMM(movaps) \
  X64_INSTR_MR_XMM(movups) \
  X64_INSTR_MR_XMM(movapd) \
  X64_INSTR_MR_XMM(movupd) \
  X64_INSTR_MR_XMM(lddqu) \
  X64_INSTR_MR_XMM(unpcklpd) \
  X64_INSTR_IRR_QLW(shld) \
  X64_INSTR_IRR_QLW(shrd) \
  X64_INSTR_IRR_XMM(roundsd) \
  X64_INSTR_IRM_QLW(shld) \
  X64_INSTR_IRM_QLW(shrd) \
  X64_INSTR_IMR_XMM(roundsd)

//////////////////////////////////////////////////////////////////////

public:

#define X64_INSTR_NO_ARGS(INSTR_NAME) \
  void INSTR_NAME ();

#define X64_INSTR_I(INSTR_NAME) \
  void INSTR_NAME (int64_t imm);

#define X64_INSTR_R_Q(INSTR_NAME) \
  void INSTR_NAME ## q (Reg64 r);

#define X64_INSTR_R_L(INSTR_NAME) \
  void INSTR_NAME ## l (Reg32 r);

#define X64_INSTR_R_W(INSTR_NAME) \
  void INSTR_NAME ## w (Reg16 r);

#define X64_INSTR_R_B(INSTR_NAME) \
  void INSTR_NAME ## b (Reg8 r);

#define X64_INSTR_M_Q(INSTR_NAME) \
  void INSTR_NAME ## q (const MemoryRef& mr);

#define X64_INSTR_M_L(INSTR_NAME) \
  void INSTR_NAME ## l (const MemoryRef& mr);

#define X64_INSTR_M_W(INSTR_NAME) \
  void INSTR_NAME ## w (const MemoryRef& mr);

#define X64_INSTR_M_B(INSTR_NAME) \
  void INSTR_NAME ## b (const MemoryRef& mr);

#define X64_INSTR_RR_Q(INSTR_NAME) \
  void INSTR_NAME ## q (Reg64 rsrc, Reg64 rdest);

#define X64_INSTR_RR_L(INSTR_NAME) \
  void INSTR_NAME ## l (Reg32 rsrc, Reg32 rdest);

#define X64_INSTR_RR_W(INSTR_NAME) \
  void INSTR_NAME ## w (Reg16 rsrc, Reg16 rdest);

#define X64_INSTR_RR_B(INSTR_NAME) \
  void INSTR_NAME ## b (Reg8 rsrc, Reg8 rdest);

#define X64_INSTR_RR_XMM(INSTR_NAME) \
  void INSTR_NAME (RegXMM rsrc, RegXMM rdest);

#define X64_INSTR_IR_Q(INSTR_NAME) \
  void INSTR_NAME ## q (int64_t imm, Reg64 r);

#define X64_INSTR_IR_L(INSTR_NAME) \
  void INSTR_NAME ## l (int64_t imm, Reg32 r);

#define X64_INSTR_IR_W(INSTR_NAME) \
  void INSTR_NAME ## w (int64_t imm, Reg16 r);

#define X64_INSTR_IR_B(INSTR_NAME) \
  void INSTR_NAME ## b (int64_t imm, Reg8 r);

#define X64_INSTR_IR_XMM(INSTR_NAME) \
  void INSTR_NAME (int64_t imm, RegXMM r);

#define X64_INSTR_IM_Q(INSTR_NAME) \
  void INSTR_NAME ## q (int64_t imm, const MemoryRef& mr);

#define X64_INSTR_IM_L(INSTR_NAME) \
  void INSTR_NAME ## l (int64_t imm, const MemoryRef& mr);

#define X64_INSTR_IM_W(INSTR_NAME) \
  void INSTR_NAME ## w (int64_t imm, const MemoryRef& mr);

#define X64_INSTR_IM_B(INSTR_NAME) \
  void INSTR_NAME ## b (int64_t imm, const MemoryRef& mr);

#define X64_INSTR_RM_Q(INSTR_NAME) \
  void INSTR_NAME ## q (Reg64 r, const MemoryRef& mr);

#define X64_INSTR_RM_L(INSTR_NAME) \
  void INSTR_NAME ## l (Reg32 r, const MemoryRef& mr);

#define X64_INSTR_RM_W(INSTR_NAME) \
  void INSTR_NAME ## w (Reg16 r, const MemoryRef& mr);

#define X64_INSTR_RM_B(INSTR_NAME) \
  void INSTR_NAME ## b (Reg8 r, const MemoryRef& mr);

#define X64_INSTR_RM_XMM(INSTR_NAME) \
  void INSTR_NAME (RegXMM r, const MemoryRef& mr);

#define X64_INSTR_MR_Q(INSTR_NAME) \
  void INSTR_NAME ## q (const MemoryRef& mr, Reg64 r);

#define X64_INSTR_MR_L(INSTR_NAME) \
  void INSTR_NAME ## l (const MemoryRef& mr, Reg32 r);

#define X64_INSTR_MR_W(INSTR_NAME) \
  void INSTR_NAME ## w (const MemoryRef& mr, Reg16 r);

#define X64_INSTR_MR_B(INSTR_NAME) \
  void INSTR_NAME ## b (const MemoryRef& mr, Reg8 r);

#define X64_INSTR_MR_XMM(INSTR_NAME) \
  void INSTR_NAME (const MemoryRef& mr, RegXMM r);

#define X64_INSTR_IRR_Q(INSTR_NAME) \
  void INSTR_NAME ## q (int64_t imm, Reg64 rsrc, Reg64 rdest);

#define X64_INSTR_IRR_L(INSTR_NAME) \
  void INSTR_NAME ## l (int64_t imm, Reg32 rsrc, Reg32 rdest);

#define X64_INSTR_IRR_W(INSTR_NAME) \
  void INSTR_NAME ## w (int64_t imm, Reg16 rsrc, Reg16 rdest);

#define X64_INSTR_IRR_B(INSTR_NAME) \
  void INSTR_NAME ## b (int64_t imm, Reg8 rsrc, Reg8 rdest);

#define X64_INSTR_IRR_XMM(INSTR_NAME) \
  void INSTR_NAME (int64_t imm, RegXMM rsrc, RegXMM rdest);

#define X64_INSTR_IRM_Q(INSTR_NAME) \
  void INSTR_NAME ## q (int64_t imm, Reg64 r, const MemoryRef& mr);

#define X64_INSTR_IRM_L(INSTR_NAME) \
  void INSTR_NAME ## l (int64_t imm, Reg32 r, const MemoryRef& mr);

#define X64_INSTR_IRM_W(INSTR_NAME) \
  void INSTR_NAME ## w (int64_t imm, Reg16 r, const MemoryRef& mr);

#define X64_INSTR_IRM_B(INSTR_NAME) \
  void INSTR_NAME ## b (int64_t imm, Reg8 r, const MemoryRef& mr);

#define X64_INSTR_IMR_XMM(INSTR_NAME) \
  void INSTR_NAME (int64_t imm, const MemoryRef& mr, RegXMM r);

    X64_INSTRS()

#undef X64_INSTR_NO_ARGS
#undef X64_INSTR_I
#undef X64_INSTR_R_Q
#undef X64_INSTR_R_L
#undef X64_INSTR_R_W
#undef X64_INSTR_R_B
#undef X64_INSTR_M_Q
#undef X64_INSTR_M_L
#undef X64_INSTR_M_W
#undef X64_INSTR_M_B
#undef X64_INSTR_RR_Q
#undef X64_INSTR_RR_L
#undef X64_INSTR_RR_W
#undef X64_INSTR_RR_B
#undef X64_INSTR_RR_XMM
#undef X64_INSTR_IR_Q
#undef X64_INSTR_IR_L
#undef X64_INSTR_IR_W
#undef X64_INSTR_IR_B
#undef X64_INSTR_IR_XMM
#undef X64_INSTR_IM_Q
#undef X64_INSTR_IM_L
#undef X64_INSTR_IM_W
#undef X64_INSTR_IM_B
#undef X64_INSTR_RM_Q
#undef X64_INSTR_RM_L
#undef X64_INSTR_RM_W
#undef X64_INSTR_RM_B
#undef X64_INSTR_RM_XMM
#undef X64_INSTR_MR_Q
#undef X64_INSTR_MR_L
#undef X64_INSTR_MR_W
#undef X64_INSTR_MR_B
#undef X64_INSTR_MR_XMM
#undef X64_INSTR_IRR_Q
#undef X64_INSTR_IRR_L
#undef X64_INSTR_IRR_W
#undef X64_INSTR_IRR_B
#undef X64_INSTR_IRR_XMM
#undef X64_INSTR_IRM_Q
#undef X64_INSTR_IRM_L
#undef X64_INSTR_IRM_W
#undef X64_INSTR_IRM_B
#undef X64_INSTR_IMR_XMM

        void cdq();
    void cwde();
    void cwd();
    void cbw();
    void pushfw();
    void popfw();

    void pushw(int64_t imm);

    void cmpxchg16b(const MemoryRef& mr);
    void cmpxchg8b(const MemoryRef& mr);

    void imulq(Reg64 rsrc, Reg64 rdest);
    void imull(Reg32 rsrc, Reg32 rdest);
    void imulw(Reg16 rsrc, Reg16 rdest);

    void movzxwq(Reg16 rsrc, Reg64 rdest);
    void movzxwl(Reg16 rsrc, Reg32 rdest);
    void movzxbq(Reg8 rsrc, Reg64 rdest);
    void movzxbl(Reg8 rsrc, Reg32 rdest);
    void movzxbw(Reg8 rsrc, Reg16 rdest);

    void movsxdq(Reg32 rsrc, Reg64 rdest);
    void movsxwq(Reg16 rsrc, Reg64 rdest);
    void movsxwl(Reg16 rsrc, Reg32 rdest);
    void movsxbq(Reg8 rsrc, Reg64 rdest);
    void movsxbl(Reg8 rsrc, Reg32 rdest);
    void movsxbw(Reg8 rsrc, Reg16 rdest);

    void movq(Reg64 rsrc, RegXMM rdest);
    void movq(RegXMM rsrc, Reg64 rdest);
    void movl(Reg32 rsrc, RegXMM rdest);
    void movl(RegXMM rsrc, Reg32 rdest);

    void cvtsi2sdq(Reg64 rsrc, RegXMM rdest);
    void cvtsi2sdl(Reg32 rsrc, RegXMM rdest);
    void cvttsd2siq(RegXMM rsrc, Reg64 rdest);
    void cvttsd2sil(RegXMM rsrc, Reg32 rdest);

    void movq(RegXMM r, const MemoryRef& mr);
    void movl(RegXMM r, const MemoryRef& mr);

    void imulq(const MemoryRef& mr, Reg64 r);
    void imull(const MemoryRef& mr, Reg32 r);
    void imulw(const MemoryRef& mr, Reg16 r);

    void movzxwq(const MemoryRef& mr, Reg64 r);
    void movzxwl(const MemoryRef& mr, Reg32 r);
    void movzxbq(const MemoryRef& mr, Reg64 r);
    void movzxbl(const MemoryRef& mr, Reg32 r);
    void movzxbw(const MemoryRef& mr, Reg16 r);
    void movsxdq(const MemoryRef& mr, Reg64 r);
    void movsxwq(const MemoryRef& mr, Reg64 r);
    void movsxwl(const MemoryRef& mr, Reg32 r);
    void movsxbq(const MemoryRef& mr, Reg64 r);
    void movsxbl(const MemoryRef& mr, Reg32 r);
    void movsxbw(const MemoryRef& mr, Reg16 r);

    void movq(const MemoryRef& mr, RegXMM r);
    void movl(const MemoryRef& mr, RegXMM r);

    void cvtsi2sdq(const MemoryRef& mr, RegXMM r);
    void cvtsi2sdl(const MemoryRef& mr, RegXMM r);
    void cvttsd2siq(const MemoryRef& mr, Reg64 r);
    void cvttsd2sil(const MemoryRef& mr, Reg32 r);

    void imulq(int64_t imm, Reg64 rsrc, Reg64 rdest);
    void imull(int64_t imm, Reg32 rsrc, Reg32 rdest);
    void imulw(int64_t imm, Reg16 rsrc, Reg16 rdest);

    void imulq(int64_t imm, const MemoryRef& mr, Reg64 r);
    void imull(int64_t imm, const MemoryRef& mr, Reg32 r);
    void imulw(int64_t imm, const MemoryRef& mr, Reg16 r);

    void jcc(int cc, int64_t imm);
    void jcc8(int cc, int64_t imm);

    void setccb(int cc, Reg8 r);

    void setccb(int cc, const MemoryRef& mr);

    void cmovccq(int cc, Reg64 rsrc, Reg64 rdest);
    void cmovccl(int cc, Reg32 rsrc, Reg32 rdest);
    void cmovccw(int cc, Reg16 rsrc, Reg16 rdest);

    void cmovccq(int cc, const MemoryRef& mr, Reg64 r);
    void cmovccl(int cc, const MemoryRef& mr, Reg32 r);
    void cmovccw(int cc, const MemoryRef& mr, Reg16 r);
};


void X64Assembler::emitModrmSibAndDisp(int r, const MemoryRef& mr)
{
    int br = int(mr.base);
    int ir = int(mr.index);
    int s = mr.scale;
    int disp = mr.disp;
    bool ripRelative = mr.ripRelative;
    assert(ir != int(reg::rsp));
    // Determine the size of the displacement. If the disp == 0 and 'br' is
    // not rbp or r13, then we don't need to emit a displacement at all.
    int dispSize =
        (disp == 0 && br != int(reg::rbp) && br != int(reg::r13)) ? sz::nosize :
        signedValueFits(disp, sz::byte) ? sz::byte : sz::dword;
    // Set 'mod' based on the size of the displacement
    int mod = (dispSize == sz::nosize) ? 0 : (dispSize == sz::byte) ? 1 : 2;
    // Indicate RIP-relative mode if appropriate by setting mod = 0 and
    // br = rbp. RIP-relative mode always uses 4 bytes for the displacement.
    if (ripRelative) {
        assert(br == int(reg::noreg));
        assert(ir == int(reg::noreg));
        mod = 0;
        br = int(reg::rbp);
        dispSize = sz::dword;
    }
    // We need a SIB byte if any of the following conditions are true:
    //   1. The base register is rsp or r12.
    //   2. We're doing a baseless disp access that is not RIP-relative.
    //   3. We're using an index register.
    bool sibIsNeeded =
        br == int(reg::rsp) || br == int(reg::r12) ||
        br == int(reg::noreg) || ir != int(reg::noreg);
    // Handle special cases for 'br'
    if (br == int(reg::noreg)) {
        // If 'reg::noreg' was specified for 'br', we use the encoding for the
        // rbp register, and we must set mod=0 and "upgrade" to a DWORD-sized
        // displacement
        br = int(reg::rbp);
        dispSize = sz::dword;
        mod = 0;
    }
    // Emit modr/m
    emitModrm(mod, r, sibIsNeeded ? int(reg::rsp) : br);
    // Emit the SIB byte if needed
    if (sibIsNeeded) {
        // s:                               0  1  2   3  4   5   6   7  8
        static const int scaleLookup[] = { -1, 0, 1, -1, 2, -1, -1, -1, 3 };
        assert(s > 0 && s <= 8);
        int scale = scaleLookup[s];
        assert(scale != -1);
        // If 'reg::noreg' was specified for 'ir', we use
        // the encoding for the rsp register
        ir = (ir != int(reg::noreg)) ? ir : int(reg::rsp);
        byte((scale << 6) | ((ir & 7) << 3) | (br & 7));
    }
    // Emit displacement if needed
    if (dispSize == sz::dword) {
        dword(disp);
    }
    else if (dispSize == sz::byte) {
        byte(disp & 0xFF);
    }
}

void patchJcc(CodeAddress instrAddr, CodeAddress targetAddr)
{
    assert(instrAddr[0] == 0x0F && (instrAddr[1] & 0xF0) == 0x80);
    int64_t delta = targetAddr - (instrAddr + 6); // 2 for opcode, 4 for offset
    assert(signedValueFits(delta, sz::dword));
    *(int32_t*)(instrAddr + 2) = static_cast<int32_t>(delta);
}

void patchJcc8(CodeAddress instrAddr, CodeAddress targetAddr)
{
    assert((instrAddr[0] & 0xF0) == 0x70);
    int64_t delta = targetAddr - (instrAddr + 2); // 1 for opcode, 4 for offset
    assert(signedValueFits(delta, sz::byte));
    *(int8_t*)(instrAddr + 1) = static_cast<int8_t>(delta);
}

void patchJmp(CodeAddress instrAddr, CodeAddress targetAddr)
{
    assert(instrAddr[0] == 0xE9);
    int64_t delta = targetAddr - (instrAddr + 5); // 1 for opcode, 4 for offset
    assert(signedValueFits(delta, sz::dword));
    *(int32_t*)(instrAddr + 1) = static_cast<int32_t>(delta);
}

void patchJmp8(CodeAddress instrAddr, CodeAddress targetAddr)
{
    assert(instrAddr[0] == 0xEB);
    int64_t delta = targetAddr - (instrAddr + 2); // 1 for opcode, 1 for offset
    assert(signedValueFits(delta, sz::byte));
    *(int8_t*)(instrAddr + 1) = static_cast<int8_t>(delta);
}

void patchCall(CodeAddress instrAddr, CodeAddress targetAddr)
{
    assert(instrAddr[0] == 0xE8);
    int64_t delta = targetAddr - (instrAddr + 5); // 1 for opcode, 4 for offset
    assert(signedValueFits(delta, sz::dword));
    *(int32_t*)(instrAddr + 1) = static_cast<int32_t>(delta);
}

#define X64_INSTR_NO_ARGS(INSTR_NAME) \
  void X64Assembler::INSTR_NAME () \
  { \
    emitNoArgs(instr_ ## INSTR_NAME); \
  }

#define X64_INSTR_I(INSTR_NAME) \
  void X64Assembler::INSTR_NAME (int64_t imm) \
  { \
    emitI(instr_ ## INSTR_NAME, imm); \
  }

#define X64_INSTR_R_Q(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## q (Reg64 r) \
  { \
    emitR(instr_ ## INSTR_NAME, rn(r)); \
  }

#define X64_INSTR_R_L(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## l (Reg32 r) \
  { \
    emitR(instr_ ## INSTR_NAME, rn(r), sz::dword); \
  }

#define X64_INSTR_R_W(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## w (Reg16 r) \
  { \
    emitR(instr_ ## INSTR_NAME, rn(r), sz::word); \
  }

#define X64_INSTR_R_B(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## b (Reg8 r) \
  { \
    emitR(instr_ ## INSTR_NAME, rn(r), sz::byte); \
  }

#define X64_INSTR_M_Q(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## q (const MemoryRef& mr) \
  { \
    emitM(instr_ ## INSTR_NAME, mr); \
  }

#define X64_INSTR_M_L(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## l (const MemoryRef& mr) \
  { \
    emitM(instr_ ## INSTR_NAME, mr, sz::dword); \
  }

#define X64_INSTR_M_W(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## w (const MemoryRef& mr) \
  { \
    emitM(instr_ ## INSTR_NAME, mr, sz::word); \
  }

#define X64_INSTR_M_B(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## b (const MemoryRef& mr) \
  { \
    emitM(instr_ ## INSTR_NAME, mr, sz::byte); \
  }

#define X64_INSTR_RR_Q(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## q (Reg64 rsrc, Reg64 rdest) \
  { \
    emitRR(instr_ ## INSTR_NAME, rsrc, rdest); \
  }

#define X64_INSTR_RR_L(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## l (Reg32 rsrc, Reg32 rdest) \
  { \
    emitRR(instr_ ## INSTR_NAME, rsrc, rdest, sz::dword); \
  }

#define X64_INSTR_RR_W(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## w (Reg16 rsrc, Reg16 rdest) \
  { \
    emitRR(instr_ ## INSTR_NAME, rsrc, rdest, sz::word); \
  }

#define X64_INSTR_RR_B(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## b (Reg8 rsrc, Reg8 rdest) \
  { \
    emitRR(instr_ ## INSTR_NAME, rsrc, rdest, sz::byte); \
  }

#define X64_INSTR_RR_XMM(INSTR_NAME) \
  void X64Assembler::INSTR_NAME (RegXMM rsrc, RegXMM rdest) \
  { \
    emitRR(instr_ ## INSTR_NAME, rsrc, rdest); \
  }

#define X64_INSTR_IR_Q(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## q (int64_t imm, Reg64 r) \
  { \
    emitIR(instr_ ## INSTR_NAME, imm, r); \
  }

#define X64_INSTR_IR_L(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## l (int64_t imm, Reg32 r) \
  { \
    emitIR(instr_ ## INSTR_NAME, imm, r, sz::dword); \
  }

#define X64_INSTR_IR_W(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## w (int64_t imm, Reg16 r) \
  { \
    emitIR(instr_ ## INSTR_NAME, imm, r, sz::word); \
  }

#define X64_INSTR_IR_B(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## b (int64_t imm, Reg8 r) \
  { \
    emitIR(instr_ ## INSTR_NAME, imm, r, sz::byte); \
  }

#define X64_INSTR_IR_XMM(INSTR_NAME) \
  void X64Assembler::INSTR_NAME (int64_t imm, RegXMM r) \
  { \
    emitIR(instr_ ## INSTR_NAME, imm, r); \
  }

#define X64_INSTR_IM_Q(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## q (int64_t imm, const MemoryRef& mr) \
  { \
    emitIM(instr_ ## INSTR_NAME, imm, mr); \
  }

#define X64_INSTR_IM_L(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## l (int64_t imm, const MemoryRef& mr) \
  { \
    emitIM(instr_ ## INSTR_NAME, imm, mr, sz::dword); \
  }

#define X64_INSTR_IM_W(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## w (int64_t imm, const MemoryRef& mr) \
  { \
    emitIM(instr_ ## INSTR_NAME, imm, mr, sz::word); \
  }

#define X64_INSTR_IM_B(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## b (int64_t imm, const MemoryRef& mr) \
  { \
    emitIM(instr_ ## INSTR_NAME, imm, mr, sz::byte); \
  }

#define X64_INSTR_RM_Q(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## q (Reg64 r, const MemoryRef& mr) \
  { \
    emitRM(instr_ ## INSTR_NAME, r, mr); \
  }

#define X64_INSTR_RM_L(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## l (Reg32 r, const MemoryRef& mr) \
  { \
    emitRM(instr_ ## INSTR_NAME, r, mr, sz::dword); \
  }

#define X64_INSTR_RM_W(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## w (Reg16 r, const MemoryRef& mr) \
  { \
    emitRM(instr_ ## INSTR_NAME, r, mr, sz::word); \
  }

#define X64_INSTR_RM_B(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## b (Reg8 r, const MemoryRef& mr) \
  { \
    emitRM(instr_ ## INSTR_NAME, r, mr, sz::byte); \
  }

#define X64_INSTR_RM_XMM(INSTR_NAME) \
  void X64Assembler::INSTR_NAME (RegXMM r, const MemoryRef& mr) \
  { \
    emitRM(instr_ ## INSTR_NAME, r, mr); \
  }

#define X64_INSTR_MR_Q(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## q (const MemoryRef& mr, Reg64 r) \
  { \
    emitMR(instr_ ## INSTR_NAME, mr, r); \
  }

#define X64_INSTR_MR_L(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## l (const MemoryRef& mr, Reg32 r) \
  { \
    emitMR(instr_ ## INSTR_NAME, mr, r, sz::dword); \
  }

#define X64_INSTR_MR_W(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## w (const MemoryRef& mr, Reg16 r) \
  { \
    emitMR(instr_ ## INSTR_NAME, mr, r, sz::word); \
  }

#define X64_INSTR_MR_B(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## b (const MemoryRef& mr, Reg8 r) \
  { \
    emitMR(instr_ ## INSTR_NAME, mr, r, sz::byte); \
  }

#define X64_INSTR_MR_XMM(INSTR_NAME) \
  void X64Assembler::INSTR_NAME (const MemoryRef& mr, RegXMM r) \
  { \
    emitMR(instr_ ## INSTR_NAME, mr, r); \
  }

#define X64_INSTR_IRR_Q(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## q (int64_t imm, Reg64 rsrc, Reg64 rdest) \
  { \
    emitIRR(instr_ ## INSTR_NAME, imm, rsrc, rdest); \
  }

#define X64_INSTR_IRR_L(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## l (int64_t imm, Reg32 rsrc, Reg32 rdest) \
  { \
    emitIRR(instr_ ## INSTR_NAME, imm, rsrc, rdest, sz::dword); \
  }

#define X64_INSTR_IRR_W(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## w (int64_t imm, Reg16 rsrc, Reg16 rdest) \
  { \
    emitIRR(instr_ ## INSTR_NAME, imm, rsrc, rdest, sz::word); \
  }

#define X64_INSTR_IRR_B(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## b (int64_t imm, Reg8 rsrc, Reg8 rdest) \
  { \
    emitIRR(instr_ ## INSTR_NAME, imm, rsrc, rdest, sz::byte); \
  }

#define X64_INSTR_IRR_XMM(INSTR_NAME) \
  void X64Assembler::INSTR_NAME (int64_t imm, RegXMM rsrc, RegXMM rdest) \
  { \
    emitIRR(instr_ ## INSTR_NAME, imm, rsrc, rdest); \
  }

#define X64_INSTR_IRM_Q(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## q (int64_t imm, Reg64 r, const MemoryRef& mr) \
  { \
    emitIRM(instr_ ## INSTR_NAME, imm, r, mr); \
  }

#define X64_INSTR_IRM_L(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## l (int64_t imm, Reg32 r, const MemoryRef& mr) \
  { \
    emitIRM(instr_ ## INSTR_NAME, imm, r, mr, sz::dword); \
  }

#define X64_INSTR_IRM_W(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## w (int64_t imm, Reg16 r, const MemoryRef& mr) \
  { \
    emitIRM(instr_ ## INSTR_NAME, imm, r, mr, sz::word); \
  }

#define X64_INSTR_IRM_B(INSTR_NAME) \
  void X64Assembler::INSTR_NAME ## b (int64_t imm, Reg8 r, const MemoryRef& mr) \
  { \
    emitIRM(instr_ ## INSTR_NAME, imm, r, mr, sz::byte); \
  }

#define X64_INSTR_IMR_XMM(INSTR_NAME) \
  void X64Assembler::INSTR_NAME (int64_t imm, const MemoryRef& mr, RegXMM r) \
  { \
    emitIMR(instr_ ## INSTR_NAME, imm, mr, r); \
  }

X64_INSTRS()

#undef X64_INSTR_NO_ARGS
#undef X64_INSTR_I
#undef X64_INSTR_R_Q
#undef X64_INSTR_R_L
#undef X64_INSTR_R_W
#undef X64_INSTR_R_B
#undef X64_INSTR_M_Q
#undef X64_INSTR_M_L
#undef X64_INSTR_M_W
#undef X64_INSTR_M_B
#undef X64_INSTR_RR_Q
#undef X64_INSTR_RR_L
#undef X64_INSTR_RR_W
#undef X64_INSTR_RR_B
#undef X64_INSTR_RR_XMM
#undef X64_INSTR_IR_Q
#undef X64_INSTR_IR_L
#undef X64_INSTR_IR_W
#undef X64_INSTR_IR_B
#undef X64_INSTR_IR_XMM
#undef X64_INSTR_IM_Q
#undef X64_INSTR_IM_L
#undef X64_INSTR_IM_W
#undef X64_INSTR_IM_B
#undef X64_INSTR_RM_Q
#undef X64_INSTR_RM_L
#undef X64_INSTR_RM_W
#undef X64_INSTR_RM_B
#undef X64_INSTR_RM_XMM
#undef X64_INSTR_MR_Q
#undef X64_INSTR_MR_L
#undef X64_INSTR_MR_W
#undef X64_INSTR_MR_B
#undef X64_INSTR_MR_XMM
#undef X64_INSTR_IRR_Q
#undef X64_INSTR_IRR_L
#undef X64_INSTR_IRR_W
#undef X64_INSTR_IRR_B
#undef X64_INSTR_IRR_XMM
#undef X64_INSTR_IRM_Q
#undef X64_INSTR_IRM_L
#undef X64_INSTR_IRM_W
#undef X64_INSTR_IRM_B
#undef X64_INSTR_IMR_XMM

void X64Assembler::cdq()
{
    emitNoArgs(instr_cqo, sz::dword);
}

void X64Assembler::cwde()
{
    emitNoArgs(instr_cdqe, sz::dword);
}

void X64Assembler::cwd()
{
    emitNoArgs(instr_cqo, sz::word);
}

void X64Assembler::cbw()
{
    emitNoArgs(instr_cdqe, sz::word);
}

void X64Assembler::pushfw()
{
    emitNoArgs(instr_pushf, sz::word);
}

void X64Assembler::popfw()
{
    emitNoArgs(instr_popf, sz::word);
}

void X64Assembler::pushw(int64_t imm)
{
    emitI(instr_push, imm, sz::word);
}

void X64Assembler::cmpxchg16b(const MemoryRef& mr)
{
    emitM(instr_cmpxchg16b, mr);
}

void X64Assembler::cmpxchg8b(const MemoryRef& mr)
{
    emitM(instr_cmpxchg16b, mr, sz::dword);
}

void X64Assembler::imulq(Reg64 rsrc, Reg64 rdest)
{
    emitRR(instr_imul2, rsrc, rdest);
}

void X64Assembler::imull(Reg32 rsrc, Reg32 rdest)
{
    emitRR(instr_imul2, rsrc, rdest, sz::dword);
}

void X64Assembler::imulw(Reg16 rsrc, Reg16 rdest)
{
    emitRR(instr_imul2, rsrc, rdest, sz::word);
}

void X64Assembler::movzxwq(Reg16 rsrc, Reg64 rdest)
{
    emitRR(instr_movzxw, rsrc, rdest);
}

void X64Assembler::movzxwl(Reg16 rsrc, Reg32 rdest)
{
    emitRR(instr_movzxw, rsrc, rdest, sz::dword);
}

void X64Assembler::movzxbq(Reg8 rsrc, Reg64 rdest)
{
    emitRR(instr_movzxb, rsrc, rdest);
}

void X64Assembler::movzxbl(Reg8 rsrc, Reg32 rdest)
{
    emitRR(instr_movzxb, rsrc, rdest, sz::dword);
}

void X64Assembler::movzxbw(Reg8 rsrc, Reg16 rdest)
{
    emitRR(instr_movzxb, rsrc, rdest, sz::word);
}

void X64Assembler::movsxdq(Reg32 rsrc, Reg64 rdest)
{
    emitRR(instr_movsxdq, rsrc, rdest);
}

void X64Assembler::movsxwq(Reg16 rsrc, Reg64 rdest)
{
    emitRR(instr_movsxw, rsrc, rdest);
}

void X64Assembler::movsxwl(Reg16 rsrc, Reg32 rdest)
{
    emitRR(instr_movsxw, rsrc, rdest, sz::dword);
}

void X64Assembler::movsxbq(Reg8 rsrc, Reg64 rdest)
{
    emitRR(instr_movsxb, rsrc, rdest);
}

void X64Assembler::movsxbl(Reg8 rsrc, Reg32 rdest)
{
    emitRR(instr_movsxb, rsrc, rdest, sz::dword);
}

void X64Assembler::movsxbw(Reg8 rsrc, Reg16 rdest)
{
    emitRR(instr_movsxb, rsrc, rdest, sz::word);
}

void X64Assembler::movq(Reg64 rsrc, RegXMM rdest)
{
    emitRR(instr_gpr2xmm, rsrc, rdest);
}

void X64Assembler::movq(RegXMM rsrc, Reg64 rdest)
{
    emitRR(instr_xmm2gpr, rsrc, rdest);
}

void X64Assembler::movl(Reg32 rsrc, RegXMM rdest)
{
    emitRR(instr_gpr2xmm, rsrc, rdest, sz::dword);
}

void X64Assembler::movl(RegXMM rsrc, Reg32 rdest)
{
    emitRR(instr_xmm2gpr, rsrc, rdest, sz::dword);
}

void X64Assembler::cvtsi2sdq(Reg64 rsrc, RegXMM rdest)
{
    emitRR(instr_cvtsi2sd, rsrc, rdest);
}

void X64Assembler::cvtsi2sdl(Reg32 rsrc, RegXMM rdest)
{
    emitRR(instr_cvtsi2sd, rsrc, rdest, sz::dword);
}

void X64Assembler::cvttsd2siq(RegXMM rsrc, Reg64 rdest)
{
    emitRR(instr_cvttsd2si, rsrc, rdest);
}

void X64Assembler::cvttsd2sil(RegXMM rsrc, Reg32 rdest)
{
    emitRR(instr_cvttsd2si, rsrc, rdest, sz::dword);
}

void X64Assembler::movq(RegXMM r, const MemoryRef& mr)
{
    emitRM(instr_xmm2gpr, r, mr);
}

void X64Assembler::movl(RegXMM r, const MemoryRef& mr)
{
    emitRM(instr_xmm2gpr, r, mr, sz::dword);
}

void X64Assembler::imulq(const MemoryRef& mr, Reg64 r)
{
    emitMR(instr_imul2, mr, r);
}

void X64Assembler::imull(const MemoryRef& mr, Reg32 r)
{
    emitMR(instr_imul2, mr, r, sz::dword);
}

void X64Assembler::imulw(const MemoryRef& mr, Reg16 r)
{
    emitMR(instr_imul2, mr, r, sz::word);
}

void X64Assembler::movzxwq(const MemoryRef& mr, Reg64 r)
{
    emitMR(instr_movzxw, mr, r);
}

void X64Assembler::movzxwl(const MemoryRef& mr, Reg32 r)
{
    emitMR(instr_movzxw, mr, r, sz::dword);
}

void X64Assembler::movzxbq(const MemoryRef& mr, Reg64 r)
{
    emitMR(instr_movzxb, mr, r);
}

void X64Assembler::movzxbl(const MemoryRef& mr, Reg32 r)
{
    emitMR(instr_movzxb, mr, r, sz::dword);
}

void X64Assembler::movzxbw(const MemoryRef& mr, Reg16 r)
{
    emitMR(instr_movzxb, mr, r, sz::word);
}

void X64Assembler::movsxdq(const MemoryRef& mr, Reg64 r)
{
    emitMR(instr_movsxdq, mr, r);
}

void X64Assembler::movsxwq(const MemoryRef& mr, Reg64 r)
{
    emitMR(instr_movsxw, mr, r);
}

void X64Assembler::movsxwl(const MemoryRef& mr, Reg32 r)
{
    emitMR(instr_movsxw, mr, r, sz::dword);
}

void X64Assembler::movsxbq(const MemoryRef& mr, Reg64 r)
{
    emitMR(instr_movsxb, mr, r);
}

void X64Assembler::movsxbl(const MemoryRef& mr, Reg32 r)
{
    emitMR(instr_movsxb, mr, r, sz::dword);
}

void X64Assembler::movsxbw(const MemoryRef& mr, Reg16 r)
{
    emitMR(instr_movsxb, mr, r, sz::word);
}

void X64Assembler::movq(const MemoryRef& mr, RegXMM r)
{
    emitMR(instr_gpr2xmm, mr, r);
}

void X64Assembler::movl(const MemoryRef& mr, RegXMM r)
{
    emitMR(instr_gpr2xmm, mr, r, sz::dword);
}

void X64Assembler::cvtsi2sdq(const MemoryRef& mr, RegXMM r)
{
    emitMR(instr_cvtsi2sd, mr, r);
}

void X64Assembler::cvtsi2sdl(const MemoryRef& mr, RegXMM r)
{
    emitMR(instr_cvtsi2sd, mr, r, sz::dword);
}

void X64Assembler::cvttsd2siq(const MemoryRef& mr, Reg64 r)
{
    emitMR(instr_cvttsd2si, mr, r);
}

void X64Assembler::cvttsd2sil(const MemoryRef& mr, Reg32 r)
{
    emitMR(instr_cvttsd2si, mr, r, sz::dword);
}

void X64Assembler::imulq(int64_t imm, Reg64 rsrc, Reg64 rdest)
{
    emitIRR(instr_imul3, imm, rsrc, rdest);
}

void X64Assembler::imull(int64_t imm, Reg32 rsrc, Reg32 rdest)
{
    emitIRR(instr_imul3, imm, rsrc, rdest, sz::dword);
}

void X64Assembler::imulw(int64_t imm, Reg16 rsrc, Reg16 rdest)
{
    emitIRR(instr_imul3, imm, rsrc, rdest, sz::word);
}

void X64Assembler::imulq(int64_t imm, const MemoryRef& mr, Reg64 r)
{
    emitIMR(instr_imul3, imm, mr, r);
}

void X64Assembler::imull(int64_t imm, const MemoryRef& mr, Reg32 r)
{
    emitIMR(instr_imul3, imm, mr, r, sz::dword);
}

void X64Assembler::imulw(int64_t imm, const MemoryRef& mr, Reg16 r)
{
    emitIMR(instr_imul3, imm, mr, r, sz::word);
}

void X64Assembler::jcc(int jcond, int64_t imm)
{
    emitCI(instr_jcc, jcond, imm);
}

void X64Assembler::jcc8(int jcond, int64_t imm)
{
    emitCI(instr_jcc8, jcond, imm);
}

void X64Assembler::setccb(int jcond, Reg8 r)
{
    emitCR(instr_setcc, jcond, r);
}

void X64Assembler::setccb(int jcond, const MemoryRef& mr)
{
    emitCM(instr_setcc, jcond, mr);
}

void X64Assembler::cmovccq(int jcond, Reg64 rsrc, Reg64 rdest)
{
    emitCRR(instr_cmovcc, jcond, rsrc, rdest);
}

void X64Assembler::cmovccl(int jcond, Reg32 rsrc, Reg32 rdest)
{
    emitCRR(instr_cmovcc, jcond, rsrc, rdest, sz::dword);
}

void X64Assembler::cmovccw(int jcond, Reg16 rsrc, Reg16 rdest)
{
    emitCRR(instr_cmovcc, jcond, rsrc, rdest, sz::word);
}

void X64Assembler::cmovccq(int jcond, const MemoryRef& mr, Reg64 r)
{
    emitCMR(instr_cmovcc, jcond, mr, r);
}

void X64Assembler::cmovccl(int jcond, const MemoryRef& mr, Reg32 r)
{
    emitCMR(instr_cmovcc, jcond, mr, r, sz::dword);
}

void X64Assembler::cmovccw(int jcond, const MemoryRef& mr, Reg16 r)
{
    emitCMR(instr_cmovcc, jcond, mr, r, sz::word);
}
#endif // ASM_X64_H__
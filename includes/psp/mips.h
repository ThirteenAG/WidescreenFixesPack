#pragma once
#include <stdint.h>

typedef enum Register {
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

typedef enum SPRegister {
    fir = 0,
    fccr = 25,
    fexr = 26,
    fenr = 28,
    fcsr = 31,
    pc
} SPRegisterID;

typedef enum FPRegister {
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

uint32_t write_directly(uint32_t data);
uint32_t nop();
uint32_t bc0f(int32_t offset);
uint32_t di();
uint32_t div1(RegisterID rs, RegisterID rt);
uint32_t ei();
uint32_t eret();
uint32_t madd(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t maddu(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t mfhi1(RegisterID rd);
uint32_t mflo1(RegisterID rd);
uint32_t mfsa(RegisterID rd);
uint32_t mthi1(RegisterID rs);
uint32_t mtlo1(RegisterID rs);
uint32_t mtsa(RegisterID rs);
uint32_t mtsab(RegisterID rs, int32_t imm);
uint32_t mult(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t mult1(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t multu(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t multu1(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t _sync();
uint32_t syscall();
uint32_t tge(RegisterID rs, RegisterID rt);
uint32_t tlbwi();
uint32_t abss(FPRegisterID fd, FPRegisterID fs);
uint32_t adds(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft);
uint32_t addas(FPRegisterID fs, FPRegisterID ft);
uint32_t bc1f(int32_t offset);
uint32_t bc1fl(int32_t offset);
uint32_t bc1tl(int32_t offset);
uint32_t ceqs(FPRegisterID fs, FPRegisterID ft);
uint32_t cvtsw(FPRegisterID fd, FPRegisterID fs);
uint32_t cvtws(FPRegisterID fd, FPRegisterID fs);
uint32_t divs(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft);
uint32_t madds(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft);
uint32_t maddas(FPRegisterID fs, FPRegisterID ft);
uint32_t maxs(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft);
uint32_t mfc1(RegisterID rt, FPRegisterID fs);
uint32_t mins(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft);
uint32_t movs(FPRegisterID fd, FPRegisterID fs);
uint32_t msubs(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft);
uint32_t msubas(FPRegisterID fs, FPRegisterID ft);
uint32_t mtc1(RegisterID rt, FPRegisterID fs);
uint32_t muls(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft);
uint32_t mulas(FPRegisterID fs, FPRegisterID ft);
uint32_t negs(FPRegisterID fd, FPRegisterID fs);
uint32_t rsqrts(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft);
uint32_t sqrts(FPRegisterID fd, FPRegisterID ft);
uint32_t subs(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft);
uint32_t paddh(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t paddsw(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t paddw(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pand(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pcgth(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pcpyh(RegisterID rd, RegisterID rt);
uint32_t pcpyld(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pcpyud(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pexch(RegisterID rd, RegisterID rt);
uint32_t pexew(RegisterID rd, RegisterID rt);
uint32_t pextlb(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pextlh(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pextlw(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pextub(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pextuw(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pmaxh(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pmaxw(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pmfhi(RegisterID rd);
uint32_t pmflo(RegisterID rd);
uint32_t pminh(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pminw(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pmulth(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pnor(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t por(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t ppacb(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t ppach(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t prot3w(RegisterID rd, RegisterID rt);
uint32_t psllh(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t psllw(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t psrah(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t psraw(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t psrlh(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t psrlw(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t psubb(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t psubw(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t pxor(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t qfsrv(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t cles(FPRegisterID fs, FPRegisterID ft);
uint32_t clts(FPRegisterID fs, FPRegisterID ft);
uint32_t add(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t addu(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t _and(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t daddu(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t _div(RegisterID rs, RegisterID rt);
uint32_t divu(RegisterID rs, RegisterID rt);
uint32_t dsll(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t dsll32(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t dsllv(RegisterID rd, RegisterID rt, RegisterID rs);
uint32_t dsra(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t dsra32(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t dsrl(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t dsrl32(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t dsrlv(RegisterID rd, RegisterID rt, RegisterID rs);
uint32_t dsubu(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t jalr(RegisterID rs);
uint32_t jr(RegisterID rs);
uint32_t mfhi(RegisterID rd);
uint32_t mflo(RegisterID rd);
uint32_t movn(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t movz(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t mthi(RegisterID rs);
uint32_t mtlo(RegisterID rs);
uint32_t nor(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t _or(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t sll(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t sllv(RegisterID rd, RegisterID rt, RegisterID rs);
uint32_t slt(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t sltu(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t sra(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t srav(RegisterID rd, RegisterID rt, RegisterID rs);
uint32_t srl(RegisterID rd, RegisterID rt, int32_t shamt);
uint32_t srlv(RegisterID rd, RegisterID rt, RegisterID rs);
uint32_t sub(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t subu(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t _xor(RegisterID rd, RegisterID rs, RegisterID rt);
uint32_t addi(RegisterID rt, RegisterID rs, int32_t imm);
uint32_t addiu(RegisterID rt, RegisterID rs, int32_t imm);
uint32_t andi(RegisterID rt, RegisterID rs, int32_t imm);
uint32_t daddi(RegisterID rt, RegisterID rs, int32_t imm);
uint32_t daddiu(RegisterID rt, RegisterID rs, int32_t imm);
uint32_t lb(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t lbu(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t ld(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t ldl(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t ldr(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t lh(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t lhu(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t lui(RegisterID rt, int32_t imm);
uint32_t lw(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t lwl(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t lwr(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t lwu(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t ori(RegisterID rt, RegisterID rs, int32_t imm);
uint32_t sq(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t slti(RegisterID rt, RegisterID rs, int32_t imm);
uint32_t sltiu(RegisterID rt, RegisterID rs, int32_t imm);
uint32_t sb(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t sh(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t sw(RegisterID rt, RegisterID rs, int32_t offset);
uint32_t lwc1(FPRegisterID ft, RegisterID rs, int32_t offset);
uint32_t swc1(FPRegisterID ft, RegisterID rs, int32_t offset);
uint32_t xori(RegisterID rt, RegisterID rs, int32_t imm);
uint32_t li(RegisterID dest, int32_t imm);
uint32_t j(int32_t address);
uint32_t jal(int32_t address);
uint32_t b(int32_t imm);
uint32_t beq(RegisterID rs, RegisterID rt, int32_t imm);
uint32_t bne(RegisterID rs, RegisterID rt, int32_t imm);
uint32_t move(RegisterID rd, RegisterID rs);
uint32_t moveq(RegisterID rd, RegisterID rs);

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
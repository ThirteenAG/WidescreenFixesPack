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

#include <cstdio>
#include "asm-x64.h"

int main()
{
    char buffer[1 << 16] __attribute__((aligned(16)));
    CodeBlock cb;
    cb.init((Address)buffer, 1 << 16);
    X64Assembler a(cb);

    // -------------------------- none (qword) -----------------------------

    a.ret();
    a.nop();
    a.int3();
    a.cqo();
    a.cdqe();
    a.pushf();
    a.popf();
    a.xlat();
    a.cmc();
    a.clc();
    a.stc();

    // -------------------------- none (dword) -----------------------------

    a.cdq();
    a.cwde();

    // -------------------------- none (word) -----------------------------

    a.cwd();
    a.cbw();
    a.pushfw();
    a.popfw();

    // -------------------------- R (qword) -----------------------------

    a.incq(reg::rax);
    a.decq(reg::rax);
    a.notq(reg::rax);
    a.negq(reg::rax);
    a.mulq(reg::rax);
    a.divq(reg::rax);
    a.idivq(reg::rax);
    a.imulq(reg::rax);
    a.jmpq(reg::rax);
    a.callq(reg::rax);
    a.pushq(reg::rax);
    a.popq(reg::rax);
    a.rolq(reg::rax);
    a.rorq(reg::rax);
    a.rclq(reg::rax);
    a.rcrq(reg::rax);
    a.shlq(reg::rax);
    a.shrq(reg::rax);
    a.sarq(reg::rax);

    a.incq(reg::r8);
    a.decq(reg::r8);
    a.notq(reg::r8);
    a.negq(reg::r8);
    a.mulq(reg::r8);
    a.divq(reg::r8);
    a.idivq(reg::r8);
    a.imulq(reg::r8);
    a.jmpq(reg::r8);
    a.callq(reg::r8);
    a.pushq(reg::r8);
    a.popq(reg::r8);
    a.rolq(reg::r8);
    a.rorq(reg::r8);
    a.rclq(reg::r8);
    a.rcrq(reg::r8);
    a.shlq(reg::r8);
    a.shrq(reg::r8);
    a.sarq(reg::r8);

    // -------------------------- R (dword) -----------------------------

    a.incl(reg::eax);
    a.decl(reg::eax);
    a.notl(reg::eax);
    a.negl(reg::eax);
    a.mull(reg::eax);
    a.divl(reg::eax);
    a.idivl(reg::eax);
    a.imull(reg::eax);
    a.roll(reg::eax);
    a.rorl(reg::eax);
    a.rcll(reg::eax);
    a.rcrl(reg::eax);
    a.shll(reg::eax);
    a.shrl(reg::eax);
    a.sarl(reg::eax);

    a.incl(reg::r8d);
    a.decl(reg::r8d);
    a.notl(reg::r8d);
    a.negl(reg::r8d);
    a.mull(reg::r8d);
    a.divl(reg::r8d);
    a.idivl(reg::r8d);
    a.imull(reg::r8d);
    a.roll(reg::r8d);
    a.rorl(reg::r8d);
    a.rcll(reg::r8d);
    a.rcrl(reg::r8d);
    a.shll(reg::r8d);
    a.shrl(reg::r8d);
    a.sarl(reg::r8d);

    // -------------------------- R (word) -----------------------------

    a.incw(reg::ax);
    a.decw(reg::ax);
    a.notw(reg::ax);
    a.negw(reg::ax);
    a.mulw(reg::ax);
    a.divw(reg::ax);
    a.idivw(reg::ax);
    a.imulw(reg::ax);
    a.pushw(reg::ax);
    a.popw(reg::ax);
    a.rolw(reg::ax);
    a.rorw(reg::ax);
    a.rclw(reg::ax);
    a.rcrw(reg::ax);
    a.shlw(reg::ax);
    a.shrw(reg::ax);
    a.sarw(reg::ax);

    a.incw(reg::r8w);
    a.decw(reg::r8w);
    a.notw(reg::r8w);
    a.negw(reg::r8w);
    a.mulw(reg::r8w);
    a.divw(reg::r8w);
    a.idivw(reg::r8w);
    a.imulw(reg::r8w);
    a.pushw(reg::r8w);
    a.popw(reg::r8w);
    a.rolw(reg::r8w);
    a.rorw(reg::r8w);
    a.rclw(reg::r8w);
    a.rcrw(reg::r8w);
    a.shlw(reg::r8w);
    a.shrw(reg::r8w);
    a.sarw(reg::r8w);

    // -------------------------- R (byte) -----------------------------

    a.incb(reg::al);
    a.decb(reg::al);
    a.notb(reg::al);
    a.negb(reg::al);
    a.mulb(reg::al);
    a.divb(reg::al);
    a.idivb(reg::al);
    a.imulb(reg::al);
    a.rolb(reg::al);
    a.rorb(reg::al);
    a.rclb(reg::al);
    a.rcrb(reg::al);
    a.shlb(reg::al);
    a.shrb(reg::al);
    a.sarb(reg::al);

    a.incb(reg::r12b);
    a.decb(reg::r12b);
    a.notb(reg::r12b);
    a.negb(reg::r12b);
    a.mulb(reg::r12b);
    a.divb(reg::r12b);
    a.idivb(reg::r12b);
    a.imulb(reg::r12b);
    a.rolb(reg::r12b);
    a.rorb(reg::r12b);
    a.rclb(reg::r12b);
    a.rcrb(reg::r12b);
    a.shlb(reg::r12b);
    a.shrb(reg::r12b);
    a.sarb(reg::r12b);

    a.incb(reg::dil);
    a.decb(reg::dil);
    a.notb(reg::dil);
    a.negb(reg::dil);
    a.mulb(reg::dil);
    a.divb(reg::dil);
    a.idivb(reg::dil);
    a.imulb(reg::dil);
    a.rolb(reg::dil);
    a.rorb(reg::dil);
    a.rclb(reg::dil);
    a.rcrb(reg::dil);
    a.shlb(reg::dil);
    a.shrb(reg::dil);
    a.sarb(reg::dil);

    a.setccb(CC_E, reg::al);
    a.setccb(CC_E, reg::r12b);
    a.setccb(CC_E, reg::dil);

    // -------------------------- I (qword, 4 byte imm) -----------------------------

    a.jmp(0x10000);
    a.call(0x10000);
    a.push(0x10000);

    a.jcc(CC_E, 0x10000);

    // -------------------------- I (qword, 2 byte imm) -----------------------------

    a.ret(0x7fff);

    // -------------------------- I (qword, 1 byte imm) -----------------------------

    a.jmp(-123);
    a.jmp8(-123);

    a.push(-123);

    a.jcc(CC_E, -123);
    a.jcc8(CC_E, -123);

    // -------------------------- I (word, 2 byte imm) -----------------------------

    a.pushw(0x7fff);

    // -------------------------- M (qword) -----------------------------

    a.incq(reg::rax[-123]);
    a.decq(reg::rax[-123]);
    a.notq(reg::rax[-123]);
    a.negq(reg::rax[-123]);
    a.mulq(reg::rax[-123]);
    a.divq(reg::rax[-123]);
    a.idivq(reg::rax[-123]);
    a.imulq(reg::rax[-123]);

    a.jmpq(reg::rax[-123]);
    a.callq(reg::rax[-123]);
    a.pushq(reg::rax[-123]);
    a.popq(reg::rax[-123]);

    a.rolq(reg::rax[-123]);
    a.rorq(reg::rax[-123]);
    a.rclq(reg::rax[-123]);
    a.rcrq(reg::rax[-123]);
    a.shlq(reg::rax[-123]);
    a.shrq(reg::rax[-123]);
    a.sarq(reg::rax[-123]);

    a.incq(reg::r13[-123]);
    a.decq(reg::r13[-123]);
    a.notq(reg::r13[-123]);
    a.negq(reg::r13[-123]);
    a.mulq(reg::r13[-123]);
    a.divq(reg::r13[-123]);
    a.idivq(reg::r13[-123]);
    a.imulq(reg::r13[-123]);
    a.jmpq(reg::r13[-123]);
    a.callq(reg::r13[-123]);
    a.pushq(reg::r13[-123]);
    a.popq(reg::r13[-123]);
    a.rolq(reg::r13[-123]);
    a.rorq(reg::r13[-123]);
    a.rclq(reg::r13[-123]);
    a.rcrq(reg::r13[-123]);
    a.shlq(reg::r13[-123]);
    a.shrq(reg::r13[-123]);
    a.sarq(reg::r13[-123]);

    // -------------------------- M (dword) -----------------------------

    a.incl(reg::rax[-123]);
    a.decl(reg::rax[-123]);
    a.notl(reg::rax[-123]);
    a.negl(reg::rax[-123]);
    a.mull(reg::rax[-123]);
    a.divl(reg::rax[-123]);
    a.idivl(reg::rax[-123]);
    a.imull(reg::rax[-123]);
    a.roll(reg::rax[-123]);
    a.rorl(reg::rax[-123]);
    a.rcll(reg::rax[-123]);
    a.rcrl(reg::rax[-123]);
    a.shll(reg::rax[-123]);
    a.shrl(reg::rax[-123]);
    a.sarl(reg::rax[-123]);

    a.incl(reg::r13[-123]);
    a.decl(reg::r13[-123]);
    a.notl(reg::r13[-123]);
    a.negl(reg::r13[-123]);
    a.mull(reg::r13[-123]);
    a.divl(reg::r13[-123]);
    a.idivl(reg::r13[-123]);
    a.imull(reg::r13[-123]);
    a.roll(reg::r13[-123]);
    a.rorl(reg::r13[-123]);
    a.rcll(reg::r13[-123]);
    a.rcrl(reg::r13[-123]);
    a.shll(reg::r13[-123]);
    a.shrl(reg::r13[-123]);
    a.sarl(reg::r13[-123]);

    // -------------------------- M (word) -----------------------------

    a.incw(reg::rax[-123]);
    a.decw(reg::rax[-123]);
    a.notw(reg::rax[-123]);
    a.negw(reg::rax[-123]);
    a.mulw(reg::rax[-123]);
    a.divw(reg::rax[-123]);
    a.idivw(reg::rax[-123]);
    a.imulw(reg::rax[-123]);
    a.pushw(reg::rax[-123]);
    a.popw(reg::rax[-123]);
    a.rolw(reg::rax[-123]);
    a.rorw(reg::rax[-123]);
    a.rclw(reg::rax[-123]);
    a.rcrw(reg::rax[-123]);
    a.shlw(reg::rax[-123]);
    a.shrw(reg::rax[-123]);
    a.sarw(reg::rax[-123]);

    a.incw(reg::r13[-123]);
    a.decw(reg::r13[-123]);
    a.notw(reg::r13[-123]);
    a.negw(reg::r13[-123]);
    a.mulw(reg::r13[-123]);
    a.divw(reg::r13[-123]);
    a.idivw(reg::r13[-123]);
    a.imulw(reg::r13[-123]);
    a.pushw(reg::r13[-123]);
    a.popw(reg::r13[-123]);
    a.rolw(reg::r13[-123]);
    a.rorw(reg::r13[-123]);
    a.rclw(reg::r13[-123]);
    a.rcrw(reg::r13[-123]);
    a.shlw(reg::r13[-123]);
    a.shrw(reg::r13[-123]);
    a.sarw(reg::r13[-123]);

    // -------------------------- M (byte) -----------------------------

    a.incb(reg::rax[-123]);
    a.decb(reg::rax[-123]);
    a.notb(reg::rax[-123]);
    a.negb(reg::rax[-123]);
    a.mulb(reg::rax[-123]);
    a.divb(reg::rax[-123]);
    a.idivb(reg::rax[-123]);
    a.imulb(reg::rax[-123]);
    a.rolb(reg::rax[-123]);
    a.rorb(reg::rax[-123]);
    a.rclb(reg::rax[-123]);
    a.rcrb(reg::rax[-123]);
    a.shlb(reg::rax[-123]);
    a.shrb(reg::rax[-123]);
    a.sarb(reg::rax[-123]);

    a.setccb(CC_E, reg::rax[-123]);

    a.incb(reg::r13[-123]);
    a.decb(reg::r13[-123]);
    a.notb(reg::r13[-123]);
    a.negb(reg::r13[-123]);
    a.mulb(reg::r13[-123]);
    a.divb(reg::r13[-123]);
    a.idivb(reg::r13[-123]);
    a.imulb(reg::r13[-123]);
    a.rolb(reg::r13[-123]);
    a.rorb(reg::r13[-123]);
    a.rclb(reg::r13[-123]);
    a.rcrb(reg::r13[-123]);
    a.shlb(reg::r13[-123]);
    a.shrb(reg::r13[-123]);
    a.sarb(reg::r13[-123]);

    a.setccb(CC_E, reg::r13[-123]);

    // -------------------------- RR (qword) -----------------------------

    a.xchgq(reg::rcx, reg::rdi);
    a.addq(reg::rcx, reg::rdi);
    a.subq(reg::rcx, reg::rdi);
    a.andq(reg::rcx, reg::rdi);
    a.orq(reg::rcx, reg::rdi);
    a.xorq(reg::rcx, reg::rdi);
    a.cmpq(reg::rcx, reg::rdi);
    a.adcq(reg::rcx, reg::rdi);
    a.sbbq(reg::rcx, reg::rdi);
    a.movq(reg::rcx, reg::rdi);
    a.testq(reg::rcx, reg::rdi);
    a.imulq(reg::rcx, reg::rdi);
    a.xaddq(reg::rcx, reg::rdi);
    a.cmpxchgq(reg::rcx, reg::rdi);
    a.movzxwq(reg::cx, reg::rdi); // src is always word size
    a.movzxbq(reg::cl, reg::rdi); // src is always byte size
    a.movsxdq(reg::ecx, reg::rdi); // src is always dword size
    a.movsxwq(reg::cx, reg::rdi); // src is always word size
    a.movsxbq(reg::cl, reg::rdi); // src is always byte size
    a.shldq(reg::rax, reg::rdi);
    a.shrdq(reg::rax, reg::rdi);
    a.popcntq(reg::rax, reg::rdi);

    a.cmovccq(CC_E, reg::rcx, reg::rdi);

    a.movq(reg::rcx, reg::xmm7);
    a.movq(reg::xmm1, reg::rdi);
    a.movsd(reg::xmm1, reg::xmm7);
    a.addsd(reg::xmm1, reg::xmm7);
    a.subsd(reg::xmm1, reg::xmm7);
    a.mulsd(reg::xmm1, reg::xmm7);
    a.divsd(reg::xmm1, reg::xmm7);
    a.sqrtsd(reg::xmm1, reg::xmm7);
    a.ucomisd(reg::xmm1, reg::xmm7);
    a.pxor(reg::xmm1, reg::xmm7);
    a.psllq(reg::xmm1, reg::xmm7);
    a.psrlq(reg::xmm1, reg::xmm7);
    a.cvtsi2sdq(reg::rcx, reg::xmm7);
    a.cvttsd2siq(reg::xmm1, reg::rdi);
    a.movdqa(reg::xmm1, reg::xmm7);
    a.movdqu(reg::xmm1, reg::xmm7);
    a.movaps(reg::xmm1, reg::xmm7);
    a.movups(reg::xmm1, reg::xmm7);
    a.movapd(reg::xmm1, reg::xmm7);
    a.movupd(reg::xmm1, reg::xmm7);
    a.unpcklpd(reg::xmm1, reg::xmm7);

    a.xchgq(reg::rcx, reg::r12);
    a.addq(reg::rcx, reg::r12);
    a.subq(reg::rcx, reg::r12);
    a.andq(reg::rcx, reg::r12);
    a.orq(reg::rcx, reg::r12);
    a.xorq(reg::rcx, reg::r12);
    a.cmpq(reg::rcx, reg::r12);
    a.adcq(reg::rcx, reg::r12);
    a.sbbq(reg::rcx, reg::r12);
    a.movq(reg::rcx, reg::r12);
    a.testq(reg::rcx, reg::r12);
    a.imulq(reg::rcx, reg::r12);
    a.xaddq(reg::rcx, reg::r12);
    a.cmpxchgq(reg::rcx, reg::r12);
    a.movzxwq(reg::cx, reg::r12); // src is always word size
    a.movzxbq(reg::cl, reg::r12); // src is always byte size
    a.movsxdq(reg::ecx, reg::r12); // src is always dword size
    a.movsxwq(reg::cx, reg::r12); // src is always word size
    a.movsxbq(reg::cl, reg::r12); // src is always byte size
    a.shldq(reg::rax, reg::r12);
    a.shrdq(reg::rax, reg::r12);
    a.popcntq(reg::rax, reg::r12);

    a.cmovccq(CC_E, reg::rcx, reg::r12);

    a.movq(reg::rcx, reg::xmm12);
    a.movq(reg::xmm1, reg::r12);
    a.movsd(reg::xmm1, reg::xmm12);
    a.addsd(reg::xmm1, reg::xmm12);
    a.subsd(reg::xmm1, reg::xmm12);
    a.mulsd(reg::xmm1, reg::xmm12);
    a.divsd(reg::xmm1, reg::xmm12);
    a.sqrtsd(reg::xmm1, reg::xmm12);
    a.ucomisd(reg::xmm1, reg::xmm12);
    a.pxor(reg::xmm1, reg::xmm12);
    a.psllq(reg::xmm1, reg::xmm12);
    a.psrlq(reg::xmm1, reg::xmm12);
    a.cvtsi2sdq(reg::rcx, reg::xmm12);
    a.cvttsd2siq(reg::xmm1, reg::r12);
    a.movdqa(reg::xmm1, reg::xmm12);
    a.movdqu(reg::xmm1, reg::xmm12);
    a.movaps(reg::xmm1, reg::xmm12);
    a.movups(reg::xmm1, reg::xmm12);
    a.movapd(reg::xmm1, reg::xmm12);
    a.movupd(reg::xmm1, reg::xmm12);
    a.unpcklpd(reg::xmm1, reg::xmm12);

    a.xchgq(reg::r8, reg::r12);
    a.addq(reg::r8, reg::r12);
    a.subq(reg::r8, reg::r12);
    a.andq(reg::r8, reg::r12);
    a.orq(reg::r8, reg::r12);
    a.xorq(reg::r8, reg::r12);
    a.cmpq(reg::r8, reg::r12);
    a.adcq(reg::r8, reg::r12);
    a.sbbq(reg::r8, reg::r12);
    a.movq(reg::r8, reg::r12);
    a.testq(reg::r8, reg::r12);
    a.imulq(reg::r8, reg::r12);
    a.xaddq(reg::r8, reg::r12);
    a.cmpxchgq(reg::r8, reg::r12);
    a.movzxwq(reg::r8w, reg::r12); // src is always word size
    a.movzxbq(reg::r8b, reg::r12); // src is always byte size
    a.movsxdq(reg::r8d, reg::r12); // src is always dword size
    a.movsxwq(reg::r8w, reg::r12); // src is always word size
    a.movsxbq(reg::r8b, reg::r12); // src is always byte size
    a.shldq(reg::r8, reg::r12);
    a.shrdq(reg::r8, reg::r12);
    a.popcntq(reg::r8, reg::r12);

    a.cmovccq(CC_E, reg::r8, reg::r12);

    a.movq(reg::r8, reg::xmm12);
    a.movq(reg::xmm8, reg::r12);
    a.movsd(reg::xmm8, reg::xmm12);
    a.addsd(reg::xmm8, reg::xmm12);
    a.subsd(reg::xmm8, reg::xmm12);
    a.mulsd(reg::xmm8, reg::xmm12);
    a.divsd(reg::xmm8, reg::xmm12);
    a.sqrtsd(reg::xmm8, reg::xmm12);
    a.ucomisd(reg::xmm8, reg::xmm12);
    a.pxor(reg::xmm8, reg::xmm12);
    a.psllq(reg::xmm8, reg::xmm12);
    a.psrlq(reg::xmm8, reg::xmm12);
    a.cvtsi2sdq(reg::r8, reg::xmm12);
    a.cvttsd2siq(reg::xmm8, reg::r12);
    a.movdqa(reg::xmm8, reg::xmm12);
    a.movdqu(reg::xmm8, reg::xmm12);
    a.movaps(reg::xmm8, reg::xmm12);
    a.movups(reg::xmm8, reg::xmm12);
    a.movapd(reg::xmm8, reg::xmm12);
    a.movupd(reg::xmm8, reg::xmm12);
    a.unpcklpd(reg::xmm8, reg::xmm12);

    // -------------------------- RR (dword) -----------------------------

    a.xchgl(reg::ecx, reg::edi);
    a.addl(reg::ecx, reg::edi);
    a.subl(reg::ecx, reg::edi);
    a.andl(reg::ecx, reg::edi);
    a.orl(reg::ecx, reg::edi);
    a.xorl(reg::ecx, reg::edi);
    a.cmpl(reg::ecx, reg::edi);
    a.adcl(reg::ecx, reg::edi);
    a.sbbl(reg::ecx, reg::edi);
    a.movl(reg::ecx, reg::edi);
    a.testl(reg::ecx, reg::edi);
    a.imull(reg::ecx, reg::edi);
    a.xaddl(reg::ecx, reg::edi);
    a.cmpxchgl(reg::ecx, reg::edi);
    a.movzxwl(reg::cx, reg::edi); // src is always word size
    a.movzxbl(reg::cl, reg::edi); // src is always byte size
    a.movsxwl(reg::cx, reg::edi); // src is always word size
    a.movsxbl(reg::cl, reg::edi); // src is always byte size
    a.shldl(reg::eax, reg::edi);
    a.shrdl(reg::eax, reg::edi);
    a.popcntl(reg::eax, reg::edi);

    a.cmovccl(CC_E, reg::ecx, reg::edi);

    a.movl(reg::ecx, reg::xmm7);
    a.movl(reg::xmm1, reg::edi);
    a.cvtsi2sdl(reg::ecx, reg::xmm7);
    a.cvttsd2sil(reg::xmm1, reg::edi);

    a.xchgl(reg::ecx, reg::r12d);
    a.addl(reg::ecx, reg::r12d);
    a.subl(reg::ecx, reg::r12d);
    a.andl(reg::ecx, reg::r12d);
    a.orl(reg::ecx, reg::r12d);
    a.xorl(reg::ecx, reg::r12d);
    a.cmpl(reg::ecx, reg::r12d);
    a.adcl(reg::ecx, reg::r12d);
    a.sbbl(reg::ecx, reg::r12d);
    a.movl(reg::ecx, reg::r12d);
    a.testl(reg::ecx, reg::r12d);
    a.imull(reg::ecx, reg::r12d);
    a.xaddl(reg::ecx, reg::r12d);
    a.cmpxchgl(reg::ecx, reg::r12d);
    a.movzxwl(reg::cx, reg::r12d); // src is always word size
    a.movzxbl(reg::cl, reg::r12d); // src is always byte size
    a.movsxwl(reg::cx, reg::r12d); // src is always word size
    a.movsxbl(reg::cl, reg::r12d); // src is always byte size
    a.shldl(reg::eax, reg::r12d);
    a.shrdl(reg::eax, reg::r12d);
    a.popcntl(reg::eax, reg::r12d);

    a.cmovccl(CC_E, reg::ecx, reg::r12d);

    a.movl(reg::ecx, reg::xmm12);
    a.movl(reg::xmm1, reg::r12d);
    a.cvtsi2sdl(reg::ecx, reg::xmm12);
    a.cvttsd2sil(reg::xmm1, reg::r12d);

    a.xchgl(reg::r8d, reg::r12d);
    a.addl(reg::r8d, reg::r12d);
    a.subl(reg::r8d, reg::r12d);
    a.andl(reg::r8d, reg::r12d);
    a.orl(reg::r8d, reg::r12d);
    a.xorl(reg::r8d, reg::r12d);
    a.cmpl(reg::r8d, reg::r12d);
    a.adcl(reg::r8d, reg::r12d);
    a.sbbl(reg::r8d, reg::r12d);
    a.movl(reg::r8d, reg::r12d);
    a.testl(reg::r8d, reg::r12d);
    a.imull(reg::r8d, reg::r12d);
    a.xaddl(reg::r8d, reg::r12d);
    a.cmpxchgl(reg::r8d, reg::r12d);
    a.movzxwl(reg::r8w, reg::r12d); // src is always word size
    a.movzxbl(reg::r8b, reg::r12d); // src is always byte size
    a.movsxwl(reg::r8w, reg::r12d); // src is always word size
    a.movsxbl(reg::r8b, reg::r12d); // src is always byte size
    a.shldl(reg::r8d, reg::r12d);
    a.shrdl(reg::r8d, reg::r12d);
    a.popcntl(reg::r8d, reg::r12d);

    a.cmovccl(CC_E, reg::r8d, reg::r12d);

    a.movl(reg::r8d, reg::xmm12);
    a.movl(reg::xmm8, reg::r12d);
    a.cvtsi2sdl(reg::r8d, reg::xmm12);
    a.cvttsd2sil(reg::xmm8, reg::r12d);

    // -------------------------- RR (word) -----------------------------

    a.xchgw(reg::cx, reg::di);
    a.addw(reg::cx, reg::di);
    a.subw(reg::cx, reg::di);
    a.andw(reg::cx, reg::di);
    a.orw(reg::cx, reg::di);
    a.xorw(reg::cx, reg::di);
    a.cmpw(reg::cx, reg::di);
    a.adcw(reg::cx, reg::di);
    a.sbbw(reg::cx, reg::di);
    a.movw(reg::cx, reg::di);
    a.testw(reg::cx, reg::di);
    a.imulw(reg::cx, reg::di);
    a.xaddw(reg::cx, reg::di);
    a.cmpxchgw(reg::cx, reg::di);
    a.movzxbw(reg::cl, reg::di); // src is always byte size
    a.movsxbw(reg::cl, reg::di); // src is always byte size
    a.shldw(reg::ax, reg::di);
    a.shrdw(reg::ax, reg::di);
    a.popcntw(reg::ax, reg::di);

    a.cmovccw(CC_E, reg::cx, reg::di);

    a.xchgw(reg::cx, reg::r12w);
    a.addw(reg::cx, reg::r12w);
    a.subw(reg::cx, reg::r12w);
    a.andw(reg::cx, reg::r12w);
    a.orw(reg::cx, reg::r12w);
    a.xorw(reg::cx, reg::r12w);
    a.cmpw(reg::cx, reg::r12w);
    a.adcw(reg::cx, reg::r12w);
    a.sbbw(reg::cx, reg::r12w);
    a.movw(reg::cx, reg::r12w);
    a.testw(reg::cx, reg::r12w);
    a.imulw(reg::cx, reg::r12w);
    a.xaddw(reg::cx, reg::r12w);
    a.cmpxchgw(reg::cx, reg::r12w);
    a.movzxbw(reg::cl, reg::r12w); // src is always byte size
    a.movsxbw(reg::cl, reg::r12w); // src is always byte size
    a.shldw(reg::ax, reg::r12w);
    a.shrdw(reg::ax, reg::r12w);
    a.popcntw(reg::ax, reg::r12w);

    a.cmovccw(CC_E, reg::cx, reg::r12w);

    a.xchgw(reg::r8w, reg::r12w);
    a.addw(reg::r8w, reg::r12w);
    a.subw(reg::r8w, reg::r12w);
    a.andw(reg::r8w, reg::r12w);
    a.orw(reg::r8w, reg::r12w);
    a.xorw(reg::r8w, reg::r12w);
    a.cmpw(reg::r8w, reg::r12w);
    a.adcw(reg::r8w, reg::r12w);
    a.sbbw(reg::r8w, reg::r12w);
    a.movw(reg::r8w, reg::r12w);
    a.testw(reg::r8w, reg::r12w);
    a.imulw(reg::r8w, reg::r12w);
    a.xaddw(reg::r8w, reg::r12w);
    a.cmpxchgw(reg::r8w, reg::r12w);
    a.movzxbw(reg::r8b, reg::r12w); // src is always byte size
    a.movsxbw(reg::r8b, reg::r12w); // src is always byte size
    a.shldw(reg::r8w, reg::r12w);
    a.shrdw(reg::r8w, reg::r12w);
    a.popcntw(reg::r8w, reg::r12w);

    a.cmovccw(CC_E, reg::r8w, reg::r12w);


    // -------------------------- RR (byte) -----------------------------

    a.xchgb(reg::cl, reg::dil);
    a.addb(reg::cl, reg::dil);
    a.subb(reg::cl, reg::dil);
    a.andb(reg::cl, reg::dil);
    a.orb(reg::cl, reg::dil);
    a.xorb(reg::cl, reg::dil);
    a.cmpb(reg::cl, reg::dil);
    a.adcb(reg::cl, reg::dil);
    a.sbbb(reg::cl, reg::dil);
    a.movb(reg::cl, reg::dil);
    a.testb(reg::cl, reg::dil);
    a.xaddb(reg::cl, reg::dil);
    a.cmpxchgb(reg::cl, reg::dil);

    a.xchgb(reg::cl, reg::r12b);
    a.addb(reg::cl, reg::r12b);
    a.subb(reg::cl, reg::r12b);
    a.andb(reg::cl, reg::r12b);
    a.orb(reg::cl, reg::r12b);
    a.xorb(reg::cl, reg::r12b);
    a.cmpb(reg::cl, reg::r12b);
    a.adcb(reg::cl, reg::r12b);
    a.sbbb(reg::cl, reg::r12b);
    a.movb(reg::cl, reg::r12b);
    a.testb(reg::cl, reg::r12b);
    a.xaddb(reg::cl, reg::r12b);
    a.cmpxchgb(reg::cl, reg::r12b);

    a.xchgb(reg::r8b, reg::r12b);
    a.addb(reg::r8b, reg::r12b);
    a.subb(reg::r8b, reg::r12b);
    a.andb(reg::r8b, reg::r12b);
    a.orb(reg::r8b, reg::r12b);
    a.xorb(reg::r8b, reg::r12b);
    a.cmpb(reg::r8b, reg::r12b);
    a.adcb(reg::r8b, reg::r12b);
    a.sbbb(reg::r8b, reg::r12b);
    a.movb(reg::r8b, reg::r12b);
    a.testb(reg::r8b, reg::r12b);
    a.xaddb(reg::r8b, reg::r12b);
    a.cmpxchgb(reg::r8b, reg::r12b);

    // -------------------------- RM (qword) -----------------------------

    a.xchgq(reg::rcx, reg::rax[-123]);
    a.addq(reg::rcx, reg::rax[-123]);
    a.subq(reg::rcx, reg::rax[-123]);
    a.andq(reg::rcx, reg::rax[-123]);
    a.orq(reg::rcx, reg::rax[-123]);
    a.xorq(reg::rcx, reg::rax[-123]);
    a.cmpq(reg::rcx, reg::rax[-123]);
    a.adcq(reg::rcx, reg::rax[-123]);
    a.sbbq(reg::rcx, reg::rax[-123]);
    a.movq(reg::rcx, reg::rax[-123]);
    a.testq(reg::rcx, reg::rax[-123]);
    a.xaddq(reg::rcx, reg::rax[-123]);
    a.cmpxchgq(reg::rcx, reg::rax[-123]);
    a.shldq(reg::rax, reg::rax[-123]);
    a.shrdq(reg::rax, reg::rax[-123]);
    a.movq(reg::xmm1, reg::rax[-123]);
    a.movsd(reg::xmm1, reg::rax[-123]);
    a.movdqa(reg::xmm1, reg::rax[-123]);
    a.movdqu(reg::xmm1, reg::rax[-123]);
    a.movaps(reg::xmm1, reg::rax[-123]);
    a.movups(reg::xmm1, reg::rax[-123]);
    a.movapd(reg::xmm1, reg::rax[-123]);
    a.movupd(reg::xmm1, reg::rax[-123]);

    a.xchgq(reg::r8, reg::rax[-123]);
    a.addq(reg::r8, reg::rax[-123]);
    a.subq(reg::r8, reg::rax[-123]);
    a.andq(reg::r8, reg::rax[-123]);
    a.orq(reg::r8, reg::rax[-123]);
    a.xorq(reg::r8, reg::rax[-123]);
    a.cmpq(reg::r8, reg::rax[-123]);
    a.adcq(reg::r8, reg::rax[-123]);
    a.sbbq(reg::r8, reg::rax[-123]);
    a.movq(reg::r8, reg::rax[-123]);
    a.testq(reg::r8, reg::rax[-123]);
    a.xaddq(reg::r8, reg::rax[-123]);
    a.cmpxchgq(reg::r8, reg::rax[-123]);
    a.shldq(reg::r8, reg::rax[-123]);
    a.shrdq(reg::r8, reg::rax[-123]);
    a.movq(reg::xmm8, reg::rax[-123]);
    a.movsd(reg::xmm8, reg::rax[-123]);
    a.movdqa(reg::xmm8, reg::rax[-123]);
    a.movdqu(reg::xmm8, reg::rax[-123]);
    a.movaps(reg::xmm8, reg::rax[-123]);
    a.movups(reg::xmm8, reg::rax[-123]);
    a.movapd(reg::xmm8, reg::rax[-123]);
    a.movupd(reg::xmm8, reg::rax[-123]);

    a.xchgq(reg::rcx, reg::r13[-123]);
    a.addq(reg::rcx, reg::r13[-123]);
    a.subq(reg::rcx, reg::r13[-123]);
    a.andq(reg::rcx, reg::r13[-123]);
    a.orq(reg::rcx, reg::r13[-123]);
    a.xorq(reg::rcx, reg::r13[-123]);
    a.cmpq(reg::rcx, reg::r13[-123]);
    a.adcq(reg::rcx, reg::r13[-123]);
    a.sbbq(reg::rcx, reg::r13[-123]);
    a.movq(reg::rcx, reg::r13[-123]);
    a.testq(reg::rcx, reg::r13[-123]);
    a.xaddq(reg::rcx, reg::r13[-123]);
    a.cmpxchgq(reg::rcx, reg::r13[-123]);
    a.shldq(reg::rax, reg::r13[-123]);
    a.shrdq(reg::rax, reg::r13[-123]);
    a.movq(reg::xmm1, reg::r13[-123]);
    a.movsd(reg::xmm1, reg::r13[-123]);
    a.movdqa(reg::xmm1, reg::r13[-123]);
    a.movdqu(reg::xmm1, reg::r13[-123]);
    a.movaps(reg::xmm1, reg::r13[-123]);
    a.movups(reg::xmm1, reg::r13[-123]);
    a.movapd(reg::xmm1, reg::r13[-123]);
    a.movupd(reg::xmm1, reg::r13[-123]);

    a.xchgq(reg::r8, reg::r13[-123]);
    a.addq(reg::r8, reg::r13[-123]);
    a.subq(reg::r8, reg::r13[-123]);
    a.andq(reg::r8, reg::r13[-123]);
    a.orq(reg::r8, reg::r13[-123]);
    a.xorq(reg::r8, reg::r13[-123]);
    a.cmpq(reg::r8, reg::r13[-123]);
    a.adcq(reg::r8, reg::r13[-123]);
    a.sbbq(reg::r8, reg::r13[-123]);
    a.movq(reg::r8, reg::r13[-123]);
    a.testq(reg::r8, reg::r13[-123]);
    a.xaddq(reg::r8, reg::r13[-123]);
    a.cmpxchgq(reg::r8, reg::r13[-123]);
    a.shldq(reg::r8, reg::r13[-123]);
    a.shrdq(reg::r8, reg::r13[-123]);
    a.movq(reg::xmm8, reg::r13[-123]);
    a.movsd(reg::xmm8, reg::r13[-123]);
    a.movdqa(reg::xmm8, reg::r13[-123]);
    a.movdqu(reg::xmm8, reg::r13[-123]);
    a.movaps(reg::xmm8, reg::r13[-123]);
    a.movups(reg::xmm8, reg::r13[-123]);
    a.movapd(reg::xmm8, reg::r13[-123]);
    a.movupd(reg::xmm8, reg::r13[-123]);

    // -------------------------- RM (dword) -----------------------------

    a.xchgl(reg::ecx, reg::rax[-123]);
    a.addl(reg::ecx, reg::rax[-123]);
    a.subl(reg::ecx, reg::rax[-123]);
    a.andl(reg::ecx, reg::rax[-123]);
    a.orl(reg::ecx, reg::rax[-123]);
    a.xorl(reg::ecx, reg::rax[-123]);
    a.cmpl(reg::ecx, reg::rax[-123]);
    a.adcl(reg::ecx, reg::rax[-123]);
    a.sbbl(reg::ecx, reg::rax[-123]);
    a.movl(reg::ecx, reg::rax[-123]);
    a.testl(reg::ecx, reg::rax[-123]);
    a.xaddl(reg::ecx, reg::rax[-123]);
    a.cmpxchgl(reg::ecx, reg::rax[-123]);
    a.shldl(reg::eax, reg::rax[-123]);
    a.shrdl(reg::eax, reg::rax[-123]);
    a.movl(reg::xmm1, reg::rax[-123]);

    a.xchgl(reg::r8d, reg::rax[-123]);
    a.addl(reg::r8d, reg::rax[-123]);
    a.subl(reg::r8d, reg::rax[-123]);
    a.andl(reg::r8d, reg::rax[-123]);
    a.orl(reg::r8d, reg::rax[-123]);
    a.xorl(reg::r8d, reg::rax[-123]);
    a.cmpl(reg::r8d, reg::rax[-123]);
    a.adcl(reg::r8d, reg::rax[-123]);
    a.sbbl(reg::r8d, reg::rax[-123]);
    a.movl(reg::r8d, reg::rax[-123]);
    a.testl(reg::r8d, reg::rax[-123]);
    a.xaddl(reg::r8d, reg::rax[-123]);
    a.cmpxchgl(reg::r8d, reg::rax[-123]);
    a.shldl(reg::r8d, reg::rax[-123]);
    a.shrdl(reg::r8d, reg::rax[-123]);
    a.movl(reg::xmm8, reg::rax[-123]);

    a.xchgl(reg::ecx, reg::r13[-123]);
    a.addl(reg::ecx, reg::r13[-123]);
    a.subl(reg::ecx, reg::r13[-123]);
    a.andl(reg::ecx, reg::r13[-123]);
    a.orl(reg::ecx, reg::r13[-123]);
    a.xorl(reg::ecx, reg::r13[-123]);
    a.cmpl(reg::ecx, reg::r13[-123]);
    a.adcl(reg::ecx, reg::r13[-123]);
    a.sbbl(reg::ecx, reg::r13[-123]);
    a.movl(reg::ecx, reg::r13[-123]);
    a.testl(reg::ecx, reg::r13[-123]);
    a.xaddl(reg::ecx, reg::r13[-123]);
    a.cmpxchgl(reg::ecx, reg::r13[-123]);
    a.shldl(reg::eax, reg::r13[-123]);
    a.shrdl(reg::eax, reg::r13[-123]);
    a.movl(reg::xmm1, reg::r13[-123]);

    a.xchgl(reg::r8d, reg::r13[-123]);
    a.addl(reg::r8d, reg::r13[-123]);
    a.subl(reg::r8d, reg::r13[-123]);
    a.andl(reg::r8d, reg::r13[-123]);
    a.orl(reg::r8d, reg::r13[-123]);
    a.xorl(reg::r8d, reg::r13[-123]);
    a.cmpl(reg::r8d, reg::r13[-123]);
    a.adcl(reg::r8d, reg::r13[-123]);
    a.sbbl(reg::r8d, reg::r13[-123]);
    a.movl(reg::r8d, reg::r13[-123]);
    a.testl(reg::r8d, reg::r13[-123]);
    a.xaddl(reg::r8d, reg::r13[-123]);
    a.cmpxchgl(reg::r8d, reg::r13[-123]);
    a.shldl(reg::r8d, reg::r13[-123]);
    a.shrdl(reg::r8d, reg::r13[-123]);
    a.movl(reg::xmm8, reg::r13[-123]);

    // -------------------------- RM (word) -----------------------------

    a.xchgw(reg::cx, reg::rax[-123]);
    a.addw(reg::cx, reg::rax[-123]);
    a.subw(reg::cx, reg::rax[-123]);
    a.andw(reg::cx, reg::rax[-123]);
    a.orw(reg::cx, reg::rax[-123]);
    a.xorw(reg::cx, reg::rax[-123]);
    a.cmpw(reg::cx, reg::rax[-123]);
    a.adcw(reg::cx, reg::rax[-123]);
    a.sbbw(reg::cx, reg::rax[-123]);
    a.movw(reg::cx, reg::rax[-123]);
    a.testw(reg::cx, reg::rax[-123]);
    a.xaddw(reg::cx, reg::rax[-123]);
    a.cmpxchgw(reg::cx, reg::rax[-123]);
    a.shldw(reg::ax, reg::rax[-123]);
    a.shrdw(reg::ax, reg::rax[-123]);

    a.xchgw(reg::r8w, reg::rax[-123]);
    a.addw(reg::r8w, reg::rax[-123]);
    a.subw(reg::r8w, reg::rax[-123]);
    a.andw(reg::r8w, reg::rax[-123]);
    a.orw(reg::r8w, reg::rax[-123]);
    a.xorw(reg::r8w, reg::rax[-123]);
    a.cmpw(reg::r8w, reg::rax[-123]);
    a.adcw(reg::r8w, reg::rax[-123]);
    a.sbbw(reg::r8w, reg::rax[-123]);
    a.movw(reg::r8w, reg::rax[-123]);
    a.testw(reg::r8w, reg::rax[-123]);
    a.xaddw(reg::r8w, reg::rax[-123]);
    a.cmpxchgw(reg::r8w, reg::rax[-123]);
    a.shldw(reg::r8w, reg::rax[-123]);
    a.shrdw(reg::r8w, reg::rax[-123]);

    a.xchgw(reg::cx, reg::r13[-123]);
    a.addw(reg::cx, reg::r13[-123]);
    a.subw(reg::cx, reg::r13[-123]);
    a.andw(reg::cx, reg::r13[-123]);
    a.orw(reg::cx, reg::r13[-123]);
    a.xorw(reg::cx, reg::r13[-123]);
    a.cmpw(reg::cx, reg::r13[-123]);
    a.adcw(reg::cx, reg::r13[-123]);
    a.sbbw(reg::cx, reg::r13[-123]);
    a.movw(reg::cx, reg::r13[-123]);
    a.testw(reg::cx, reg::r13[-123]);
    a.xaddw(reg::cx, reg::r13[-123]);
    a.cmpxchgw(reg::cx, reg::r13[-123]);
    a.shldw(reg::ax, reg::r13[-123]);
    a.shrdw(reg::ax, reg::r13[-123]);

    a.xchgw(reg::r8w, reg::r13[-123]);
    a.addw(reg::r8w, reg::r13[-123]);
    a.subw(reg::r8w, reg::r13[-123]);
    a.andw(reg::r8w, reg::r13[-123]);
    a.orw(reg::r8w, reg::r13[-123]);
    a.xorw(reg::r8w, reg::r13[-123]);
    a.cmpw(reg::r8w, reg::r13[-123]);
    a.adcw(reg::r8w, reg::r13[-123]);
    a.sbbw(reg::r8w, reg::r13[-123]);
    a.movw(reg::r8w, reg::r13[-123]);
    a.testw(reg::r8w, reg::r13[-123]);
    a.xaddw(reg::r8w, reg::r13[-123]);
    a.cmpxchgw(reg::r8w, reg::r13[-123]);
    a.shldw(reg::r8w, reg::r13[-123]);
    a.shrdw(reg::r8w, reg::r13[-123]);

    // -------------------------- RM (byte) -----------------------------

    a.xchgb(reg::cl, reg::rax[-123]);
    a.addb(reg::cl, reg::rax[-123]);
    a.subb(reg::cl, reg::rax[-123]);
    a.andb(reg::cl, reg::rax[-123]);
    a.orb(reg::cl, reg::rax[-123]);
    a.xorb(reg::cl, reg::rax[-123]);
    a.cmpb(reg::cl, reg::rax[-123]);
    a.adcb(reg::cl, reg::rax[-123]);
    a.sbbb(reg::cl, reg::rax[-123]);
    a.movb(reg::cl, reg::rax[-123]);
    a.testb(reg::cl, reg::rax[-123]);
    a.xaddb(reg::cl, reg::rax[-123]);
    a.cmpxchgb(reg::cl, reg::rax[-123]);

    a.xchgb(reg::dil, reg::rax[-123]);
    a.addb(reg::dil, reg::rax[-123]);
    a.subb(reg::dil, reg::rax[-123]);
    a.andb(reg::dil, reg::rax[-123]);
    a.orb(reg::dil, reg::rax[-123]);
    a.xorb(reg::dil, reg::rax[-123]);
    a.cmpb(reg::dil, reg::rax[-123]);
    a.adcb(reg::dil, reg::rax[-123]);
    a.sbbb(reg::dil, reg::rax[-123]);
    a.movb(reg::dil, reg::rax[-123]);
    a.testb(reg::dil, reg::rax[-123]);
    a.xaddb(reg::dil, reg::rax[-123]);
    a.cmpxchgb(reg::dil, reg::rax[-123]);

    a.xchgb(reg::r8b, reg::rax[-123]);
    a.addb(reg::r8b, reg::rax[-123]);
    a.subb(reg::r8b, reg::rax[-123]);
    a.andb(reg::r8b, reg::rax[-123]);
    a.orb(reg::r8b, reg::rax[-123]);
    a.xorb(reg::r8b, reg::rax[-123]);
    a.cmpb(reg::r8b, reg::rax[-123]);
    a.adcb(reg::r8b, reg::rax[-123]);
    a.sbbb(reg::r8b, reg::rax[-123]);
    a.movb(reg::r8b, reg::rax[-123]);
    a.testb(reg::r8b, reg::rax[-123]);
    a.xaddb(reg::r8b, reg::rax[-123]);
    a.cmpxchgb(reg::r8b, reg::rax[-123]);

    a.xchgb(reg::cl, reg::r13[-123]);
    a.addb(reg::cl, reg::r13[-123]);
    a.subb(reg::cl, reg::r13[-123]);
    a.andb(reg::cl, reg::r13[-123]);
    a.orb(reg::cl, reg::r13[-123]);
    a.xorb(reg::cl, reg::r13[-123]);
    a.cmpb(reg::cl, reg::r13[-123]);
    a.adcb(reg::cl, reg::r13[-123]);
    a.sbbb(reg::cl, reg::r13[-123]);
    a.movb(reg::cl, reg::r13[-123]);
    a.testb(reg::cl, reg::r13[-123]);
    a.xaddb(reg::cl, reg::r13[-123]);
    a.cmpxchgb(reg::cl, reg::r13[-123]);

    a.xchgb(reg::dil, reg::r13[-123]);
    a.addb(reg::dil, reg::r13[-123]);
    a.subb(reg::dil, reg::r13[-123]);
    a.andb(reg::dil, reg::r13[-123]);
    a.orb(reg::dil, reg::r13[-123]);
    a.xorb(reg::dil, reg::r13[-123]);
    a.cmpb(reg::dil, reg::r13[-123]);
    a.adcb(reg::dil, reg::r13[-123]);
    a.sbbb(reg::dil, reg::r13[-123]);
    a.movb(reg::dil, reg::r13[-123]);
    a.testb(reg::dil, reg::r13[-123]);
    a.xaddb(reg::dil, reg::r13[-123]);
    a.cmpxchgb(reg::dil, reg::r13[-123]);

    a.xchgb(reg::r8b, reg::r13[-123]);
    a.addb(reg::r8b, reg::r13[-123]);
    a.subb(reg::r8b, reg::r13[-123]);
    a.andb(reg::r8b, reg::r13[-123]);
    a.orb(reg::r8b, reg::r13[-123]);
    a.xorb(reg::r8b, reg::r13[-123]);
    a.cmpb(reg::r8b, reg::r13[-123]);
    a.adcb(reg::r8b, reg::r13[-123]);
    a.sbbb(reg::r8b, reg::r13[-123]);
    a.movb(reg::r8b, reg::r13[-123]);
    a.testb(reg::r8b, reg::r13[-123]);
    a.xaddb(reg::r8b, reg::r13[-123]);
    a.cmpxchgb(reg::r8b, reg::r13[-123]);

    // -------------------------- MR (qword) -----------------------------

    a.leaq(reg::rax[-123], reg::rdi);
    a.xchgq(reg::rax[-123], reg::rdi);
    a.addq(reg::rax[-123], reg::rdi);
    a.subq(reg::rax[-123], reg::rdi);
    a.andq(reg::rax[-123], reg::rdi);
    a.orq(reg::rax[-123], reg::rdi);
    a.xorq(reg::rax[-123], reg::rdi);
    a.cmpq(reg::rax[-123], reg::rdi);
    a.adcq(reg::rax[-123], reg::rdi);
    a.sbbq(reg::rax[-123], reg::rdi);
    a.movq(reg::rax[-123], reg::rdi);
    a.testq(reg::rax[-123], reg::rdi);
    a.imulq(reg::rax[-123], reg::rdi);
    a.movzxwq(reg::rax[-123], reg::rdi); // src is always word size
    a.movzxbq(reg::rax[-123], reg::rdi); // src is always byte size
    a.movsxdq(reg::rax[-123], reg::rdi); // src is always dword size
    a.movsxwq(reg::rax[-123], reg::rdi); // src is always word size
    a.movsxbq(reg::rax[-123], reg::rdi); // src is always byte size

    a.cmovccq(CC_E, reg::rax[-123], reg::rdi);

    a.movq(reg::rax[-123], reg::xmm7);
    a.movsd(reg::rax[-123], reg::xmm7);
    a.addsd(reg::rax[-123], reg::xmm7);
    a.subsd(reg::rax[-123], reg::xmm7);
    a.mulsd(reg::rax[-123], reg::xmm7);
    a.divsd(reg::rax[-123], reg::xmm7);
    a.sqrtsd(reg::rax[-123], reg::xmm7);
    a.ucomisd(reg::rax[-123], reg::xmm7);
    a.pxor(reg::rax[-123], reg::xmm7);
    a.psllq(reg::rax[-123], reg::xmm7);
    a.psrlq(reg::rax[-123], reg::xmm7);
    a.cvtsi2sdq(reg::rax[-123], reg::xmm7);
    a.cvttsd2siq(reg::rax[-123], reg::rdi);
    a.movdqa(reg::rax[-123], reg::xmm7);
    a.movdqu(reg::rax[-123], reg::xmm7);
    a.movaps(reg::rax[-123], reg::xmm7);
    a.movups(reg::rax[-123], reg::xmm7);
    a.movapd(reg::rax[-123], reg::xmm7);
    a.movupd(reg::rax[-123], reg::xmm7);
    a.lddqu(reg::rax[-123], reg::xmm7);
    a.unpcklpd(reg::rax[-123], reg::xmm7);

    a.leaq(reg::rax[-123], reg::r12);
    a.xchgq(reg::rax[-123], reg::r12);
    a.addq(reg::rax[-123], reg::r12);
    a.subq(reg::rax[-123], reg::r12);
    a.andq(reg::rax[-123], reg::r12);
    a.orq(reg::rax[-123], reg::r12);
    a.xorq(reg::rax[-123], reg::r12);
    a.cmpq(reg::rax[-123], reg::r12);
    a.adcq(reg::rax[-123], reg::r12);
    a.sbbq(reg::rax[-123], reg::r12);
    a.movq(reg::rax[-123], reg::r12);
    a.testq(reg::rax[-123], reg::r12);
    a.imulq(reg::rax[-123], reg::r12);
    a.movzxwq(reg::rax[-123], reg::r12); // src is always word size
    a.movzxbq(reg::rax[-123], reg::r12); // src is always byte size
    a.movsxdq(reg::rax[-123], reg::r12); // src is always dword size
    a.movsxwq(reg::rax[-123], reg::r12); // src is always word size
    a.movsxbq(reg::rax[-123], reg::r12); // src is always byte size

    a.cmovccq(CC_E, reg::rax[-123], reg::r12);

    a.movq(reg::rax[-123], reg::xmm12);
    a.movsd(reg::rax[-123], reg::xmm12);
    a.addsd(reg::rax[-123], reg::xmm12);
    a.subsd(reg::rax[-123], reg::xmm12);
    a.mulsd(reg::rax[-123], reg::xmm12);
    a.divsd(reg::rax[-123], reg::xmm12);
    a.sqrtsd(reg::rax[-123], reg::xmm12);
    a.ucomisd(reg::rax[-123], reg::xmm12);
    a.pxor(reg::rax[-123], reg::xmm12);
    a.psllq(reg::rax[-123], reg::xmm12);
    a.psrlq(reg::rax[-123], reg::xmm12);
    a.cvtsi2sdq(reg::rax[-123], reg::xmm12);
    a.cvttsd2siq(reg::rax[-123], reg::r12);
    a.movdqa(reg::rax[-123], reg::xmm12);
    a.movdqu(reg::rax[-123], reg::xmm12);
    a.movaps(reg::rax[-123], reg::xmm12);
    a.movups(reg::rax[-123], reg::xmm12);
    a.movapd(reg::rax[-123], reg::xmm12);
    a.movupd(reg::rax[-123], reg::xmm12);
    a.lddqu(reg::rax[-123], reg::xmm12);
    a.unpcklpd(reg::rax[-123], reg::xmm12);

    a.leaq(reg::r13[-123], reg::rdi);
    a.xchgq(reg::r13[-123], reg::rdi);
    a.addq(reg::r13[-123], reg::rdi);
    a.subq(reg::r13[-123], reg::rdi);
    a.andq(reg::r13[-123], reg::rdi);
    a.orq(reg::r13[-123], reg::rdi);
    a.xorq(reg::r13[-123], reg::rdi);
    a.cmpq(reg::r13[-123], reg::rdi);
    a.adcq(reg::r13[-123], reg::rdi);
    a.sbbq(reg::r13[-123], reg::rdi);
    a.movq(reg::r13[-123], reg::rdi);
    a.testq(reg::r13[-123], reg::rdi);
    a.imulq(reg::r13[-123], reg::rdi);
    a.movzxwq(reg::r13[-123], reg::rdi); // src is always word size
    a.movzxbq(reg::r13[-123], reg::rdi); // src is always byte size
    a.movsxdq(reg::r13[-123], reg::rdi); // src is always dword size
    a.movsxwq(reg::r13[-123], reg::rdi); // src is always word size
    a.movsxbq(reg::r13[-123], reg::rdi); // src is always byte size

    a.cmovccq(CC_E, reg::r13[-123], reg::rdi);

    a.movq(reg::r13[-123], reg::xmm7);
    a.movsd(reg::r13[-123], reg::xmm7);
    a.addsd(reg::r13[-123], reg::xmm7);
    a.subsd(reg::r13[-123], reg::xmm7);
    a.mulsd(reg::r13[-123], reg::xmm7);
    a.divsd(reg::r13[-123], reg::xmm7);
    a.sqrtsd(reg::r13[-123], reg::xmm7);
    a.ucomisd(reg::r13[-123], reg::xmm7);
    a.pxor(reg::r13[-123], reg::xmm7);
    a.psllq(reg::r13[-123], reg::xmm7);
    a.psrlq(reg::r13[-123], reg::xmm7);
    a.cvtsi2sdq(reg::r13[-123], reg::xmm7);
    a.cvttsd2siq(reg::r13[-123], reg::rdi);
    a.movdqa(reg::r13[-123], reg::xmm7);
    a.movdqu(reg::r13[-123], reg::xmm7);
    a.movaps(reg::r13[-123], reg::xmm7);
    a.movups(reg::r13[-123], reg::xmm7);
    a.movapd(reg::r13[-123], reg::xmm7);
    a.movupd(reg::r13[-123], reg::xmm7);
    a.lddqu(reg::r13[-123], reg::xmm7);
    a.unpcklpd(reg::r13[-123], reg::xmm7);

    a.leaq(reg::r13[-123], reg::r12);
    a.xchgq(reg::r13[-123], reg::r12);
    a.addq(reg::r13[-123], reg::r12);
    a.subq(reg::r13[-123], reg::r12);
    a.andq(reg::r13[-123], reg::r12);
    a.orq(reg::r13[-123], reg::r12);
    a.xorq(reg::r13[-123], reg::r12);
    a.cmpq(reg::r13[-123], reg::r12);
    a.adcq(reg::r13[-123], reg::r12);
    a.sbbq(reg::r13[-123], reg::r12);
    a.movq(reg::r13[-123], reg::r12);
    a.testq(reg::r13[-123], reg::r12);
    a.imulq(reg::r13[-123], reg::r12);
    a.movzxwq(reg::r13[-123], reg::r12); // src is always word size
    a.movzxbq(reg::r13[-123], reg::r12); // src is always byte size
    a.movsxdq(reg::r13[-123], reg::r12); // src is always dword size
    a.movsxwq(reg::r13[-123], reg::r12); // src is always word size
    a.movsxbq(reg::r13[-123], reg::r12); // src is always byte size

    a.cmovccq(CC_E, reg::r13[-123], reg::r12);

    a.movq(reg::r13[-123], reg::xmm12);
    a.movsd(reg::r13[-123], reg::xmm12);
    a.addsd(reg::r13[-123], reg::xmm12);
    a.subsd(reg::r13[-123], reg::xmm12);
    a.mulsd(reg::r13[-123], reg::xmm12);
    a.divsd(reg::r13[-123], reg::xmm12);
    a.sqrtsd(reg::r13[-123], reg::xmm12);
    a.ucomisd(reg::r13[-123], reg::xmm12);
    a.pxor(reg::r13[-123], reg::xmm12);
    a.psllq(reg::r13[-123], reg::xmm12);
    a.psrlq(reg::r13[-123], reg::xmm12);
    a.cvtsi2sdq(reg::r13[-123], reg::xmm12);
    a.cvttsd2siq(reg::r13[-123], reg::r12);
    a.movdqa(reg::r13[-123], reg::xmm12);
    a.movdqu(reg::r13[-123], reg::xmm12);
    a.movaps(reg::r13[-123], reg::xmm12);
    a.movups(reg::r13[-123], reg::xmm12);
    a.movapd(reg::r13[-123], reg::xmm12);
    a.movupd(reg::r13[-123], reg::xmm12);
    a.lddqu(reg::r13[-123], reg::xmm12);
    a.unpcklpd(reg::r13[-123], reg::xmm12);

    // -------------------------- MR (dword) -----------------------------

    a.leal(reg::rax[-123], reg::edi);
    a.xchgl(reg::rax[-123], reg::edi);
    a.addl(reg::rax[-123], reg::edi);
    a.subl(reg::rax[-123], reg::edi);
    a.andl(reg::rax[-123], reg::edi);
    a.orl(reg::rax[-123], reg::edi);
    a.xorl(reg::rax[-123], reg::edi);
    a.cmpl(reg::rax[-123], reg::edi);
    a.adcl(reg::rax[-123], reg::edi);
    a.sbbl(reg::rax[-123], reg::edi);
    a.movl(reg::rax[-123], reg::edi);
    a.testl(reg::rax[-123], reg::edi);
    a.imull(reg::rax[-123], reg::edi);
    a.movzxwl(reg::rax[-123], reg::edi); // src is always word size
    a.movzxbl(reg::rax[-123], reg::edi); // src is always byte size
    a.movsxwl(reg::rax[-123], reg::edi); // src is always word size
    a.movsxbl(reg::rax[-123], reg::edi); // src is always byte size

    a.cmovccl(CC_E, reg::rax[-123], reg::edi);

    a.movl(reg::rax[-123], reg::xmm7);
    a.cvtsi2sdl(reg::rax[-123], reg::xmm7);
    a.cvttsd2sil(reg::rax[-123], reg::edi);

    a.leal(reg::rax[-123], reg::r12d);
    a.xchgl(reg::rax[-123], reg::r12d);
    a.addl(reg::rax[-123], reg::r12d);
    a.subl(reg::rax[-123], reg::r12d);
    a.andl(reg::rax[-123], reg::r12d);
    a.orl(reg::rax[-123], reg::r12d);
    a.xorl(reg::rax[-123], reg::r12d);
    a.cmpl(reg::rax[-123], reg::r12d);
    a.adcl(reg::rax[-123], reg::r12d);
    a.sbbl(reg::rax[-123], reg::r12d);
    a.movl(reg::rax[-123], reg::r12d);
    a.testl(reg::rax[-123], reg::r12d);
    a.imull(reg::rax[-123], reg::r12d);
    a.movzxwl(reg::rax[-123], reg::r12d); // src is always word size
    a.movzxbl(reg::rax[-123], reg::r12d); // src is always byte size
    a.movsxwl(reg::rax[-123], reg::r12d); // src is always word size
    a.movsxbl(reg::rax[-123], reg::r12d); // src is always byte size

    a.cmovccl(CC_E, reg::rax[-123], reg::r12d);

    a.movl(reg::rax[-123], reg::xmm12);
    a.cvtsi2sdl(reg::rax[-123], reg::xmm12);
    a.cvttsd2sil(reg::rax[-123], reg::r12d);

    a.leal(reg::r13[-123], reg::edi);
    a.xchgl(reg::r13[-123], reg::edi);
    a.addl(reg::r13[-123], reg::edi);
    a.subl(reg::r13[-123], reg::edi);
    a.andl(reg::r13[-123], reg::edi);
    a.orl(reg::r13[-123], reg::edi);
    a.xorl(reg::r13[-123], reg::edi);
    a.cmpl(reg::r13[-123], reg::edi);
    a.adcl(reg::r13[-123], reg::edi);
    a.sbbl(reg::r13[-123], reg::edi);
    a.movl(reg::r13[-123], reg::edi);
    a.testl(reg::r13[-123], reg::edi);
    a.imull(reg::r13[-123], reg::edi);
    a.movzxwl(reg::r13[-123], reg::edi); // src is always word size
    a.movzxbl(reg::r13[-123], reg::edi); // src is always byte size
    a.movsxwl(reg::r13[-123], reg::edi); // src is always word size
    a.movsxbl(reg::r13[-123], reg::edi); // src is always byte size

    a.cmovccl(CC_E, reg::r13[-123], reg::edi);

    a.movl(reg::r13[-123], reg::xmm7);
    a.cvtsi2sdl(reg::r13[-123], reg::xmm7);
    a.cvttsd2sil(reg::r13[-123], reg::edi);

    a.leal(reg::r13[-123], reg::r12d);
    a.xchgl(reg::r13[-123], reg::r12d);
    a.addl(reg::r13[-123], reg::r12d);
    a.subl(reg::r13[-123], reg::r12d);
    a.andl(reg::r13[-123], reg::r12d);
    a.orl(reg::r13[-123], reg::r12d);
    a.xorl(reg::r13[-123], reg::r12d);
    a.cmpl(reg::r13[-123], reg::r12d);
    a.adcl(reg::r13[-123], reg::r12d);
    a.sbbl(reg::r13[-123], reg::r12d);
    a.movl(reg::r13[-123], reg::r12d);
    a.testl(reg::r13[-123], reg::r12d);
    a.imull(reg::r13[-123], reg::r12d);
    a.movzxwl(reg::r13[-123], reg::r12d); // src is always word size
    a.movzxbl(reg::r13[-123], reg::r12d); // src is always byte size
    a.movsxwl(reg::r13[-123], reg::r12d); // src is always word size
    a.movsxbl(reg::r13[-123], reg::r12d); // src is always byte size

    a.cmovccl(CC_E, reg::r13[-123], reg::r12d);

    a.movl(reg::r13[-123], reg::xmm12);
    a.cvtsi2sdl(reg::r13[-123], reg::xmm12);
    a.cvttsd2sil(reg::r13[-123], reg::r12d);

    // -------------------------- MR (word) -----------------------------

    a.leaw(reg::rax[-123], reg::di);
    a.xchgw(reg::rax[-123], reg::di);
    a.addw(reg::rax[-123], reg::di);
    a.subw(reg::rax[-123], reg::di);
    a.andw(reg::rax[-123], reg::di);
    a.orw(reg::rax[-123], reg::di);
    a.xorw(reg::rax[-123], reg::di);
    a.cmpw(reg::rax[-123], reg::di);
    a.adcw(reg::rax[-123], reg::di);
    a.sbbw(reg::rax[-123], reg::di);
    a.movw(reg::rax[-123], reg::di);
    a.testw(reg::rax[-123], reg::di);
    a.imulw(reg::rax[-123], reg::di);
    a.movzxbw(reg::rax[-123], reg::di); // src is always byte size
    a.movsxbw(reg::rax[-123], reg::di); // src is always byte size

    a.cmovccw(CC_E, reg::rax[-123], reg::di);

    a.leaw(reg::rax[-123], reg::r12w);
    a.xchgw(reg::rax[-123], reg::r12w);
    a.addw(reg::rax[-123], reg::r12w);
    a.subw(reg::rax[-123], reg::r12w);
    a.andw(reg::rax[-123], reg::r12w);
    a.orw(reg::rax[-123], reg::r12w);
    a.xorw(reg::rax[-123], reg::r12w);
    a.cmpw(reg::rax[-123], reg::r12w);
    a.adcw(reg::rax[-123], reg::r12w);
    a.sbbw(reg::rax[-123], reg::r12w);
    a.movw(reg::rax[-123], reg::r12w);
    a.testw(reg::rax[-123], reg::r12w);
    a.imulw(reg::rax[-123], reg::r12w);
    a.movzxbw(reg::rax[-123], reg::r12w); // src is always byte size
    a.movsxbw(reg::rax[-123], reg::r12w); // src is always byte size

    a.cmovccw(CC_E, reg::rax[-123], reg::r12w);

    a.leaw(reg::r13[-123], reg::di);
    a.xchgw(reg::r13[-123], reg::di);
    a.addw(reg::r13[-123], reg::di);
    a.subw(reg::r13[-123], reg::di);
    a.andw(reg::r13[-123], reg::di);
    a.orw(reg::r13[-123], reg::di);
    a.xorw(reg::r13[-123], reg::di);
    a.cmpw(reg::r13[-123], reg::di);
    a.adcw(reg::r13[-123], reg::di);
    a.sbbw(reg::r13[-123], reg::di);
    a.movw(reg::r13[-123], reg::di);
    a.testw(reg::r13[-123], reg::di);
    a.imulw(reg::r13[-123], reg::di);
    a.movzxbw(reg::r13[-123], reg::di); // src is always byte size
    a.movsxbw(reg::r13[-123], reg::di); // src is always byte size

    a.cmovccw(CC_E, reg::r13[-123], reg::di);

    a.leaw(reg::r13[-123], reg::r12w);
    a.xchgw(reg::r13[-123], reg::r12w);
    a.addw(reg::r13[-123], reg::r12w);
    a.subw(reg::r13[-123], reg::r12w);
    a.andw(reg::r13[-123], reg::r12w);
    a.orw(reg::r13[-123], reg::r12w);
    a.xorw(reg::r13[-123], reg::r12w);
    a.cmpw(reg::r13[-123], reg::r12w);
    a.adcw(reg::r13[-123], reg::r12w);
    a.sbbw(reg::r13[-123], reg::r12w);
    a.movw(reg::r13[-123], reg::r12w);
    a.testw(reg::r13[-123], reg::r12w);
    a.imulw(reg::r13[-123], reg::r12w);
    a.movzxbw(reg::r13[-123], reg::r12w); // src is always byte size
    a.movsxbw(reg::r13[-123], reg::r12w); // src is always byte size

    a.cmovccw(CC_E, reg::r13[-123], reg::r12w);

    // -------------------------- MR (byte) -----------------------------

    a.xchgb(reg::rax[-123], reg::dil);
    a.addb(reg::rax[-123], reg::dil);
    a.subb(reg::rax[-123], reg::dil);
    a.andb(reg::rax[-123], reg::dil);
    a.orb(reg::rax[-123], reg::dil);
    a.xorb(reg::rax[-123], reg::dil);
    a.cmpb(reg::rax[-123], reg::dil);
    a.adcb(reg::rax[-123], reg::dil);
    a.sbbb(reg::rax[-123], reg::dil);
    a.movb(reg::rax[-123], reg::dil);
    a.testb(reg::rax[-123], reg::dil);

    a.xchgb(reg::rax[-123], reg::r12b);
    a.addb(reg::rax[-123], reg::r12b);
    a.subb(reg::rax[-123], reg::r12b);
    a.andb(reg::rax[-123], reg::r12b);
    a.orb(reg::rax[-123], reg::r12b);
    a.xorb(reg::rax[-123], reg::r12b);
    a.cmpb(reg::rax[-123], reg::r12b);
    a.adcb(reg::rax[-123], reg::r12b);
    a.sbbb(reg::rax[-123], reg::r12b);
    a.movb(reg::rax[-123], reg::r12b);
    a.testb(reg::rax[-123], reg::r12b);

    a.xchgb(reg::r13[-123], reg::dil);
    a.addb(reg::r13[-123], reg::dil);
    a.subb(reg::r13[-123], reg::dil);
    a.andb(reg::r13[-123], reg::dil);
    a.orb(reg::r13[-123], reg::dil);
    a.xorb(reg::r13[-123], reg::dil);
    a.cmpb(reg::r13[-123], reg::dil);
    a.adcb(reg::r13[-123], reg::dil);
    a.sbbb(reg::r13[-123], reg::dil);
    a.movb(reg::r13[-123], reg::dil);
    a.testb(reg::r13[-123], reg::dil);

    a.xchgb(reg::r13[-123], reg::r12b);
    a.addb(reg::r13[-123], reg::r12b);
    a.subb(reg::r13[-123], reg::r12b);
    a.andb(reg::r13[-123], reg::r12b);
    a.orb(reg::r13[-123], reg::r12b);
    a.xorb(reg::r13[-123], reg::r12b);
    a.cmpb(reg::r13[-123], reg::r12b);
    a.adcb(reg::r13[-123], reg::r12b);
    a.sbbb(reg::r13[-123], reg::r12b);
    a.movb(reg::r13[-123], reg::r12b);
    a.testb(reg::r13[-123], reg::r12b);

    // -------------------------- IR (qword, 4 byte imm) -----------------------------

    a.addq(0x10000, reg::rdi);
    a.subq(0x10000, reg::rdi);
    a.andq(0x10000, reg::rdi);
    a.orq(0x10000, reg::rdi);
    a.xorq(0x10000, reg::rdi);
    a.cmpq(0x10000, reg::rdi);
    a.adcq(0x10000, reg::rdi);
    a.sbbq(0x10000, reg::rdi);
    a.movq(0x10000, reg::rdi);
    a.testq(0x10000, reg::rdi);

    a.addq(0x10000, reg::r12);
    a.subq(0x10000, reg::r12);
    a.andq(0x10000, reg::r12);
    a.orq(0x10000, reg::r12);
    a.xorq(0x10000, reg::r12);
    a.cmpq(0x10000, reg::r12);
    a.adcq(0x10000, reg::r12);
    a.sbbq(0x10000, reg::r12);
    a.movq(0x10000, reg::r12);
    a.testq(0x10000, reg::r12);

    // -------------------------- IR (qword, byte imm) -----------------------------

    a.addq(-123, reg::rdi);
    a.subq(-123, reg::rdi);
    a.andq(-123, reg::rdi);
    a.orq(-123, reg::rdi);
    a.xorq(-123, reg::rdi);
    a.cmpq(-123, reg::rdi);
    a.adcq(-123, reg::rdi);
    a.sbbq(-123, reg::rdi);
    a.rolq(63, reg::rdi);
    a.rorq(63, reg::rdi);
    a.rclq(63, reg::rdi);
    a.rcrq(63, reg::rdi);
    a.shlq(63, reg::rdi);
    a.shrq(63, reg::rdi);
    a.sarq(63, reg::rdi);
    a.psllq(63, reg::xmm7);
    a.psrlq(63, reg::xmm7);

    a.addq(-123, reg::r12);
    a.subq(-123, reg::r12);
    a.andq(-123, reg::r12);
    a.orq(-123, reg::r12);
    a.xorq(-123, reg::r12);
    a.cmpq(-123, reg::r12);
    a.adcq(-123, reg::r12);
    a.sbbq(-123, reg::r12);
    a.rolq(63, reg::r12);
    a.rorq(63, reg::r12);
    a.rclq(63, reg::r12);
    a.rcrq(63, reg::r12);
    a.shlq(63, reg::r12);
    a.shrq(63, reg::r12);
    a.sarq(63, reg::r12);
    a.psllq(63, reg::xmm12);
    a.psrlq(63, reg::xmm12);

    // -------------------------- IR (qword, special cases) -----------------------------

    a.andq(0x00000000ffffffffLL, reg::rdi);
    a.testq(0x00000000ffffffffLL, reg::rdi);
    a.movq(0x00000000ffffffffLL, reg::rdi);
    a.movq(0x7fffffffffffffffLL, reg::rdi);
    a.movq(0x8000000000000000LL, reg::rdi);

    a.andq(0x00000000ffffffffLL, reg::r12);
    a.testq(0x00000000ffffffffLL, reg::r12);
    a.movq(0x00000000ffffffffLL, reg::r12);
    a.movq(0x7fffffffffffffffLL, reg::r12);
    a.movq(0x8000000000000000LL, reg::r12);

    // -------------------------- IR (dword, 4 byte imm) -----------------------------

    a.addl(0x10000, reg::edi);
    a.subl(0x10000, reg::edi);
    a.andl(0x10000, reg::edi);
    a.orl(0x10000, reg::edi);
    a.xorl(0x10000, reg::edi);
    a.cmpl(0x10000, reg::edi);
    a.adcl(0x10000, reg::edi);
    a.sbbl(0x10000, reg::edi);
    a.movl(0x10000, reg::edi);
    a.testl(0x10000, reg::edi);

    a.addl(0x10000, reg::r12d);
    a.subl(0x10000, reg::r12d);
    a.andl(0x10000, reg::r12d);
    a.orl(0x10000, reg::r12d);
    a.xorl(0x10000, reg::r12d);
    a.cmpl(0x10000, reg::r12d);
    a.adcl(0x10000, reg::r12d);
    a.sbbl(0x10000, reg::r12d);
    a.movl(0x10000, reg::r12d);
    a.testl(0x10000, reg::r12d);

    // -------------------------- IR (dword, byte imm) -----------------------------

    a.addl(-123, reg::edi);
    a.subl(-123, reg::edi);
    a.andl(-123, reg::edi);
    a.orl(-123, reg::edi);
    a.xorl(-123, reg::edi);
    a.cmpl(-123, reg::edi);
    a.adcl(-123, reg::edi);
    a.sbbl(-123, reg::edi);
    a.roll(31, reg::edi);
    a.rorl(31, reg::edi);
    a.rcll(31, reg::edi);
    a.rcrl(31, reg::edi);
    a.shll(31, reg::edi);
    a.shrl(31, reg::edi);
    a.sarl(31, reg::edi);

    a.addl(-123, reg::r12d);
    a.subl(-123, reg::r12d);
    a.andl(-123, reg::r12d);
    a.orl(-123, reg::r12d);
    a.xorl(-123, reg::r12d);
    a.cmpl(-123, reg::r12d);
    a.adcl(-123, reg::r12d);
    a.sbbl(-123, reg::r12d);
    a.roll(31, reg::r12d);
    a.rorl(31, reg::r12d);
    a.rcll(31, reg::r12d);
    a.rcrl(31, reg::r12d);
    a.shll(31, reg::r12d);
    a.shrl(31, reg::r12d);
    a.sarl(31, reg::r12d);

    // -------------------------- IR (dword, special case) -----------------------------

    a.andl(0x00000000ffffffffLL, reg::edi);
    a.testl(0x00000000ffffffffLL, reg::edi);
    a.movl(0x00000000ffffffffLL, reg::edi);

    a.andl(0x00000000ffffffffLL, reg::r12d);
    a.testl(0x00000000ffffffffLL, reg::r12d);
    a.movl(0x00000000ffffffffLL, reg::r12d);

    // -------------------------- IR (word, 2 byte imm) -----------------------------

    a.addw(0x7fff, reg::di);
    a.subw(0x7fff, reg::di);
    a.andw(0x7fff, reg::di);
    a.orw(0x7fff, reg::di);
    a.xorw(0x7fff, reg::di);
    a.cmpw(0x7fff, reg::di);
    a.adcw(0x7fff, reg::di);
    a.sbbw(0x7fff, reg::di);
    a.movw(0x7fff, reg::di);
    a.testw(0x7fff, reg::di);

    a.addw(0x7fff, reg::r12w);
    a.subw(0x7fff, reg::r12w);
    a.andw(0x7fff, reg::r12w);
    a.orw(0x7fff, reg::r12w);
    a.xorw(0x7fff, reg::r12w);
    a.cmpw(0x7fff, reg::r12w);
    a.adcw(0x7fff, reg::r12w);
    a.sbbw(0x7fff, reg::r12w);
    a.movw(0x7fff, reg::r12w);
    a.testw(0x7fff, reg::r12w);

    // -------------------------- IR (word, byte imm) -----------------------------

    a.addw(-123, reg::di);
    a.subw(-123, reg::di);
    a.andw(-123, reg::di);
    a.orw(-123, reg::di);
    a.xorw(-123, reg::di);
    a.cmpw(-123, reg::di);
    a.adcw(-123, reg::di);
    a.sbbw(-123, reg::di);
    a.rolw(15, reg::di);
    a.rorw(15, reg::di);
    a.rclw(15, reg::di);
    a.rcrw(15, reg::di);
    a.shlw(15, reg::di);
    a.shrw(15, reg::di);
    a.sarw(15, reg::di);

    a.addw(-123, reg::r12w);
    a.subw(-123, reg::r12w);
    a.andw(-123, reg::r12w);
    a.orw(-123, reg::r12w);
    a.xorw(-123, reg::r12w);
    a.cmpw(-123, reg::r12w);
    a.adcw(-123, reg::r12w);
    a.sbbw(-123, reg::r12w);
    a.rolw(15, reg::r12w);
    a.rorw(15, reg::r12w);
    a.rclw(15, reg::r12w);
    a.rcrw(15, reg::r12w);
    a.shlw(15, reg::r12w);
    a.shrw(15, reg::r12w);
    a.sarw(15, reg::r12w);

    // -------------------------- IR (byte, byte imm) -----------------------------

    a.addb(-123, reg::dil);
    a.subb(-123, reg::dil);
    a.andb(-123, reg::dil);
    a.orb(-123, reg::dil);
    a.xorb(-123, reg::dil);
    a.cmpb(-123, reg::dil);
    a.adcb(-123, reg::dil);
    a.sbbb(-123, reg::dil);
    a.movb(-123, reg::dil);
    a.testb(-123, reg::dil);
    a.rolb(7, reg::dil);
    a.rorb(7, reg::dil);
    a.rclb(7, reg::dil);
    a.rcrb(7, reg::dil);
    a.shlb(7, reg::dil);
    a.shrb(7, reg::dil);
    a.sarb(7, reg::dil);

    a.addb(-123, reg::r12b);
    a.subb(-123, reg::r12b);
    a.andb(-123, reg::r12b);
    a.orb(-123, reg::r12b);
    a.xorb(-123, reg::r12b);
    a.cmpb(-123, reg::r12b);
    a.adcb(-123, reg::r12b);
    a.sbbb(-123, reg::r12b);
    a.testb(-123, reg::r12b);
    a.movb(-123, reg::r12b);
    a.rolb(7, reg::r12b);
    a.rorb(7, reg::r12b);
    a.rclb(7, reg::r12b);
    a.rcrb(7, reg::r12b);
    a.shlb(7, reg::r12b);
    a.shrb(7, reg::r12b);
    a.sarb(7, reg::r12b);

    // -------------------------- IM (qword, 4 byte imm) -----------------------------

    a.addq(0x10000, reg::rax[-123]);
    a.subq(0x10000, reg::rax[-123]);
    a.andq(0x10000, reg::rax[-123]);
    a.orq(0x10000, reg::rax[-123]);
    a.xorq(0x10000, reg::rax[-123]);
    a.cmpq(0x10000, reg::rax[-123]);
    a.adcq(0x10000, reg::rax[-123]);
    a.sbbq(0x10000, reg::rax[-123]);
    a.movq(0x10000, reg::rax[-123]);
    a.testq(0x10000, reg::rax[-123]);

    a.addq(0x10000, reg::r13[-123]);
    a.subq(0x10000, reg::r13[-123]);
    a.andq(0x10000, reg::r13[-123]);
    a.orq(0x10000, reg::r13[-123]);
    a.xorq(0x10000, reg::r13[-123]);
    a.cmpq(0x10000, reg::r13[-123]);
    a.adcq(0x10000, reg::r13[-123]);
    a.sbbq(0x10000, reg::r13[-123]);
    a.movq(0x10000, reg::r13[-123]);
    a.testq(0x10000, reg::r13[-123]);

    // -------------------------- IM (qword, byte imm) -----------------------------

    a.addq(-123, reg::rax[-123]);
    a.subq(-123, reg::rax[-123]);
    a.andq(-123, reg::rax[-123]);
    a.orq(-123, reg::rax[-123]);
    a.xorq(-123, reg::rax[-123]);
    a.cmpq(-123, reg::rax[-123]);
    a.adcq(-123, reg::rax[-123]);
    a.sbbq(-123, reg::rax[-123]);
    a.rolq(63, reg::rax[-123]);
    a.rorq(63, reg::rax[-123]);
    a.rclq(63, reg::rax[-123]);
    a.rcrq(63, reg::rax[-123]);
    a.shlq(63, reg::rax[-123]);
    a.shrq(63, reg::rax[-123]);
    a.sarq(63, reg::rax[-123]);

    a.addq(-123, reg::r13[-123]);
    a.subq(-123, reg::r13[-123]);
    a.andq(-123, reg::r13[-123]);
    a.orq(-123, reg::r13[-123]);
    a.xorq(-123, reg::r13[-123]);
    a.cmpq(-123, reg::r13[-123]);
    a.adcq(-123, reg::r13[-123]);
    a.sbbq(-123, reg::r13[-123]);
    a.rolq(63, reg::r13[-123]);
    a.rorq(63, reg::r13[-123]);
    a.rclq(63, reg::r13[-123]);
    a.rcrq(63, reg::r13[-123]);
    a.shlq(63, reg::r13[-123]);
    a.shrq(63, reg::r13[-123]);
    a.sarq(63, reg::r13[-123]);

    // -------------------------- IM (dword, 4 byte imm) -----------------------------

    a.addl(0x10000, reg::rax[-123]);
    a.subl(0x10000, reg::rax[-123]);
    a.andl(0x10000, reg::rax[-123]);
    a.orl(0x10000, reg::rax[-123]);
    a.xorl(0x10000, reg::rax[-123]);
    a.cmpl(0x10000, reg::rax[-123]);
    a.adcl(0x10000, reg::rax[-123]);
    a.sbbl(0x10000, reg::rax[-123]);
    a.movl(0x10000, reg::rax[-123]);
    a.testl(0x10000, reg::rax[-123]);

    a.addl(0x10000, reg::r13[-123]);
    a.subl(0x10000, reg::r13[-123]);
    a.andl(0x10000, reg::r13[-123]);
    a.orl(0x10000, reg::r13[-123]);
    a.xorl(0x10000, reg::r13[-123]);
    a.cmpl(0x10000, reg::r13[-123]);
    a.adcl(0x10000, reg::r13[-123]);
    a.sbbl(0x10000, reg::r13[-123]);
    a.movl(0x10000, reg::r13[-123]);
    a.testl(0x10000, reg::r13[-123]);

    // -------------------------- IM (dword, byte imm) -----------------------------

    a.addl(-123, reg::rax[-123]);
    a.subl(-123, reg::rax[-123]);
    a.andl(-123, reg::rax[-123]);
    a.orl(-123, reg::rax[-123]);
    a.xorl(-123, reg::rax[-123]);
    a.cmpl(-123, reg::rax[-123]);
    a.adcl(-123, reg::rax[-123]);
    a.sbbl(-123, reg::rax[-123]);
    a.roll(31, reg::rax[-123]);
    a.rorl(31, reg::rax[-123]);
    a.rcll(31, reg::rax[-123]);
    a.rcrl(31, reg::rax[-123]);
    a.shll(31, reg::rax[-123]);
    a.shrl(31, reg::rax[-123]);
    a.sarl(31, reg::rax[-123]);

    a.addl(-123, reg::r13[-123]);
    a.subl(-123, reg::r13[-123]);
    a.andl(-123, reg::r13[-123]);
    a.orl(-123, reg::r13[-123]);
    a.xorl(-123, reg::r13[-123]);
    a.cmpl(-123, reg::r13[-123]);
    a.adcl(-123, reg::r13[-123]);
    a.sbbl(-123, reg::r13[-123]);
    a.roll(31, reg::r13[-123]);
    a.rorl(31, reg::r13[-123]);
    a.rcll(31, reg::r13[-123]);
    a.rcrl(31, reg::r13[-123]);
    a.shll(31, reg::r13[-123]);
    a.shrl(31, reg::r13[-123]);
    a.sarl(31, reg::r13[-123]);

    // -------------------------- IM (word, 2 byte imm) -----------------------------

    a.addw(0x7fff, reg::rax[-123]);
    a.subw(0x7fff, reg::rax[-123]);
    a.andw(0x7fff, reg::rax[-123]);
    a.orw(0x7fff, reg::rax[-123]);
    a.xorw(0x7fff, reg::rax[-123]);
    a.cmpw(0x7fff, reg::rax[-123]);
    a.adcw(0x7fff, reg::rax[-123]);
    a.sbbw(0x7fff, reg::rax[-123]);
    a.movw(0x7fff, reg::rax[-123]);
    a.testw(0x7fff, reg::rax[-123]);

    a.addw(0x7fff, reg::r13[-123]);
    a.subw(0x7fff, reg::r13[-123]);
    a.andw(0x7fff, reg::r13[-123]);
    a.orw(0x7fff, reg::r13[-123]);
    a.xorw(0x7fff, reg::r13[-123]);
    a.cmpw(0x7fff, reg::r13[-123]);
    a.adcw(0x7fff, reg::r13[-123]);
    a.sbbw(0x7fff, reg::r13[-123]);
    a.movw(0x7fff, reg::r13[-123]);
    a.testw(0x7fff, reg::r13[-123]);

    // -------------------------- IM (word, byte imm) -----------------------------

    a.addw(-123, reg::rax[-123]);
    a.subw(-123, reg::rax[-123]);
    a.andw(-123, reg::rax[-123]);
    a.orw(-123, reg::rax[-123]);
    a.xorw(-123, reg::rax[-123]);
    a.cmpw(-123, reg::rax[-123]);
    a.adcw(-123, reg::rax[-123]);
    a.sbbw(-123, reg::rax[-123]);
    a.rolw(15, reg::rax[-123]);
    a.rorw(15, reg::rax[-123]);
    a.rclw(15, reg::rax[-123]);
    a.rcrw(15, reg::rax[-123]);
    a.shlw(15, reg::rax[-123]);
    a.shrw(15, reg::rax[-123]);
    a.sarw(15, reg::rax[-123]);

    a.addw(-123, reg::r13[-123]);
    a.subw(-123, reg::r13[-123]);
    a.andw(-123, reg::r13[-123]);
    a.orw(-123, reg::r13[-123]);
    a.xorw(-123, reg::r13[-123]);
    a.cmpw(-123, reg::r13[-123]);
    a.adcw(-123, reg::r13[-123]);
    a.sbbw(-123, reg::r13[-123]);
    a.rolw(15, reg::r13[-123]);
    a.rorw(15, reg::r13[-123]);
    a.rclw(15, reg::r13[-123]);
    a.rcrw(15, reg::r13[-123]);
    a.shlw(15, reg::r13[-123]);
    a.shrw(15, reg::r13[-123]);
    a.sarw(15, reg::r13[-123]);

    // -------------------------- IM (byte, byte imm) -----------------------------

    a.addb(-123, reg::rax[-123]);
    a.subb(-123, reg::rax[-123]);
    a.andb(-123, reg::rax[-123]);
    a.orb(-123, reg::rax[-123]);
    a.xorb(-123, reg::rax[-123]);
    a.cmpb(-123, reg::rax[-123]);
    a.adcb(-123, reg::rax[-123]);
    a.sbbb(-123, reg::rax[-123]);
    a.movb(-123, reg::rax[-123]);
    a.testb(-123, reg::rax[-123]);
    a.rolb(7, reg::rax[-123]);
    a.rorb(7, reg::rax[-123]);
    a.rclb(7, reg::rax[-123]);
    a.rcrb(7, reg::rax[-123]);
    a.shlb(7, reg::rax[-123]);
    a.shrb(7, reg::rax[-123]);
    a.sarb(7, reg::rax[-123]);

    a.addb(-123, reg::r13[-123]);
    a.subb(-123, reg::r13[-123]);
    a.andb(-123, reg::r13[-123]);
    a.orb(-123, reg::r13[-123]);
    a.xorb(-123, reg::r13[-123]);
    a.cmpb(-123, reg::r13[-123]);
    a.adcb(-123, reg::r13[-123]);
    a.sbbb(-123, reg::r13[-123]);
    a.movb(-123, reg::r13[-123]);
    a.testb(-123, reg::r13[-123]);
    a.rolb(7, reg::r13[-123]);
    a.rorb(7, reg::r13[-123]);
    a.rclb(7, reg::r13[-123]);
    a.rcrb(7, reg::r13[-123]);
    a.shlb(7, reg::r13[-123]);
    a.shrb(7, reg::r13[-123]);
    a.sarb(7, reg::r13[-123]);

    // -------------------------- IRR (qword, 4 byte imm) -----------------------------

    a.imulq(0x10000, reg::rcx, reg::rdi);

    a.imulq(0x10000, reg::rcx, reg::r12);

    a.imulq(0x10000, reg::r8, reg::r12);

    // -------------------------- IRR (qword, 1 byte imm) -----------------------------

    a.imulq(-123, reg::rcx, reg::rdi);
    a.shldq(63, reg::rcx, reg::rdi);
    a.shrdq(63, reg::rcx, reg::rdi);
    a.roundsd(0xf, reg::xmm1, reg::xmm7);

    a.imulq(-123, reg::rcx, reg::r12);
    a.shldq(63, reg::rcx, reg::r12);
    a.shrdq(63, reg::rcx, reg::r12);
    a.roundsd(0xf, reg::xmm1, reg::xmm12);

    a.imulq(-123, reg::r8, reg::r12);
    a.shldq(63, reg::r8, reg::r12);
    a.shrdq(63, reg::r8, reg::r12);
    a.roundsd(0xf, reg::xmm8, reg::xmm12);

    // -------------------------- IRR (dword, 4 byte imm) -----------------------------

    a.imull(0x10000, reg::ecx, reg::edi);

    a.imull(0x10000, reg::ecx, reg::r12d);

    a.imull(0x10000, reg::r8d, reg::r12d);

    // -------------------------- IRR (dword, 1 byte imm) -----------------------------

    a.imull(-123, reg::ecx, reg::edi);
    a.shldl(31, reg::ecx, reg::edi);
    a.shrdl(31, reg::ecx, reg::edi);

    a.imull(-123, reg::ecx, reg::r12d);
    a.shldl(31, reg::ecx, reg::r12d);
    a.shrdl(31, reg::ecx, reg::r12d);

    a.imull(-123, reg::r8d, reg::r12d);
    a.shldl(31, reg::r8d, reg::r12d);
    a.shrdl(31, reg::r8d, reg::r12d);

    // -------------------------- IRR (word, 2 byte) -----------------------------

    a.imulw(0x7fff, reg::cx, reg::di);

    a.imulw(0x7fff, reg::cx, reg::r12w);

    a.imulw(0x7fff, reg::r8w, reg::r12w);

    // -------------------------- IRR (word, 1 byte) -----------------------------

    a.imulw(-123, reg::cx, reg::di);
    a.shldw(15, reg::cx, reg::di);
    a.shrdw(15, reg::cx, reg::di);

    a.imulw(-123, reg::cx, reg::r12w);
    a.shldw(15, reg::cx, reg::r12w);
    a.shrdw(15, reg::cx, reg::r12w);

    a.imulw(-123, reg::r8w, reg::r12w);
    a.shldw(15, reg::r8w, reg::r12w);
    a.shrdw(15, reg::r8w, reg::r12w);

    // -------------------------- IMR (qword, 4 byte imm) -----------------------------

    a.imulq(0x10000, reg::rax[-123], reg::rdi);
    a.imulq(0x10000, reg::rax[-123], reg::r12);
    a.imulq(0x10000, reg::r13[-123], reg::rdi);
    a.imulq(0x10000, reg::r13[-123], reg::r12);

    // -------------------------- IMR (qword, 1 byte imm) -----------------------------

    a.imulq(-123, reg::rax[-123], reg::rdi);
    a.roundsd(0xf, reg::rax[-123], reg::xmm7);

    a.imulq(-123, reg::rax[-123], reg::r12);
    a.roundsd(0xf, reg::rax[-123], reg::xmm12);

    a.imulq(-123, reg::r13[-123], reg::rdi);
    a.roundsd(0xf, reg::r13[-123], reg::xmm7);

    a.imulq(-123, reg::r13[-123], reg::r12);
    a.roundsd(0xf, reg::r13[-123], reg::xmm12);

    // -------------------------- IMR (dword, 4 byte imm) -----------------------------

    a.imull(0x10000, reg::rax[-123], reg::edi);
    a.imull(0x10000, reg::rax[-123], reg::r12d);
    a.imull(0x10000, reg::r13[-123], reg::edi);
    a.imull(0x10000, reg::r13[-123], reg::r12d);

    // -------------------------- IMR (dword, 1 byte imm) -----------------------------

    a.imull(-123, reg::rax[-123], reg::edi);
    a.imull(-123, reg::rax[-123], reg::r12d);
    a.imull(-123, reg::r13[-123], reg::edi);
    a.imull(-123, reg::r13[-123], reg::r12d);

    // -------------------------- IMR (word, 2 byte imm) -----------------------------

    a.imulw(0x7fff, reg::rax[-123], reg::di);
    a.imulw(0x7fff, reg::rax[-123], reg::r12w);
    a.imulw(0x7fff, reg::r13[-123], reg::di);
    a.imulw(0x7fff, reg::r13[-123], reg::r12w);

    // -------------------------- IMR (word, 1 byte imm) -----------------------------

    a.imulw(-123, reg::rax[-123], reg::di);
    a.imulw(-123, reg::rax[-123], reg::r12w);
    a.imulw(-123, reg::r13[-123], reg::di);
    a.imulw(-123, reg::r13[-123], reg::r12w);

    // -------------------------- IRM (qword, 1 byte imm) -----------------------------

    a.shldq(63, reg::rdi, reg::rax[-123]);
    a.shrdq(63, reg::rdi, reg::rax[-123]);
    a.shldq(63, reg::r12, reg::rax[-123]);
    a.shrdq(63, reg::r12, reg::rax[-123]);
    a.shldq(63, reg::rdi, reg::r13[-123]);
    a.shrdq(63, reg::rdi, reg::r13[-123]);
    a.shldq(63, reg::r12, reg::r13[-123]);
    a.shrdq(63, reg::r12, reg::r13[-123]);

    // -------------------------- IRM (dword, 1 byte imm) -----------------------------

    a.shldl(31, reg::edi, reg::rax[-123]);
    a.shrdl(31, reg::edi, reg::rax[-123]);
    a.shldl(31, reg::r12d, reg::rax[-123]);
    a.shrdl(31, reg::r12d, reg::rax[-123]);
    a.shldl(31, reg::edi, reg::r13[-123]);
    a.shrdl(31, reg::edi, reg::r13[-123]);
    a.shldl(31, reg::r12d, reg::r13[-123]);
    a.shrdl(31, reg::r12d, reg::r13[-123]);

    // -------------------------- IRM (word, 1 byte imm) -----------------------------

    a.shldw(15, reg::di, reg::rax[-123]);
    a.shrdw(15, reg::di, reg::rax[-123]);
    a.shldw(15, reg::r12w, reg::rax[-123]);
    a.shrdw(15, reg::r12w, reg::rax[-123]);
    a.shldw(15, reg::di, reg::r13[-123]);
    a.shrdw(15, reg::di, reg::r13[-123]);
    a.shldw(15, reg::r12w, reg::r13[-123]);
    a.shrdw(15, reg::r12w, reg::r13[-123]);

    // -------------------------- High byte regs -----------------------------

    a.incb(reg::ah);
    a.decb(reg::ah);
    a.notb(reg::ah);
    a.negb(reg::ah);
    a.mulb(reg::ah);
    a.divb(reg::ah);
    a.idivb(reg::ah);
    a.imulb(reg::ah);
    a.rolb(reg::ah);
    a.rorb(reg::ah);
    a.rclb(reg::ah);
    a.rcrb(reg::ah);
    a.shlb(reg::ah);
    a.shrb(reg::ah);
    a.sarb(reg::ah);

    a.setccb(CC_E, reg::ah);

    a.xchgb(reg::cl, reg::ah);
    a.addb(reg::cl, reg::ah);
    a.subb(reg::cl, reg::ah);
    a.andb(reg::cl, reg::ah);
    a.orb(reg::cl, reg::ah);
    a.xorb(reg::cl, reg::ah);
    a.cmpb(reg::cl, reg::ah);
    a.adcb(reg::cl, reg::ah);
    a.sbbb(reg::cl, reg::ah);
    a.movb(reg::cl, reg::ah);
    a.testb(reg::cl, reg::ah);
    a.xaddb(reg::cl, reg::ah);
    a.cmpxchgb(reg::cl, reg::ah);

    a.xchgb(reg::ch, reg::ah);
    a.addb(reg::ch, reg::ah);
    a.subb(reg::ch, reg::ah);
    a.andb(reg::ch, reg::ah);
    a.orb(reg::ch, reg::ah);
    a.xorb(reg::ch, reg::ah);
    a.cmpb(reg::ch, reg::ah);
    a.adcb(reg::ch, reg::ah);
    a.sbbb(reg::ch, reg::ah);
    a.movb(reg::ch, reg::ah);
    a.testb(reg::ch, reg::ah);
    a.xaddb(reg::ch, reg::ah);
    a.cmpxchgb(reg::ch, reg::ah);

    a.xchgb(reg::ah, reg::rax[-123]);
    a.addb(reg::ah, reg::rax[-123]);
    a.subb(reg::ah, reg::rax[-123]);
    a.andb(reg::ah, reg::rax[-123]);
    a.orb(reg::ah, reg::rax[-123]);
    a.xorb(reg::ah, reg::rax[-123]);
    a.cmpb(reg::ah, reg::rax[-123]);
    a.adcb(reg::ah, reg::rax[-123]);
    a.sbbb(reg::ah, reg::rax[-123]);
    a.movb(reg::ah, reg::rax[-123]);
    a.testb(reg::ah, reg::rax[-123]);
    a.xaddb(reg::ah, reg::rax[-123]);
    a.cmpxchgb(reg::ah, reg::rax[-123]);

    a.xchgb(reg::rax[-123], reg::ah);
    a.addb(reg::rax[-123], reg::ah);
    a.subb(reg::rax[-123], reg::ah);
    a.andb(reg::rax[-123], reg::ah);
    a.orb(reg::rax[-123], reg::ah);
    a.xorb(reg::rax[-123], reg::ah);
    a.cmpb(reg::rax[-123], reg::ah);
    a.adcb(reg::rax[-123], reg::ah);
    a.sbbb(reg::rax[-123], reg::ah);
    a.movb(reg::rax[-123], reg::ah);
    a.testb(reg::rax[-123], reg::ah);

    a.addb(-123, reg::ah);
    a.subb(-123, reg::ah);
    a.andb(-123, reg::ah);
    a.orb(-123, reg::ah);
    a.xorb(-123, reg::ah);
    a.cmpb(-123, reg::ah);
    a.adcb(-123, reg::ah);
    a.sbbb(-123, reg::ah);
    a.movb(-123, reg::ah);
    a.testb(-123, reg::ah);
    a.rolb(7, reg::ah);
    a.rorb(7, reg::ah);
    a.rclb(7, reg::ah);
    a.rcrb(7, reg::ah);
    a.shlb(7, reg::ah);
    a.shrb(7, reg::ah);
    a.sarb(7, reg::ah);

    // ------------------ movzxb, movzxw, movsxb, movsxw, movsxdq, setcc ---------------------

    a.movzxbq(reg::dil, reg::rsi);
    a.movzxbq(reg::dil, reg::r13);

    a.movzxbl(reg::dil, reg::esi);
    a.movzxbl(reg::ah, reg::esi);
    a.movzxbl(reg::dil, reg::r13d);

    a.movzxbw(reg::dil, reg::si);
    a.movzxbw(reg::ah, reg::si);
    a.movzxbw(reg::dil, reg::r13w);

    a.movzxbq(reg::rdi[-123], reg::rsi);

    a.movzxbl(reg::rdi[-123], reg::esi);

    a.movzxbw(reg::rdi[-123], reg::si);

    a.movzxwq(reg::di, reg::rsi);

    a.movzxwl(reg::di, reg::esi);

    a.movzxwq(reg::rdi[-123], reg::rsi);

    a.movzxwl(reg::rdi[-123], reg::esi);

    a.movsxbq(reg::dil, reg::rsi);
    a.movsxbq(reg::dil, reg::r13);

    a.movsxbl(reg::dil, reg::esi);
    a.movsxbl(reg::ah, reg::esi);
    a.movsxbl(reg::dil, reg::r13d);

    a.movsxbw(reg::dil, reg::si);
    a.movsxbw(reg::ah, reg::si);
    a.movsxbw(reg::dil, reg::r13w);

    a.movsxbq(reg::rdi[-123], reg::rsi);

    a.movsxbl(reg::rdi[-123], reg::esi);

    a.movsxbw(reg::rdi[-123], reg::si);

    a.movsxwq(reg::di, reg::rsi);

    a.movsxwl(reg::di, reg::esi);

    a.movsxwq(reg::rdi[-123], reg::rsi);

    a.movsxwl(reg::rdi[-123], reg::esi);

    a.movsxdq(reg::edi, reg::rsi);

    a.movsxdq(reg::rdi[-123], reg::rsi);

    a.setccb(CC_E, reg::dil);
    a.setccb(CC_E, reg::ah);

    // ------------------ xor/mov/and/test special cases ---------------------

    a.xorq(reg::rax, reg::rax);
    a.xorq(reg::r8, reg::r8);

    a.movq(0x000000007fffffffLL, reg::rax);
    a.movq(0x00000000ffffffffLL, reg::rax);
    a.movq(0x000000007fffffffLL, reg::r8);
    a.movq(0x00000000ffffffffLL, reg::r8);
    a.andq(0x000000007fffffffLL, reg::rax);
    a.andq(0x00000000ffffffffLL, reg::rax);
    a.andq(0x000000007fffffffLL, reg::r8);
    a.andq(0x00000000ffffffffLL, reg::r8);
    a.testq(0x000000007fffffffLL, reg::rax);
    a.testq(0x00000000ffffffffLL, reg::rax);
    a.testq(0x000000007fffffffLL, reg::r8);
    a.testq(0x00000000ffffffffLL, reg::r8);

    // -------------------------- mov (byte regs) -----------------------------

    a.movb(-123, reg::cl);
    a.movb(-123, reg::dil);
    a.movb(-123, reg::r12b);
    a.movb(-123, reg::ah);

    a.movb(-123, reg::rax[-123]);

    // -------------------------- add -----------------------------

    a.addq(0x7fffffff, reg::rax);
    a.addl(0x7fffffff, reg::eax);
    a.addw(0x7fff, reg::ax);
    a.addb(-123, reg::al);

    a.addq(0x7fffffff, reg::rcx);
    a.addl(0x7fffffff, reg::ecx);
    a.addw(0x7fff, reg::cx);
    a.addb(-123, reg::cl);

    a.addq(0x7fffffff, reg::rax[-123]);
    a.addl(0x7fffffff, reg::rax[-123]);
    a.addw(0x7fff, reg::rax[-123]);
    a.addb(-123, reg::rax[-123]);

    a.addq(-123, reg::rcx);
    a.addl(-123, reg::ecx);
    a.addw(-123, reg::cx);

    a.addq(-123, reg::rax[-123]);
    a.addl(-123, reg::rax[-123]);
    a.addw(-123, reg::rax[-123]);

    a.addq(reg::rax, reg::rdi);
    a.addl(reg::eax, reg::edi);
    a.addw(reg::ax, reg::di);
    a.addb(reg::al, reg::dil);

    a.addq(reg::rax, reg::rax[-123]);
    a.addl(reg::eax, reg::rax[-123]);
    a.addw(reg::ax, reg::rax[-123]);
    a.addb(reg::al, reg::rax[-123]);

    a.addq(reg::rax[-123], reg::rdi);
    a.addl(reg::rax[-123], reg::edi);
    a.addw(reg::rax[-123], reg::di);
    a.addb(reg::rax[-123], reg::dil);

    // ------------------ xchg special cases ---------------------

    a.xchgq(reg::rax, reg::rcx);
    a.xchgq(reg::rax, reg::r8);
    a.xchgq(reg::rcx, reg::rax);
    a.xchgq(reg::r8, reg::rax);
    a.xchgl(reg::eax, reg::ecx);
    a.xchgl(reg::eax, reg::r8d);
    a.xchgl(reg::ecx, reg::eax);
    a.xchgl(reg::r8d, reg::eax);
    a.xchgw(reg::ax, reg::cx);
    a.xchgw(reg::ax, reg::r8w);
    a.xchgw(reg::cx, reg::ax);
    a.xchgw(reg::r8w, reg::ax);
    a.xchgb(reg::al, reg::cl);
    a.xchgb(reg::al, reg::r8b);
    a.xchgb(reg::cl, reg::al);
    a.xchgb(reg::r8b, reg::al);

    // -------------------------------------------------------

    a.cmpxchg16b(reg::rax[-123]);
    a.cmpxchg8b(reg::rax[-123]);

    a.bswapq(reg::rcx);
    a.bswapl(reg::ecx);

    // -------------------------------------------------------

    printf("Done\n");

    return 0;
}

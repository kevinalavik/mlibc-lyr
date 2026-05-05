#include <bits/syscall.h>

using sc_word_t = __sc_word_t;

extern "C" long __do_syscall_ret(unsigned long ret) {
	return ret;
}

sc_word_t __do_syscall0(long sc) {
	register sc_word_t rax asm("rax") = (sc_word_t)sc;
	asm volatile (
		"syscall"
		: "+r"(rax)
		:
		: "rcx", "r11", "memory"
	);
	return rax;
}

sc_word_t __do_syscall1(long sc, sc_word_t arg1) {
	register sc_word_t rax asm("rax") = (sc_word_t)sc;
	register sc_word_t rdi asm("rdi") = arg1;
	asm volatile (
		"syscall"
		: "+r"(rax)
		: "r"(rdi)
		: "rcx", "r11", "memory"
	);
	return rax;
}

sc_word_t __do_syscall2(long sc, sc_word_t arg1, sc_word_t arg2) {
	register sc_word_t rax asm("rax") = (sc_word_t)sc;
	register sc_word_t rdi asm("rdi") = arg1;
	register sc_word_t rsi asm("rsi") = arg2;
	asm volatile (
		"syscall"
		: "+r"(rax)
		: "r"(rdi), "r"(rsi)
		: "rcx", "r11", "memory"
	);
	return rax;
}

sc_word_t __do_syscall3(long sc, sc_word_t arg1, sc_word_t arg2, sc_word_t arg3) {
	register sc_word_t rax asm("rax") = (sc_word_t)sc;
	register sc_word_t rdi asm("rdi") = arg1;
	register sc_word_t rsi asm("rsi") = arg2;
	register sc_word_t rdx asm("rdx") = arg3;
	asm volatile (
		"syscall"
		: "+r"(rax)
		: "r"(rdi), "r"(rsi), "r"(rdx)
		: "rcx", "r11", "memory"
	);
	return rax;
}

sc_word_t __do_syscall4(long sc,
		sc_word_t arg1, sc_word_t arg2, sc_word_t arg3,
		sc_word_t arg4) {
	register sc_word_t rax asm("rax") = (sc_word_t)sc;
	register sc_word_t rdi asm("rdi") = arg1;
	register sc_word_t rsi asm("rsi") = arg2;
	register sc_word_t rdx asm("rdx") = arg3;
	register sc_word_t r10 asm("r10") = arg4;
	asm volatile (
		"syscall"
		: "+r"(rax)
		: "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10)
		: "rcx", "r11", "memory"
	);
	return rax;
}

sc_word_t __do_syscall5(long sc,
		sc_word_t arg1, sc_word_t arg2, sc_word_t arg3,
		sc_word_t arg4, sc_word_t arg5) {
	register sc_word_t rax asm("rax") = (sc_word_t)sc;
	register sc_word_t rdi asm("rdi") = arg1;
	register sc_word_t rsi asm("rsi") = arg2;
	register sc_word_t rdx asm("rdx") = arg3;
	register sc_word_t r10 asm("r10") = arg4;
	register sc_word_t r8  asm("r8")  = arg5;
	asm volatile (
		"syscall"
		: "+r"(rax)
		: "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8)
		: "rcx", "r11", "memory"
	);
	return rax;
}

sc_word_t __do_syscall6(long sc,
		sc_word_t arg1, sc_word_t arg2, sc_word_t arg3,
		sc_word_t arg4, sc_word_t arg5, sc_word_t arg6) {
	register sc_word_t rax asm("rax") = (sc_word_t)sc;
	register sc_word_t rdi asm("rdi") = arg1;
	register sc_word_t rsi asm("rsi") = arg2;
	register sc_word_t rdx asm("rdx") = arg3;
	register sc_word_t r10 asm("r10") = arg4;
	register sc_word_t r8  asm("r8")  = arg5;
	register sc_word_t r9  asm("r9")  = arg6;
	asm volatile (
		"syscall"
		: "+r"(rax)
		: "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8), "r"(r9)
		: "rcx", "r11", "memory"
	);
	return rax;
}

sc_word_t __do_syscall7(long sc,
		sc_word_t arg1, sc_word_t arg2, sc_word_t arg3,
		sc_word_t arg4, sc_word_t arg5, sc_word_t arg6,
		sc_word_t arg7) {
	register sc_word_t rax asm("rax") = (sc_word_t)sc;
	register sc_word_t rdi asm("rdi") = arg1;
	register sc_word_t rsi asm("rsi") = arg2;
	register sc_word_t rdx asm("rdx") = arg3;
	register sc_word_t r10 asm("r10") = arg4;
	register sc_word_t r8  asm("r8")  = arg5;
	register sc_word_t r9  asm("r9")  = arg6;
	register sc_word_t rbx asm("rbx") = arg7;
	asm volatile (
		"syscall"
		: "+r"(rax)
		: "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8), "r"(r9), "r"(rbx)
		: "rcx", "r11", "memory"
	);
	return rax;
}

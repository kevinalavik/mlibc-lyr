#include "mlibc/tcb.hpp"
#include <abi-bits/errno.h>
#include <abi-bits/fcntl.h>
#include <abi-bits/vm-flags.h>
#include <bits/ensure.h>
#include <bits/syscall.h>
#include <mlibc/all-sysdeps.hpp>
#include <string.h>

enum {
	SYS_READ = 0,
	SYS_WRITE,
	SYS_OPEN,
	SYS_CLOSE,
	SYS_STAT,
	SYS_LSEEK,
	SYS_ACCESS,
	SYS_GETDENTS,
	SYS_EXIT,
	SYS_CHMOD,
	SYS_CHOWN,
	SYS_MKDIR,
	SYS_RMDIR,
	SYS_UNLINK,
	SYS_CHROOT,
	SYS_MOUNT,
	SYS_CHANGE_ROOT,
	SYS_EXECVE,
	SYS_ARCH_PRCTL,
	SYS_MMAP,
	SYS_MUNMAP,
	SYS_MPROTECT,
};

static constexpr long ARCH_SET_FS = 0x1002;

namespace {

int sc_error(long ret) {
	return ret < 0 ? static_cast<int>(-ret) : 0;
}

uint32_t open_flags_to_lyr(int flags) {
	uint32_t out = 0;

	switch (flags & O_ACCMODE) {
	case O_WRONLY:
		out |= 0x0001u;
		break;
	case O_RDWR:
		out |= 0x0002u;
		break;
	default:
		break;
	}

	if (flags & O_CREAT)
		out |= 0x0100u;
	if (flags & O_EXCL)
		out |= 0x0200u;
	if (flags & O_TRUNC)
		out |= 0x0400u;
	if (flags & O_APPEND)
		out |= 0x0800u;
	if (flags & O_DIRECTORY)
		out |= 0x1000u;

	return out;
}

}

namespace mlibc {

void Sysdeps<LibcPanic>::operator()() {
	sysdep<LibcLog>("!!! mlibc panic !!!");
	sysdep<Exit>(-1);
	__builtin_trap();
}

void Sysdeps<LibcLog>::operator()(const char *msg) {
	ssize_t unused;
	sysdep<Write>(1, msg, strlen(msg), &unused);
}

int Sysdeps<Isatty>::operator()(int) {
	return ENOSYS;
}

int Sysdeps<Write>::operator()(int fd, void const *buf, size_t size, ssize_t *ret) {
	auto sc = syscall(SYS_WRITE, fd, buf, size);
	if (int e = sc_error(sc); e)
		return e;
	*ret = sc;
	return 0;
}

int Sysdeps<TcbSet>::operator()(void *pointer) {
	auto sc = syscall(SYS_ARCH_PRCTL, ARCH_SET_FS, pointer);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<AnonAllocate>::operator()(size_t size, void **pointer) {
	return sysdep<VmMap>(nullptr, size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0, pointer);
}

int Sysdeps<AnonFree>::operator()(void *pointer, unsigned long size) {
	return sysdep<VmUnmap>(pointer, size);
}

int Sysdeps<VmProtect>::operator()(void *pointer, size_t size, int prot) {
	auto sc = syscall(SYS_MPROTECT, pointer, size, prot);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Seek>::operator()(int fd, off_t offset, int whence, off_t *new_offset) {
	auto sc = syscall(SYS_LSEEK, fd, offset, whence);
	if (int e = sc_error(sc); e)
		return e;
	*new_offset = sc;
	return 0;
}

void Sysdeps<Exit>::operator()(int status) {
	syscall(SYS_EXIT, status);
	__builtin_trap();
}

int Sysdeps<Close>::operator()(int fd) {
	auto sc = syscall(SYS_CLOSE, fd);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<FutexWake>::operator()(int *, bool) {
	return ENOSYS;
}

int Sysdeps<FutexWait>::operator()(int *, int, timespec const *) {
	return ENOSYS;
}

int Sysdeps<Read>::operator()(int fd, void *buf, unsigned long size, long *bytes_read) {
	auto sc = syscall(SYS_READ, fd, buf, size);
	if (int e = sc_error(sc); e)
		return e;
	*bytes_read = sc;
	return 0;
}

int Sysdeps<Open>::operator()(const char *path, int flags, unsigned int mode, int *fd) {
	auto sc = syscall(SYS_OPEN, path, open_flags_to_lyr(flags), mode);
	if (int e = sc_error(sc); e)
		return e;
	*fd = sc;
	return 0;
}

int Sysdeps<VmMap>::operator()(void *hint, size_t size, int prot, int flags,
		int fd, off_t offset, void **window) {
	auto sc = __do_syscall6(SYS_MMAP, (long)hint, size, prot, flags, fd, offset);
	if (int e = sc_error(sc); e)
		return e;
	*window = reinterpret_cast<void *>(sc);
	return 0;
}

int Sysdeps<VmUnmap>::operator()(void *pointer, size_t size) {
	auto sc = syscall(SYS_MUNMAP, pointer, size);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Execve>::operator()(const char *path, char *const argv[],
		char *const envp[]) {
	auto sc = syscall(SYS_EXECVE, path, argv, envp);
	if (int e = sc_error(sc); e)
		return e;
	__ensure(!"sys_execve() returned unexpectedly");
}

int Sysdeps<ClockGet>::operator()(int, time_t *, long *) {
	return ENOSYS;
}

} // namespace mlibc

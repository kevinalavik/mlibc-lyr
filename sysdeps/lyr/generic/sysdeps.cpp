#include "mlibc/tcb.hpp"
#include <abi-bits/errno.h>
#include <abi-bits/fcntl.h>
#include <abi-bits/gid_t.h>
#include <abi-bits/uid_t.h>
#include <abi-bits/vm-flags.h>
#include <bits/ensure.h>
#include <bits/syscall.h>
#include <mlibc/all-sysdeps.hpp>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <termios.h>
#include <unistd.h>

static constexpr long ARCH_SET_FS = 0x1002;

#ifndef TCGETS
#define TCGETS 0x5401
#endif
#ifndef TCSETS
#define TCSETS 0x5402
#endif
#ifndef TCSETSW
#define TCSETSW 0x5403
#endif
#ifndef TCSETSF
#define TCSETSF 0x5404
#endif
#ifndef TCSBRK
#define TCSBRK 0x5409
#endif
#ifndef TCXONC
#define TCXONC 0x540A
#endif
#ifndef TCFLSH
#define TCFLSH 0x540B
#endif
#ifndef TIOCGWINSZ
#define TIOCGWINSZ 0x5413
#endif
#ifndef TIOCSWINSZ
#define TIOCSWINSZ 0x5414
#endif

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif
#ifndef AT_REMOVEDIR
#define AT_REMOVEDIR 0x200
#endif

namespace {

int sc_error(long ret) { return ret < 0 ? static_cast<int>(-ret) : 0; }

struct lyr_sigaction {
	uint64_t handler;
	uint64_t flags;
	uint64_t restorer;
	uint64_t mask;
};

static uint64_t sigset_to_u64(const sigset_t *set) {
	if (!set)
		return 0;

	uint64_t out = 0;
	const unsigned char *p = reinterpret_cast<const unsigned char *>(set);

	size_t n = sizeof(sigset_t) < sizeof(out) ? sizeof(sigset_t) : sizeof(out);
	for (size_t i = 0; i < n; i++)
		out |= static_cast<uint64_t>(p[i]) << (i * 8);

	return out;
}

static void u64_to_sigset(uint64_t value, sigset_t *set) {
	if (!set)
		return;

	memset(set, 0, sizeof(*set));

	unsigned char *p = reinterpret_cast<unsigned char *>(set);
	size_t n = sizeof(sigset_t) < sizeof(value) ? sizeof(sigset_t) : sizeof(value);

	for (size_t i = 0; i < n; i++)
		p[i] = static_cast<unsigned char>((value >> (i * 8)) & 0xff);
}

__attribute__((naked)) void __lyr_sigreturn_trampoline(void) {
	asm volatile("mov %[nr], %%rax\n\t"
	             "syscall\n\t"
	             "hlt\n\t"
	             :
	             : [nr] "i"(SYS_SIGRETURN)
	             : "rax", "rcx", "r11", "memory");
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

} // namespace

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

int Sysdeps<Isatty>::operator()(int fd) {
	struct termios attr;
	auto sc_ret = syscall(SYS_IOCTL, fd, TCGETS, &attr);
	if (int e = sc_error(sc_ret); e)
		return e;
	return 0;
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
	return sysdep<VmMap>(
	    nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0, pointer
	);
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

int Sysdeps<FutexWake>::operator()(int *, bool) { return ENOSYS; }

int Sysdeps<FutexWait>::operator()(int *, int, timespec const *) { return ENOSYS; }

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

int Sysdeps<VmMap>::operator()(
    void *hint, size_t size, int prot, int flags, int fd, off_t offset, void **window
) {
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

int Sysdeps<Execve>::operator()(const char *path, char *const argv[], char *const envp[]) {
	auto sc = syscall(SYS_EXECVE, path, argv, envp);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<ClockGet>::operator()(int clock, time_t *secs, long *nanos) {
	auto sc_ret = syscall(SYS_CLOCK_GET, clock, secs, nanos);
	if (int e = sc_error(sc_ret); e)
		return e;
	return 0;
}

int Sysdeps<OpenDir>::operator()(const char *path, int *handle) {
	return sysdep<Open>(path, O_RDONLY | O_DIRECTORY, 0, handle);
}

int Sysdeps<ReadEntries>::operator()(
    int handle, void *buffer, size_t max_size, size_t *bytes_read
) {
	auto sc = syscall(SYS_GETDENTS, handle, buffer, max_size);
	if (int e = sc_error(sc); e)
		return e;
	*bytes_read = sc;
	return 0;
}

int Sysdeps<Stat>::operator()(
    fsfd_target fsfdt, int fd, const char *path, int flags, struct stat *statbuf
) {
	(void)flags;

	switch (fsfdt) {
		case fsfd_target::path:
			break;
		case fsfd_target::fd_path:
			if (fd != AT_FDCWD)
				return ENOSYS;
			break;
		case fsfd_target::fd:
		default:
			return ENOSYS;
	}

	auto sc = syscall(SYS_STAT, path, statbuf);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Access>::operator()(const char *path, int mode) {
	auto sc = syscall(SYS_ACCESS, path, mode);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Faccessat>::operator()(int dirfd, const char *pathname, int mode, int flags) {
	if (dirfd != AT_FDCWD || flags)
		return ENOSYS;
	return sysdep<Access>(pathname, mode);
}

int Sysdeps<Chmod>::operator()(const char *pathname, mode_t mode) {
	auto sc = syscall(SYS_CHMOD, pathname, mode);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Fchmodat>::operator()(int fd, const char *pathname, mode_t mode, int flags) {
	if (fd != AT_FDCWD || flags)
		return ENOSYS;
	return sysdep<Chmod>(pathname, mode);
}

int Sysdeps<Fchmod>::operator()(int fd, mode_t mode) {
	(void)fd;
	(void)mode;
	return ENOSYS;
}

int Sysdeps<Fchownat>::operator()(
    int dirfd, const char *pathname, uid_t owner, gid_t group, int flags
) {
	if (dirfd != AT_FDCWD || flags)
		return ENOSYS;

	auto sc = syscall(SYS_CHOWN, pathname, owner, group);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Mkdir>::operator()(const char *path, mode_t mode) {
	auto sc = syscall(SYS_MKDIR, path, mode);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Mkdirat>::operator()(int dirfd, const char *path, mode_t mode) {
	if (dirfd != AT_FDCWD)
		return ENOSYS;
	return sysdep<Mkdir>(path, mode);
}

int Sysdeps<Rmdir>::operator()(const char *path) {
	auto sc = syscall(SYS_RMDIR, path);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Unlinkat>::operator()(int fd, const char *path, int flags) {
	if (fd != AT_FDCWD)
		return ENOSYS;

	if (flags & AT_REMOVEDIR)
		return sysdep<Rmdir>(path);

	if (flags)
		return ENOSYS;

	auto sc = syscall(SYS_UNLINK, path);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Ioctl>::operator()(int fd, unsigned long request, void *arg, int *result) {
	auto sc = syscall(SYS_IOCTL, fd, request, arg);
	if (int e = sc_error(sc); e)
		return e;
	*result = sc;
	return 0;
}

int Sysdeps<Tcgetattr>::operator()(int fd, struct termios *attr) {
	int result;
	return sysdep<Ioctl>(fd, TCGETS, attr, &result);
}

int Sysdeps<Tcsetattr>::operator()(int fd, int act, const struct termios *attr) {
	unsigned long req;

	switch (act) {
		case TCSANOW:
			req = TCSETS;
			break;
		case TCSADRAIN:
			req = TCSETSW;
			break;
		case TCSAFLUSH:
			req = TCSETSF;
			break;
		default:
			return EINVAL;
	}

	int result;
	return sysdep<Ioctl>(fd, req, const_cast<struct termios *>(attr), &result);
}

int Sysdeps<Chroot>::operator()(const char *path) {
	auto sc = syscall(SYS_CHROOT, path);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<MsgSend>::operator()(int fd, const struct msghdr *hdr, int flags, ssize_t *length) {
	if (!hdr || hdr->msg_iovlen != 1)
		return ENOSYS;

	const auto &iov = hdr->msg_iov[0];

	long sc;
	if (hdr->msg_name) {
		sc = syscall(
		    SYS_SENDTO, fd, iov.iov_base, iov.iov_len, flags, hdr->msg_name, hdr->msg_namelen
		);
	} else {
		sc = syscall(SYS_SEND, fd, iov.iov_base, iov.iov_len, flags);
	}

	if (int e = sc_error(sc); e)
		return e;

	*length = sc;
	return 0;
}

int Sysdeps<MsgRecv>::operator()(int fd, struct msghdr *hdr, int flags, ssize_t *length) {
	if (!hdr || hdr->msg_iovlen != 1)
		return ENOSYS;

	auto &iov = hdr->msg_iov[0];

	long sc;
	if (hdr->msg_name) {
		sc = syscall(
		    SYS_RECVFROM, fd, iov.iov_base, iov.iov_len, flags, hdr->msg_name, &hdr->msg_namelen
		);
	} else {
		sc = syscall(SYS_RECV, fd, iov.iov_base, iov.iov_len, flags);
	}

	if (int e = sc_error(sc); e)
		return e;

	*length = sc;
	return 0;
}

int Sysdeps<Socket>::operator()(int family, int type, int protocol, int *fd) {
	auto sc = syscall(SYS_SOCKET, family, type, protocol);
	if (int e = sc_error(sc); e)
		return e;
	*fd = sc;
	return 0;
}

int Sysdeps<Bind>::operator()(int fd, const struct sockaddr *addr_ptr, socklen_t addr_length) {
	auto sc = syscall(SYS_BIND, fd, addr_ptr, addr_length);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Connect>::operator()(int fd, const struct sockaddr *addr_ptr, socklen_t addr_length) {
	auto sc = syscall(SYS_CONNECT, fd, addr_ptr, addr_length);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Listen>::operator()(int fd, int backlog) {
	auto sc = syscall(SYS_LISTEN, fd, backlog);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Accept>::operator()(
    int fd, int *newfd, struct sockaddr *addr_ptr, socklen_t *addr_length, int flags
) {
	if (flags)
		return ENOSYS;

	auto sc = syscall(SYS_ACCEPT, fd, addr_ptr, addr_length);
	if (int e = sc_error(sc); e)
		return e;

	*newfd = sc;
	return 0;
}

int Sysdeps<Sockname>::operator()(
    int fd, struct sockaddr *addr_ptr, socklen_t max_addr_length, socklen_t *actual_length
) {
	socklen_t length = max_addr_length;

	auto sc = syscall(SYS_GETSOCKNAME, fd, addr_ptr, &length);
	if (int e = sc_error(sc); e)
		return e;

	*actual_length = length;
	return 0;
}

int Sysdeps<Peername>::operator()(
    int fd, struct sockaddr *addr_ptr, socklen_t max_addr_length, socklen_t *actual_length
) {
	socklen_t length = max_addr_length;

	auto sc = syscall(SYS_GETPEERNAME, fd, addr_ptr, &length);
	if (int e = sc_error(sc); e)
		return e;

	*actual_length = length;
	return 0;
}

int Sysdeps<Shutdown>::operator()(int fd, int how) {
	auto sc = syscall(SYS_SHUTDOWN, fd, how);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<SetSockopt>::operator()(
    int fd, int layer, int number, const void *buffer, socklen_t size
) {
	auto sc = syscall(SYS_SETSOCKOPT, fd, layer, number, buffer, size);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<GetSockopt>::operator()(int fd, int layer, int number, void *buffer, socklen_t *size) {
	auto sc = syscall(SYS_GETSOCKOPT, fd, layer, number, buffer, size);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Poll>::operator()(struct pollfd *fds, nfds_t nfds, int timeout, int *num_events) {
	auto ret = syscall(SYS_POLL, fds, nfds, timeout);

	if (ret < 0)
		return -ret;

	*num_events = ret;
	return 0;
}

int Sysdeps<Fork>::operator()(pid_t *child) {
	auto sc_ret = syscall(SYS_FORK);
	if (int e = sc_error(sc_ret); e)
		return e;
	if (child)
		*child = static_cast<pid_t>(sc_ret);
	return 0;
}

pid_t Sysdeps<GetPid>::operator()() {
	auto sc_ret = syscall(SYS_GETPID);
	if (int e = sc_error(sc_ret); e)
		return (pid_t)-e;
	return static_cast<pid_t>(sc_ret);
}

uid_t Sysdeps<GetUid>::operator()() {
	auto sc_ret = syscall(SYS_GETUID);
	if (int e = sc_error(sc_ret); e)
		return (uid_t)-1;
	return static_cast<uid_t>(sc_ret);
}

uid_t Sysdeps<GetEuid>::operator()() {
	auto sc_ret = syscall(SYS_GETEUID);
	if (int e = sc_error(sc_ret); e)
		return (uid_t)-1;
	return static_cast<uid_t>(sc_ret);
}

gid_t Sysdeps<GetGid>::operator()() {
	auto sc_ret = syscall(SYS_GETGID);
	if (int e = sc_error(sc_ret); e)
		return (gid_t)-1;
	return static_cast<gid_t>(sc_ret);
}

gid_t Sysdeps<GetEgid>::operator()() {
	auto sc_ret = syscall(SYS_GETEGID);
	if (int e = sc_error(sc_ret); e)
		return (gid_t)-1;
	return static_cast<gid_t>(sc_ret);
}

int Sysdeps<SetUid>::operator()(uid_t uid) {
	auto sc = syscall(SYS_SETUID, uid);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<SetEuid>::operator()(uid_t euid) {
	auto sc = syscall(SYS_SETEUID, euid);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<SetGid>::operator()(gid_t gid) {
	auto sc = syscall(SYS_SETGID, gid);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<SetEgid>::operator()(gid_t egid) {
	auto sc = syscall(SYS_SETEGID, egid);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

pid_t Sysdeps<GetTid>::operator()() {
	auto sc_ret = syscall(SYS_GETTID);
	if (int e = sc_error(sc_ret); e)
		return (pid_t)-e;
	return static_cast<pid_t>(sc_ret);
}

pid_t Sysdeps<GetPpid>::operator()() {
	auto sc_ret = syscall(SYS_GETPPID);
	if (int e = sc_error(sc_ret); e)
		return (pid_t)-e;
	return static_cast<pid_t>(sc_ret);
}

int Sysdeps<Chdir>::operator()(const char *path) {
	auto sc = syscall(SYS_CHDIR, path);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<GetCwd>::operator()(char *buffer, size_t size) {
	auto sc = syscall(SYS_GETCWD, buffer, size);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Pselect>::operator()(
    int num_fds,
    fd_set *read_set,
    fd_set *write_set,
    fd_set *except_set,
    const struct timespec *timeout,
    const sigset_t *sigmask,
    int *num_events
) {
	auto sc = syscall(SYS_PSELECT, num_fds, read_set, write_set, except_set, timeout, sigmask);

	if (int e = sc_error(sc); e)
		return e;

	if (num_events)
		*num_events = static_cast<int>(sc);

	return 0;
}

int Sysdeps<Fcntl>::operator()(int fd, int request, va_list args, int *result) {
	long arg = 0;

	switch (request) {
		case F_DUPFD:
		case F_DUPFD_CLOEXEC:
		case F_SETFD:
		case F_SETFL:
			arg = va_arg(args, long);
			break;
		default:
			arg = 0;
			break;
	}

	auto sc = syscall(SYS_FCNTL, fd, request, arg);
	if (int e = sc_error(sc); e)
		return e;

	*result = static_cast<int>(sc);
	return 0;
}

int Sysdeps<Waitpid>::operator()(
    pid_t pid, int *status, int flags, struct rusage *ru, pid_t *ret_pid
) {
	if (ru)
		return ENOSYS;

	auto sc = syscall(SYS_WAITPID, pid, status, flags);
	if (int e = sc_error(sc); e)
		return e;

	if (ret_pid)
		*ret_pid = static_cast<pid_t>(sc);

	return 0;
}

int Sysdeps<Sleep>::operator()(time_t *secs, long *nanos) {
	if (!secs || !nanos)
		return EINVAL;

	auto sc = syscall(SYS_NSLEEP, (long)*secs, (long)*nanos);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Ttyname>::operator()(int fd, char *buffer, size_t size) {
#define TTY_IOCTL_NAME 0x4c590001UL
#define TTY_NAME_MAX 32
#define TTY_PREFIX "/dev/"

	const char prefix[] = TTY_PREFIX;
	constexpr size_t prefix_len = sizeof(prefix) - 1;

	if (!buffer)
		return EFAULT;

	if (size < prefix_len + TTY_NAME_MAX + 1)
		return ENAMETOOLONG;

	memcpy(buffer, prefix, prefix_len);

	int res;
	int e = sysdep<Ioctl>(fd, TTY_IOCTL_NAME, buffer + prefix_len, &res);
	if (e)
		return e;

	buffer[size - 1] = '\0';
	return 0;
}

int Sysdeps<Uname>::operator()(struct utsname *buf) {
	auto sc = syscall(SYS_UNAME, buf);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Kill>::operator()(pid_t pid, int signal) {
	auto sc = syscall(SYS_KILL, pid, signal);
	if (int e = sc_error(sc); e)
		return e;
	return 0;
}

int Sysdeps<Sigprocmask>::operator()(
    int how, const sigset_t *__restrict set, sigset_t *__restrict retrieve
) {
	uint64_t raw_set = sigset_to_u64(set);
	uint64_t raw_old = 0;

	auto sc =
	    syscall(SYS_SIGPROCMASK, how, set ? &raw_set : nullptr, retrieve ? &raw_old : nullptr);

	if (int e = sc_error(sc); e)
		return e;

	if (retrieve)
		u64_to_sigset(raw_old, retrieve);

	return 0;
}

int Sysdeps<Sigaction>::operator()(
    int sig, const struct sigaction *__restrict act, struct sigaction *__restrict oldact
) {
	lyr_sigaction kact;
	lyr_sigaction kold;

	lyr_sigaction *kactp = nullptr;
	lyr_sigaction *koldp = oldact ? &kold : nullptr;

	if (act) {
		memset(&kact, 0, sizeof(kact));

		kact.handler = reinterpret_cast<uint64_t>(act->sa_handler);
		kact.flags = static_cast<uint64_t>(act->sa_flags);
		kact.restorer = reinterpret_cast<uint64_t>(&__lyr_sigreturn_trampoline);
		kact.mask = sigset_to_u64(&act->sa_mask);

		kactp = &kact;
	}

	auto sc = syscall(SYS_SIGACTION, sig, kactp, koldp);

	if (int e = sc_error(sc); e)
		return e;

	if (oldact) {
		memset(oldact, 0, sizeof(*oldact));

		oldact->sa_handler = reinterpret_cast<void (*)(int)>(kold.handler);
		oldact->sa_flags = static_cast<int>(kold.flags);
		u64_to_sigset(kold.mask, &oldact->sa_mask);
	}

	return 0;
}

} // namespace mlibc

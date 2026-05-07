#include <lyr/mount.h>

#include <bits/syscall.h>
#include <errno.h>

extern "C" {

int mount(
    const char *source,
    const char *target,
    const char *fstype,
    unsigned long mntflags,
    const void *data
) {
	auto sc = syscall(SYS_MOUNT, source, target, fstype, mntflags, data);

	if (sc < 0) {
		errno = static_cast<int>(-sc);
		return -1;
	}

	return 0;
}
}

#ifndef _LYR_MOUNT_H
#define _LYR_MOUNT_H

#ifdef __cplusplus
extern "C" {
#endif

int mount(
    const char *source,
    const char *target,
    const char *fstype,
    unsigned long mntflags,
    const void *data
);

#ifdef __cplusplus
}
#endif

#endif // _LYR_MOUNT_H

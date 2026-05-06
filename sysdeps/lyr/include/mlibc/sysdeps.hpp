#pragma once

#include <mlibc/sysdep-signatures.hpp>

namespace mlibc {

struct LyrSysdepsTags : LibcPanic,
                        LibcLog,
                        Isatty,
                        Write,
                        TcbSet,
                        AnonAllocate,
                        AnonFree,
                        VmProtect,
                        Seek,
                        Exit,
                        Close,
                        FutexWake,
                        FutexWait,
                        Read,
                        Open,
                        OpenDir,
                        ReadEntries,
                        Stat,
                        Access,
                        Faccessat,
                        Chmod,
                        Fchmod,
                        Fchmodat,
                        Fchownat,
                        Mkdir,
                        Mkdirat,
                        Rmdir,
                        Unlinkat,
                        Ioctl,
                        Tcgetattr,
                        Tcsetattr,
                        Chroot,
                        Socket,
                        Bind,
                        Connect,
                        Listen,
                        Accept,
                        Sockname,
                        Peername,
                        Shutdown,
                        SetSockopt,
                        GetSockopt,
                        MsgSend,
                        MsgRecv,
                        VmMap,
                        VmUnmap,
                        Execve,
                        ClockGet {};

template <typename Tag>
using Sysdeps = SysdepOf<LyrSysdepsTags, Tag>;

} // namespace mlibc
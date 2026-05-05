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
                          Seek,
                          Exit,
                          Close,
                          FutexWake,
                          FutexWait,
                          Read,
                          Open,
                          VmMap,
                          VmUnmap,
                          ClockGet {};

template <typename Tag>
using Sysdeps = SysdepOf<LyrSysdepsTags, Tag>;

} // namespace mlibc

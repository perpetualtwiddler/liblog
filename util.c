#ifdef _WIN32_WINNT
#include <windows.h>
#include <stdio.h>
#include "util.h"

#define DELTA_EPOCH_IN_MICROSECS  116444736000000000Ui64

i4b
gettimeofday(struct timeval * tv, struct timezone * /* tzp */)
{
    FILETIME file_time;
    ULARGE_INTEGER ul;

    GetSystemTimeAsFileTime(&file_time);
    ul.LowPart = file_time.dwLowDateTime;
    ul.HighPart = file_time.dwHighDateTime;

    tv->tv_sec = (long) ((ul.QuadPart - DELTA_EPOCH_IN_MICROSECS) / 10000000L);
    tv->tv_usec = (long) (ul.QuadPart % 10000000) / 10;
    return 0;
}

i4b
llvsnprintf(char * buf, size_t cnt, const char * fmt, va_list ap)
{
    i4b rv = _vsnprintf(buf, cnt, fmt, ap);
    return (rv < 0) ? cnt : rv;
}

#endif  /* _WIN32_WINNT for gettimeofday() */

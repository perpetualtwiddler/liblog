
#include <stdio.h>
#include <stdint.h>
#include <time.h>

typedef int32_t i4b;
typedef uint32_t u4b;
typedef int64_t i8b;
typedef uint64_t u8b;

typedef enum LogLevel_ {
    LL_UNUSED = 0,
    LL_USR,                     // Information for the user, an error or warning
    LL_SPT,                     // Information for support
    LL_DEV                      // Useful for the developer
} LogLevel;

typedef struct LogHandle_ {
    i4b lhfd;
    time_t lhfdopents;          // for rollover?
} LogHandle;

enum LogLibError {
    LLE_SUCCESS,
    LLE_INVALIDLOGLEVEL,
    LLE_NOLOGDIR,
    LLE_NOPERMLOGDIR,
    LLE_DATEFMT,
    LLE_SNPRINTF_FAIL,
    LLE_LOGFILEPATHTOOLONG,
    LLE_GETTIMEOFDAY,
    LLE_LOGWRITE_FAIL,
    LLE_LASTPLUSONE
};

#ifndef _WIN32_WINNT
#define PATH_SEP "/"
#else  // _WIN32_WINNT
#define PATH_SEP "\\"
#endif  // _WIN32_WINNT

i4b logopen(const char * dir, const char * fnpfx, LogHandle ** lhpp);

i4b logwrite(LogHandle * lhp, const char * fn, const char * func,
             i4b ln, LogLevel ll, const char * fmt, ...)
    __attribute__ ((format (printf, 6, 7)));

i4b logclose(LogHandle ** lhpp);

#define LDEV(lhp, fmt, ...) logwrite(lhp, __FILE__, __func__, __LINE__, \
                                     LL_DEV, fmt, __VA_ARGS__)

#define LSPT(lhp, fmt, ...) logwrite(lhp, __FILE__, __func__, __LINE__, \
                                     LL_SPT, fmt, __VA_ARGS__)

#define LUSR(lhp, fmt, ...) logwrite(lhp, __FILE__, __func__, __LINE__, \
                                     LL_USR, fmt, __VA_ARGS__)



// #ifdef _LOG_COMPACT_FMT_SPEC_
// // Assume POSIX
// #define D4 PRId32
// #define D8 PRId64
// #define U4 PRIu32
// #define U8 PRIu64

// #endif  // _LOG_COMPACT_FMT_SPEC_ 


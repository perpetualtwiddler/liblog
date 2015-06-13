#ifndef _LOG_HEADER_INCLUDED_
#define _LOG_HEADER_INCLUDED_

#include <stdio.h>
#ifdef _WIN32_WINNT
#include <windows.h>
#include <winbase.h>
#define __func__ __FUNCTION__
#else
#include <pthread.h>            /* for pthread_*() */
#endif
#include "util.h"

typedef enum LogLevel_ {
    LL_NUL = 0,                 // Logging disabled
    LL_USR,                     // Information for user: error or warning
    LL_SPT,                     // Information for support
    LL_DEV                      // Useful for the developer
} LogLevel;

typedef struct LogHandle_ {
    i4b lhfd;
    i1b hour;                  // for rollover?
    i1b mday;                   // for rollover?
    i1b autorollover;
    i1b log_level;
    char ldir[256];
    char lfnpfx[64];
#ifdef _WIN32_WINNT
    CRITICAL_SECTION mutex;
#else
    pthread_mutex_t mutex;
#endif
} LogHandle;

enum LogLibError {
    LLE_SUCCESS,
    LLE_INVALIDLOGLEVEL,
    LLE_NOLOGDIR,
    LLE_NOPERMLOGDIR,
    LLE_LOGFILEPATHTOOLONG,
    LLE_GETTIMEOFDAY,
    LLE_LOGWRITE_FAIL,
    LLE_LOGCLOSE_FAIL,
    LLE_LASTPLUSONE
};

i4b log_open(const char * dir, const char * fnpfx, LogHandle ** lhpp);

i4b log_write(LogHandle * lhp, const char * fn, const char * func,
              i4b ln, LogLevel ll, const char * fmt, ...)
#ifndef _WIN32_WINNT
    __attribute__ ((format (printf, 6, 7)))
#endif
;

i4b log_close(LogHandle ** lhpp);

void log_set_loglevel(LogHandle * lhp, i4b ll);
i4b log_get_loglevel(const LogHandle * lhp);

void log_disable_autorollover(LogHandle * lhp);
void log_enable_autorollover(LogHandle * lhp);
i4b log_get_autorollover(const LogHandle * lhp);

#define LDEV(lhp, fmt, ...) log_write(lhp, __FILE__, __func__, __LINE__, \
                                      LL_DEV, fmt, ##__VA_ARGS__)

#define LSPT(lhp, fmt, ...) log_write(lhp, __FILE__, __func__, __LINE__, \
                                      LL_SPT, fmt, ##__VA_ARGS__)

#define LUSR(lhp, fmt, ...) log_write(lhp, __FILE__, __func__, __LINE__, \
                                      LL_USR, fmt, ##__VA_ARGS__)
#endif // _LOG_HEADER_INCLUDED_

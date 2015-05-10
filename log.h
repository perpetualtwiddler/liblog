
#ifndef _LOG_HEADER_INCLUDED_
#define _LOG_HEADER_INCLUDED_

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>            /* for pthread_*() */

typedef int32_t i4b;
typedef uint32_t u4b;
typedef int64_t i8b;
typedef uint64_t u8b;

typedef enum LogLevel_ {
    LL_NUL = 0,                 // Logging disabled
    LL_USR,                     // Information for the user, an error or warning
    LL_SPT,                     // Information for support
    LL_DEV                      // Useful for the developer
} LogLevel;

typedef struct LogHandle_ {
    i4b lhfd;
    i4b mday;          // for rollover?
    char ldir[256];
    char lfnpfx[64];
    pthread_mutex_t mutex;
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
    __attribute__ ((format (printf, 6, 7)));

i4b log_close(LogHandle ** lhpp);

void log_set_logginglevel(i4b ll);
i4b log_get_logginglevel(void);

void log_disable_autorollover(void);
void log_enable_autorollover(void);
int32_t log_get_autorollover(void);

#define LDEV(lhp, fmt, ...) log_write(lhp, __FILE__, __func__, __LINE__, \
                                      LL_DEV, fmt, __VA_ARGS__)

#define LSPT(lhp, fmt, ...) log_write(lhp, __FILE__, __func__, __LINE__, \
                                      LL_SPT, fmt, __VA_ARGS__)

#define LUSR(lhp, fmt, ...) log_write(lhp, __FILE__, __func__, __LINE__, \
                                      LL_USR, fmt, __VA_ARGS__)
#endif // _LOG_HEADER_INCLUDED_

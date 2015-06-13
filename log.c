#include "util.h"
#include "log.h"

#ifndef _WIN32_WINNT
#include <unistd.h>             /* for access(), write() */
#include <sys/time.h>           /* for gettimeofday() */
#include <inttypes.h>           /* for PRI* format specifiers */
#else
#include <io.h>                 /* for _access() */
#include <process.h>            /* for _getpid() */
#endif                          /* _WIN32_WINNT */

#include <time.h>               /* for time(), strftime(), localtime_r() */
#include <sys/types.h>          /* for open() */
#include <sys/stat.h>           /* for open() */
#include <fcntl.h>              /* for open() */
#include <errno.h>              /* for errno */
#include <stdlib.h>             /* for malloc() */
#include <stdarg.h>             /* for vsnprintf() */
#include <string.h>             /* for strncpy() */

#ifdef _WIN32_WINNT
#define F_OK 0
#define W_OK 2
#define R_OK 4
#define access _access
#define open _open
#define close _close
#define write _write
#define getpid _getpid
#define snprintf _snprintf
#define localtime_r(a,b)        localtime_s((b),(a))

#define INIT_MUTEX InitializeCriticalSection
#define LOCK_MUTEX EnterCriticalSection
#define UNLOCK_MUTEX LeaveCriticalSection
#define THREAD_ID GetCurrentThreadId

#define D8 "%I64d"
#define U8 "%I64u"
#define PATH_SEP "\\"

#else  /* _WIN32_WINNT */

#define llvsnprintf vsnprintf
#define INIT_MUTEX(a) pthread_mutex_init((a), NULL)
#define LOCK_MUTEX pthread_mutex_lock
#define UNLOCK_MUTEX pthread_mutex_lock
#define THREAD_ID pthread_self

#define D8 "%" PRId64
#define U8 "%" PRIu64
#define PATH_SEP "/"

#endif

#define STRLCPY(d,s,dsz)        \
    strncpy((d), (s), (dsz));   \
    (d)[(dsz)-1] = '\0';

void
log_set_loglevel(LogHandle * lhp, i4b ll)
{
    lhp->log_level = ll;
}

i4b
log_get_loglevel(const LogHandle * lhp)
{
    return lhp->log_level;
}

void
log_disable_autorollover(LogHandle * lhp)
{
    lhp->autorollover = 0;
}

void
log_enable_autorollover(LogHandle * lhp)
{
    lhp->autorollover = 1;
}

i4b
log_get_autorollover(const LogHandle * lhp)
{
    return lhp->autorollover;
}

static i4b
getdatestring(char * buf, u8b bsz, i1b * mday, i1b * hour)
{
    time_t now = time(NULL);
    struct tm tms = { 0 };
    localtime_r(&now, &tms);
    strftime(buf, (size_t) bsz, "%Y-%m-%d", &tms);
    *mday = tms.tm_mday;
    *hour = tms.tm_hour;
    return LLE_SUCCESS;
}

static void
init_log_handle(LogHandle * lhp, const char * dir,
                const char * fnpfx, i1b autoro, i1b ll)
{
    lhp->autorollover = autoro;
    lhp->log_level = ll;
    STRLCPY(lhp->ldir, dir, sizeof(lhp->ldir));
    STRLCPY(lhp->lfnpfx, fnpfx, sizeof(lhp->lfnpfx));
    INIT_MUTEX(&lhp->mutex);
}

i4b
log_open(const char * dir, const char * fnpfx, LogHandle ** lhpp)
{
    i4b rv = LLE_SUCCESS;
    LogHandle * lhp = NULL;
    do {
        if (access(dir, F_OK) != 0) {
            rv = LLE_NOLOGDIR;                  break;
        }
        if (access(dir, R_OK | W_OK) != 0) {
            rv = LLE_NOPERMLOGDIR;              break;
        }
        char dbuf[16];
        i1b mday = 0;
        i1b hour = -1;
        getdatestring(dbuf, sizeof(dbuf), &mday, &hour);
        char fpath[321];
        i4b psz = snprintf(fpath, sizeof(fpath), "%s%s%s-%s.log",
                           dir, PATH_SEP, fnpfx, dbuf);
        if (psz >= (i4b) sizeof(fpath)) {
            rv = LLE_LOGFILEPATHTOOLONG;        break;
        }
        lhp = (*lhpp) ? (*lhpp) : (LogHandle *) malloc(sizeof(LogHandle));
        i4b lfd = open(fpath, O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (lfd < 0) {
            rv = LLE_LASTPLUSONE + errno;       break;
        }
        if (*lhpp) {
            if (close(lhp->lhfd) < 0) {
                char tb[2048];
                snprintf(tb, sizeof(tb),
                         "Log close failure ignored: OLDFD=%d ERRNO=%d LLE=%d",
                         lhp->lhfd, errno, LLE_LOGCLOSE_FAIL);
                write(lfd, tb, strlen(tb));
            }
        }
        lhp->lhfd = lfd;
        lhp->mday = mday;
        lhp->hour = hour;
        if (*lhpp == NULL) {
            init_log_handle(lhp, dir, fnpfx, 1, LL_DEV);
            *lhpp = lhp;
        }
    } while (0);
    if (rv != LLE_SUCCESS) {
        free(lhp);
    }
    return rv;
}

static i4b
log_rollover(LogHandle * lhp)
{
    return log_open(lhp->ldir, lhp->lfnpfx, &lhp);
}

i4b
is_autorollover_nearly_due(const LogHandle * lhp, const struct tm * tms)
{
    if (log_get_autorollover(lhp) == 0)
        return 0;
    if (tms->tm_mday != lhp->mday)
        return 1;
    return ((tms->tm_hour == 23) &&
            (tms->tm_min == 59) &&
            (tms->tm_sec > 57)) ? 1 : 0;
}

i4b
is_autorollover_due(const LogHandle * lhp, const struct tm * tms)
{
    if (log_get_autorollover(lhp) == 0)
        return 0;
    return (tms->tm_mday != lhp->mday) ? 1 : 0;
}

i4b
log_write(LogHandle * lhp, const char * fn, const char * func,
          i4b ln, LogLevel ll, const char * fmt, ...)
{
    if (!lhp || (log_get_loglevel(lhp) < ll)) {
        return LLE_SUCCESS;
    }
    if (!ll || (ll > LL_DEV)) {
        return LLE_INVALIDLOGLEVEL;
    }
    i4b rv = LLE_SUCCESS;
    u4b lockacquired = 0;
    do {
        struct timeval tv = { 0 };
        if (gettimeofday(&tv, NULL) != 0) {
            rv = LLE_GETTIMEOFDAY;              break;
        }
        struct tm tms = { 0 };
        time_t ts = tv.tv_sec;
        localtime_r(&ts, &tms);

        char lbuf[2048];
        u8b cnt = strftime(lbuf, sizeof(lbuf), "%H:%M:%S", &tms);

        char llc = (ll == LL_DEV) ? 'D' : ((ll == LL_SPT) ? 'S' : 'U');

        cnt += snprintf(lbuf+cnt, sizeof(lbuf) - (size_t) cnt,
                        ".%06d %c %d:" U8 " [%s:%d:%s] ",
                        (i4b) tv.tv_usec, llc, getpid(), (u8b) THREAD_ID(),
                        fn, ln, func);

        va_list ap;
        va_start(ap, fmt);
        cnt += llvsnprintf(lbuf+cnt, sizeof(lbuf) - (size_t) cnt, fmt, ap);
        va_end(ap);
        if (cnt >= (sizeof(lbuf))) {
            cnt = sizeof(lbuf) - 1;
        }
        lbuf[cnt++] = '\n';     /* replace trailing '\0', can't use strlen() */

        if (is_autorollover_nearly_due(lhp, &tms)) {
            LOCK_MUTEX(&lhp->mutex);
            lockacquired = 1;
            if (is_autorollover_due(lhp, &tms)) {
                /* Ignore potential failures from log_rollover() for now,
                   we'll effectively continue writing in the same log file
                   */
                log_rollover(lhp);
            }
        }
        i8b wcnt = write(lhp->lhfd, lbuf, (size_t) cnt);
        if (cnt != (u8b) wcnt) {
            rv = LLE_LOGWRITE_FAIL;
            break;
        }
    } while (0);
    if (lockacquired) {
        UNLOCK_MUTEX(&lhp->mutex);
        lockacquired = 0;
    }
    return rv;
}

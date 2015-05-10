#include "log.h"
#include <unistd.h>             /* for access(), write() */
#include <time.h>               /* for time(), strftime(), localtime_r() */
#include <sys/time.h>           /* for gettimeofday() */
#include <sys/types.h>          /* for open() */
#include <sys/stat.h>           /* for open() */
#include <fcntl.h>              /* for open() */
#include <errno.h>              /* for errno */
#include <stdlib.h>             /* for malloc() */
#include <pthread.h>            /* for pthread_self() */
#include <stdarg.h>             /* for vsnprintf() */
#include <inttypes.h>           /* for PRI* format specifiers */

#define D4 "%" PRId32
#define D8 "%" PRId64
#define U4 "%" PRIu32
#define U8 "%" PRIu64

#ifndef _WIN32_WINNT

#define PATH_SEP "/"

#else

#define PATH_SEP "\\"

#endif  // _WIN32_WINNT

static i4b
getdate(char * buf, u8b bsz)
{
    time_t now = time(NULL);
    struct tm tms = { 0 };
    localtime_r(&now, &tms);
    u8b rv = strftime(buf, bsz, "%Y-%m-%d", &tms);
    if (!rv) {
        buf[0] = '\0';
        return LLE_DATEFMT;
    }
    return LLE_SUCCESS;
}

i4b
logopen(const char * dir, const char * fnpfx, LogHandle ** lhpp)
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
        char dbuf[12];
        rv = getdate(dbuf, sizeof(dbuf));
        if (rv != LLE_SUCCESS) {                break;
        }
        char fpath[1024];
        i4b psz = snprintf(fpath, sizeof(fpath), "%s%s%s-%s.log",
                           dir, PATH_SEP, fnpfx, dbuf);
        if (psz < 0) {
            rv = LLE_SNPRINTF_FAIL;             break;
        } 
        if (psz >= (i4b) sizeof(fpath)) {
            rv = LLE_LOGFILEPATHTOOLONG;        break;
        }
        lhp = (LogHandle *) malloc(sizeof(LogHandle));
        i4b lfd = open(fpath, O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (lfd < 0) {
            rv = LLE_LASTPLUSONE + errno;       break;
        }
        lhp->lhfdopents = time(NULL);
        lhp->lhfd = lfd;
        *lhpp = lhp;
    } while (0);
    if (rv != LLE_SUCCESS) {
        free(lhp);
        *lhpp = NULL;
    }
    return rv;
}

i4b
logwrite(LogHandle * lhp, const char * fn, const char * func,
         i4b ln, LogLevel ll, const char * fmt, ...)
{
    i4b rv = LLE_SUCCESS;
    do {
        if (!ll || (ll > LL_DEV)) {
            rv = LLE_INVALIDLOGLEVEL;           break;
        }
        struct timeval tv = { 0 };
        if (gettimeofday(&tv, NULL) != 0) {
            rv = LLE_GETTIMEOFDAY;              break;
        }
        struct tm tms = { 0 };
        localtime_r(&tv.tv_sec, &tms);

        char lbuf[2048];
        u8b cnt = strftime(lbuf, sizeof(lbuf), "%H:%M:%S", &tms);

        char llc = (ll == LL_DEV) ? 'D' : ((ll == LL_SPT) ? 'S' : 'U');

        cnt += snprintf(lbuf+cnt, sizeof(lbuf)-cnt,
                        ".%06d %c %d:" U8 " [%s:%d:%s] ",
                        (i4b) tv.tv_usec, llc, getpid(), (u8b) pthread_self(),
                        fn, ln, func);

        va_list ap;
        va_start(ap, fmt);
        cnt += vsnprintf(lbuf+cnt, sizeof(lbuf)-cnt, fmt, ap);
        va_end(ap);
        if (cnt >= (sizeof(lbuf))) {
            cnt = sizeof(lbuf) - 1;
        }
        lbuf[cnt++] = '\n';     /* replace trailing '\0', can't use strlen() */
        i8b wcnt = write(lhp->lhfd, lbuf, cnt);
        if (cnt != (u8b) wcnt) {
            rv = LLE_LOGWRITE_FAIL;
            break;
        }
    } while (0);
    return rv;
}

i4b
main(i4b argc, char ** argv)
{
    LogHandle * lhp = NULL;
    i4b rv = LLE_SUCCESS;
    do {
        rv = logopen("/tmp", "test", &lhp);
        if (rv != LLE_SUCCESS) {
            printf("Failed to open log file. E=%d\n", rv);
            break;
        }
        printf("Opened log file FD=%d\n", lhp->lhfd);
        /* rv = logwrite(lhp, __FILE__, __func__, __LINE__, LL_DEV, */
        /*               "longtextwithoutspacefor %s", "developer"); */
        /* rv = logwrite(lhp, __FILE__, __func__, __LINE__, LL_SPT, */
        /*               "longtextwithoutspacefor %s", "support"); */
        rv = LDEV(lhp, "longtextwithoutspaceusingmacrofor %s", "developer");
        rv = LSPT(lhp, "longtextwithoutspaceusingmacrofor %s", "support");
        rv = LUSR(lhp, "longtextwithoutspaceusingmacrofor %s", "user");
        rv = logwrite(lhp, __FILE__, __func__, __LINE__, LL_USR,
                      "longtextwithoutspacefor %s", "user");
        if (rv != LLE_SUCCESS) {
            printf("logwrite() failed with %d\n", rv);
            break;
        }
    } while (0);
    
    return rv;
}

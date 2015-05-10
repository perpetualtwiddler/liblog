#include "log.h"

#include <unistd.h>             /* for access(), write() */
#include <time.h>               /* for time(), strftime(), localtime_r() */
#include <sys/time.h>           /* for gettimeofday() */
#include <sys/types.h>          /* for open() */
#include <sys/stat.h>           /* for open() */
#include <fcntl.h>              /* for open() */
#include <errno.h>              /* for errno */
#include <stdlib.h>             /* for malloc() */
#include <stdarg.h>             /* for vsnprintf() */
#include <inttypes.h>           /* for PRI* format specifiers */
#include <string.h>             /* for strncpy() */

#define D4 "%" PRId32
#define D8 "%" PRId64
#define U4 "%" PRIu32
#define U8 "%" PRIu64

#ifndef _WIN32_WINNT

#define PATH_SEP "/"

#else

#define PATH_SEP "\\"

#endif  // _WIN32_WINNT

#define STRLCPY(d,s,dsz)        \
    strncpy((d), (s), (dsz));   \
    (d)[(dsz)-1] = '\0';


static i4b s_autorollover = 1;
static i4b s_process_log_level = LL_DEV;

void
log_set_logginglevel(i4b ll)
{
    s_process_log_level = ll;
}

i4b
log_get_logginglevel()
{
    return s_process_log_level;
}

void
log_disable_autorollover()
{
    s_autorollover = 0;
}

void
log_enable_autorollover()
{
    s_autorollover = 1;
}

int32_t
log_get_autorollover()
{
    return s_autorollover;
}

static i4b
getdatestring(char * buf, u8b bsz, u4b * mday)
{
    time_t now = time(NULL);
    struct tm tms = { 0 };
    localtime_r(&now, &tms);
    strftime(buf, bsz, "%Y-%m-%d", &tms);
    *mday = tms.tm_mday;
    return LLE_SUCCESS;
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
        u4b mday = 0;
        getdatestring(dbuf, sizeof(dbuf), &mday);
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
                char tb[256];
                snprintf(tb, sizeof(tb),
                         "Log close failure ignored: OLDFD=%d ERRNO=%d LLE=%d",
                         lhp->lhfd, errno, LLE_LOGCLOSE_FAIL);
                write(lfd, tb, strlen(tb));
            }
        }
        lhp->lhfd = lfd;
        lhp->mday = mday;
        if (*lhpp == NULL) {
            STRLCPY(lhp->ldir, dir, sizeof(lhp->ldir));
            STRLCPY(lhp->lfnpfx, fnpfx, sizeof(lhp->lfnpfx));
            pthread_mutex_init(&lhp->mutex, NULL);
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
log_write(LogHandle * lhp, const char * fn, const char * func,
          i4b ln, LogLevel ll, const char * fmt, ...)
{
    if (!lhp || (s_process_log_level < ll)) {
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

        if (s_autorollover && ((tms.tm_mday != lhp->mday) ||
                               ((tms.tm_hour == 23) &&
                                (tms.tm_min == 59) &&
                                (tms.tm_sec > 57)))) {
            pthread_mutex_lock(&lhp->mutex);
            lockacquired = 1;
            if (tms.tm_mday != lhp->mday) {
                /* Ignore potential failures from log_rollover() for now,
                   we'll effectively continue writing in the same log file
                   */
                log_rollover(lhp);
            }
        }
        i8b wcnt = write(lhp->lhfd, lbuf, cnt);
        if (cnt != (u8b) wcnt) {
            rv = LLE_LOGWRITE_FAIL;
            break;
        }
    } while (0);
    if (lockacquired) {
        pthread_mutex_unlock(&lhp->mutex);
        lockacquired = 0;
    }
    return rv;
}

typedef struct threadinfo_ {
    pthread_t thr_id;
    i4b thr_num;
    LogHandle * lhp;
} threadinfo;

static void *
thread_start(void * arg)
{
    threadinfo * tinfo = (threadinfo *) arg;
    LogHandle * lhp = tinfo->lhp;
    for (;;) {
        usleep(3 * 1000 * 1000 * tinfo->thr_num);
        for (u8b c = 0; c < 100; ++c) {
            LDEV(lhp, "LogIter=" U8 " Thread=%d pthread_t=" U8 " ID=" U8,
                 c, tinfo->thr_num, (u8b) tinfo->thr_id, (u8b) pthread_self());
        }
    }
    char * rv = (char *) malloc(512);
    snprintf(rv, 100, "Thread=%d Self=" U8, tinfo->thr_num,
             (u8b) pthread_self());
    return rv;
}

i4b
main(i4b argc, char ** argv)
{
    LogHandle * lhp = NULL;
    i4b rv = LLE_SUCCESS;
    do {
        rv = log_open(argv[1], "logtest", &lhp);
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

        if (rv != LLE_SUCCESS) {
            printf("logwrite() failed with %d\n", rv);
            break;
        }
        pthread_attr_t thr_attr;
        i4b pe = pthread_attr_init(&thr_attr);
        char pebuf[256];
        if (pe != 0) {
            strerror_r(pe, pebuf, sizeof(pebuf));
            LSPT(lhp, "pthread_attr_init() failed: E=%d MSG=%s", pe, pebuf);
            rv = 1;
            break;
        }
        i4b nthr = atoi(argv[2]);
        LSPT(lhp, "Starting %d threads", nthr);
        threadinfo * thr_arr = (threadinfo *) calloc(nthr, sizeof(threadinfo));
        for (i4b tc = 0; tc < nthr; ++tc)  {
            thr_arr[tc].thr_num = tc+1;
            thr_arr[tc].lhp = lhp;
            pthread_create(&thr_arr[tc].thr_id, &thr_attr,
                           thread_start, &thr_arr[tc]);
        }
        pthread_attr_destroy(&thr_attr);
        void * res = 0;
        for (i4b tc = 0; tc < nthr; ++tc) {
            pe = pthread_join(thr_arr[tc].thr_id, &res);
            LSPT(lhp, "TC=%d returned=%s", tc, (char *) res);
            free(res);
        }
        free(thr_arr);
    } while (0);
    
    return rv;
}

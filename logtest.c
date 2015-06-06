
#include "log.h"

#ifdef _WIN32_WINNT
#define THR_ID_T DWORD
#define D8 "%I64d"
#define U8 "%I64u"
#define THREAD_ID GetCurrentThreadId
#else
#include <inttypes.h>               /* for PRId64, etc. */
#include <stdlib.h>                 /* for malloc(), etc. */
#include <string.h>                 /* for strerror_r() */
#include <unistd.h>                 /* for usleep() */
#define THR_ID_T pthread_t
#define D8 "%" PRId64
#define U8 "%" PRIu64
#define THREAD_ID pthread_self
#endif

typedef struct threadinfo_ {
    THR_ID_T thr_id;
    i4b thr_num;
    LogHandle * lhp;
} threadinfo;

#ifdef _WIN32_WINNT
static DWORD
#else
static void *
#endif
thread_start(void * arg)
{
    threadinfo * tinfo = (threadinfo *) arg;
    LogHandle * lhp = tinfo->lhp;
    for (;;) {
#ifdef _WIN32_WINNT
        Sleep(3 * 1000 * tinfo->thr_num);
#else
        usleep(3 * 1000 * 1000 * tinfo->thr_num);
#endif
        for (u8b c = 0; c < 100; ++c) {
            LDEV(lhp, "LogIter=" U8 " Thread=%d pthread_t=" U8 " ID=" U8,
                 c, tinfo->thr_num, (u8b) tinfo->thr_id, (u8b) THREAD_ID());
        }
    }
#ifdef _WIN32_WINNT
    return 0;
#else
    char * rv = (char *) malloc(512);
    snprintf(rv, 100, "Thread=%d Self=" U8, tinfo->thr_num,
             (u8b) THREAD_ID());
    return rv;
#endif
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
        rv = LDEV(lhp, "longtextwithoutspaceusingmacrofor %s", "developer");
        rv = LSPT(lhp, "longtextwithoutspaceusingmacrofor %s", "support");
        rv = LUSR(lhp, "longtextwithoutspaceusingmacrofor %s", "user");

        if (rv != LLE_SUCCESS) {
            printf("logwrite() failed with %d\n", rv);
            break;
        }
#ifndef _WIN32_WINNT
        pthread_attr_t thr_attr;
        i4b pe = pthread_attr_init(&thr_attr);
        char pebuf[256];
        if (pe != 0) {
            strerror_r(pe, pebuf, sizeof(pebuf));
            LSPT(lhp, "pthread_attr_init() failed: E=%d MSG=%s", pe, pebuf);
            rv = 1;
            break;
        }
#endif
        printf("Starting %s threads\n", argv[2]);
        i4b nthr = atoi(argv[2]);
        LSPT(lhp, "Starting %d threads", nthr);
        threadinfo * thr_arr = (threadinfo *) calloc(nthr, sizeof(threadinfo));
#ifdef _WIN32_WINNT
        HANDLE * thr_hndls = (HANDLE *) calloc(nthr, sizeof(HANDLE));
#endif
        for (i4b tc = 0; tc < nthr; ++tc)  {
            thr_arr[tc].thr_num = tc+1;
            thr_arr[tc].lhp = lhp;
#ifdef _WIN32_WINNT
            thr_hndls[tc] = CreateThread(NULL, 0,
                                         (LPTHREAD_START_ROUTINE) thread_start,
                                         &thr_arr[tc], 0, &thr_arr[tc].thr_id);
#else
            pthread_create(&thr_arr[tc].thr_id, &thr_attr,
                           thread_start, &thr_arr[tc]);
#endif
        }
#ifndef _WIN32_WINNT
        pthread_attr_destroy(&thr_attr);
#endif

#ifdef _WIN32_WINNT
        WaitForMultipleObjects(nthr, thr_hndls, TRUE, INFINITE);
        for (i4b tc = 0; tc < nthr; ++tc) {
            CloseHandle(thr_hndls[tc]);
        }
#else
        void * res = 0;
        for (i4b tc = 0; tc < nthr; ++tc) {
            pe = pthread_join(thr_arr[tc].thr_id, &res);
            LSPT(lhp, "TC=%d returned=%s", tc, (char *) res);
            free(res);
        }
#endif
        free(thr_arr);
    } while (0);
    
    return rv;
}

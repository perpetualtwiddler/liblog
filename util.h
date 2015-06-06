#ifndef _UTIL_HEADER_INCLUDED_
#define _UTIL_HEADER_INCLUDED_
#include <stdint.h>
#include <time.h>

typedef int8_t i1b;
typedef uint8_t u1b;
typedef int16_t i2b;
typedef uint16_t u2b;
typedef int32_t i4b;
typedef uint32_t u4b;
typedef int64_t i8b;
typedef uint64_t u8b;

i4b gettimeofday(struct timeval * tv, struct timezone * /* tzp */);
i4b llvsnprintf(char * buf, size_t cnt, const char * fmt, va_list ap);

#endif // _UTIL_HEADER_INCLUDED_

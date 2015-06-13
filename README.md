# liblog
Simple C/C++ logging library

# API
int32_t log_open(const char * dir, const char * lfpfx, LogHandle ** lhpp);

int32_t log_write(LogHandle * lhp, const char * filename, const char * funcname, int32_t linenum, LogLevel ll, const char * fmt, ...);

# Build

On Windows:

Requires, Visual Studio 2012 & gyp
set GYP_MSVS_VERSION=2012
gyp --depth=. log.gyp
msbuild log.sln

On Linux:
gyp --depth=. log.gyp
make

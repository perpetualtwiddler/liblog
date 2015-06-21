# liblog
Simple C/C++ logging library

# API
int32_t log_open(const char * dir, const char * lfpfx, LogHandle ** lhpp);

int32_t log_write(LogHandle * lhp, const char * filename, const char * funcname, int32_t linenum, LogLevel ll, const char * fmt, ...);

Ready to use Macros to log using each of the 3 levels of logging supported:

- LUSR(lhp, fmt, ...)
- LSPT(lhp, fmt, ...)
- LDEV(lhp, fmt, ...)

Use LDEV() to log debug messages for the developer.

LSPT() is for log messages useful to Support (troubleshooting messages that provide information).

LUSR() is for warning or error messages that could be useful to users.

# Build

On Windows:

Requires:
* [Visual Studio 2012](http://www.microsoft.com/en-in/download/details.aspx?id=34673)
* [Python](https://www.python.org)
* [gyp](https://chromium.googlesource.com/external/gyp/)

Ensure `python` and `gyp` are available in %PATH%. In a VC++ command shell:

* set GYP_MSVS_VERSION=2012
* gyp --depth=. log.gyp
* msbuild log.sln

On Linux:
* gyp --depth=. log.gyp
* make

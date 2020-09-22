#ifndef _LIMITS_H
#define _LIMITS_H

#include <endian.h>

#define CHAR_BIT        8
#define SCHAR_MIN       (-1 - SCHAR_MAX)
#define SCHAR_MAX       (__SCHAR_MAX__)
#define UCHAR_MAX       (SCHAR_MAX * 2 + 1)
#undef CHAR_MIN
#define CHAR_MIN        SCHAR_MIN
#undef CHAR_MAX
#define CHAR_MAX        SCHAR_MAX
#define SHRT_MIN	    (-1 - SHRT_MAX)
#define SHRT_MAX	    (__SHRT_MAX__)
#define USHRT_MAX	    (SHRT_MAX * 2 + 1)
#define INT_MIN		    (-1 - INT_MAX)
#define INT_MAX		    (__INT_MAX__)
#define UINT_MAX	    (INT_MAX * 2u + 1)
#define LONG_MIN	    (-1l - LONG_MAX)
#define LONG_MAX	    (__LONG_MAX__)
#define ULONG_MAX	    (LONG_MAX * 2ul + 1)
#define LLONG_MAX	    0x7fffffffffffffffll
#define LLONG_MIN	    (-1ll - LLONG_MAX)
#define ULLONG_MAX      (~0ull)
#define SSIZE_MIN       LONG_MIN
#define SSIZE_MAX       LONG_MAX

#endif

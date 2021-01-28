/* Copyright (C) 2018, Ward Jaradat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * Notes:
 * 
 * Some parts have been ported and adapted from dietlibc, or written from
 * scratch to accommodate integration with other components including the
 * Java interpreter and related dependencies
 */

#include <os/config.h>
#include <errno.h>
#include <strings.h>
#include <sys/cdefs.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <os/config.h>
#include <os/console.h>
#include <os/sched.h>
#include <os/mm.h>
#include <os/bug.h>
#include <sys/mman.h>
#include <os/config.h>
#include <os/sched.h>
#include <os/lib.h>
#include <os/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <endian.h>
#include <stddef.h>
#ifdef ENABLE_JAVA
#include <locale.h>
#endif
#include <inttypes.h>
#include <stdint.h>
#include <os/kernel.h>
#include <os/xmalloc.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <os/unused.h>
#include <float.h>
#include "libm.h"
#include <setjmp.h>
#include <signal.h>

static char buf[25];
static const char days[] = "Sun Mon Tue Wed Thu Fri Sat ";
static const char months[] = "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec ";
static char *strtok_pos;

int * __errno_location(void)
{
	return &errno;
}

char * strtok_r(char*s,const char*delim,char**ptrptr)
{
    char*tmp=0;
    if (s==0) s=*ptrptr;
    s+=strspn(s,delim);             /* overread leading delimiter */
    if (__likely(*s))
    {
        tmp=s;
        s+=strcspn(s,delim);
        if (__likely(*s))
        {
            *s++=0; /* not the end ? => terminate it */
        }
    }
    *ptrptr=s;
    return tmp;
}

char * strtok(char *s, const char *delim)
{
    return strtok_r(s,delim,&strtok_pos);
}

char * strstr(const char * s1,const char * s2)
{
    int l1, l2;
    l2 = strlen(s2);
    if (!l2)
        return (char *) s1;
    l1 = strlen(s1);
    while (l1 >= l2)
    {
        l1--;
        if (!memcmp(s1,s2,l2))
            return (char *) s1;
        s1++;
    }
    return NULL;
}

size_t strspn(const char *s, const char *accept)
{
    size_t l = 0;
    const char *a;
    for (; *s; s++)
    {
        for (a = accept; *a && *s != *a; a++);

        if (!*a)
        {
            break;
        }
        else
        {
            l++;
        }
    }
    return l;
}

char * strrchr(const char *t, int c)
{
    register char ch;
    register const char *l=0;
    ch = c;
    for (;;)
    {
        if (__unlikely(*t == ch)) l=t; if (__unlikely(!*t)) return (char*)l; ++t;
#ifndef WANT_SMALL_STRING_ROUTINES
        if (__unlikely(*t == ch)) l=t; if (__unlikely(!*t)) return (char*)l; ++t;
        if (__unlikely(*t == ch)) l=t; if (__unlikely(!*t)) return (char*)l; ++t;
        if (__unlikely(*t == ch)) l=t; if (__unlikely(!*t)) return (char*)l; ++t;
#endif
    }
    return (char*)l;
}

char *strpbrk(const char *s, const char *accept)
{
    register unsigned int i;
    for (; *s; s++)
        for (i=0; accept[i]; i++)
            if (*s == accept[i])
                return (char*)s;
    return 0;
}

size_t strnlen(const char * s, size_t count) {

    const char *sc;

    for (sc = s; count-- && *sc != '\0'; ++sc)
        /* nothing */;

    return sc - s;
}

char * strncpy(char * dest,const char *src,size_t count)
{
    char *tmp = dest;
    while (count-- && (*dest++ = *src++) != '\0')
        /* nothing */;
    return tmp;
}

int strncmp(const char * cs,const char * ct,size_t count)
{
    register signed char __res = 0;
    while (count)
    {
        if ((__res = *cs - *ct++) != 0 || !*cs++)
            break;
        count--;
    }
    return __res;
}

char *strncat(char *s, const char *t, size_t n)
{
    char *dest=s;
    register char *max;
    s+=strlen(s);
    if (__unlikely((max=s+n)==s)) goto fini;
    for (;;)
    {
        if (__unlikely(!(*s = *t))) break; if (__unlikely(++s==max)) break; ++t;
#ifndef WANT_SMALL_STRING_ROUTINES
        if (__unlikely(!(*s = *t))) break; if (__unlikely(++s==max)) break; ++t;
        if (__unlikely(!(*s = *t))) break; if (__unlikely(++s==max)) break; ++t;
        if (__unlikely(!(*s = *t))) break; if (__unlikely(++s==max)) break; ++t;
#endif
    }
    *s=0;
    fini:
    return dest;
}

size_t strlen(const char * s)
{
    const char *sc;
    for (sc = s; *sc != '\0'; ++sc)
        /* nothing */;
    return sc - s;
}

char * strerror(int errnum)
{
    return "strerror unsupported";
}

char * strerror_r(int errnum, char *buf, size_t buflen)
{
    return strerror(errnum);
}

char * strdup(const char *x)
{
    int l = strlen(x);
    char *res = xmalloc_align(l + 1, 4);
    if (!res) return NULL;
    memcpy(res, x, l + 1);
    return res;
}

size_t strcspn(const char *s, const char *reject)
{
    size_t l=0;
    int i;
    for (; *s; ++s)
    {
        for (i=0; reject[i]; ++i)

            if (*s==reject[i]) return l;

        ++l;
    }
    return l;
}

char * strcpy(char * dest,const char *src)
{
    char *tmp = dest;
    while ((*dest++ = *src++) != '\0')
        /* nothing */;
    return tmp;
}

int strcmp(const char * cs,const char * ct)
{
    register signed char __res;
    while (1)
    {
        if ((__res = *cs - *ct++) != 0 || !*cs++)
            break;
    }
    return __res;
}

int strcoll(const char *s,const char* t) __attribute__((weak,alias("strcmp")));

char * strchr(const char * s, int c)
{
    for(; *s != (char) c; ++s)
        if (*s == '\0')
            return NULL;
    return (char *)s;
}

char * strcat(char * dest, const char * src)
{
    char *tmp = dest;
    while (*dest)
        dest++;
    while ((*dest++ = *src++) != '\0');
    return tmp;
}

void * memset(void * s,int c,size_t count)
{
    char *xs = (char *) s;
    while (count--)
        *xs++ = c;
    return s;
}

void * memmove(void *dst, const void *src, size_t count)
{
    char *a = dst;
    const char *b = src;
    if (src!=dst)
    {
        if (src>dst)
        {
            while (count--) *a++ = *b++;

        }
        else
        {
            a+=count-1;
            b+=count-1;
            while (count--) *a-- = *b--;
        }
    }
    return dst;
}

void * memcpy(void * dest,const void *src,size_t count)
{
    char *tmp = (char *) dest;
    const char *s = src;
    while (count--)
        *tmp++ = *s++;
    return dest;
}

int memcmp(const void * cs,const void * ct,size_t count)
{
    const unsigned char *su1, *su2;
    signed char res = 0;
    for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
        if ((res = *su1 - *su2) != 0)
            break;
    return res;
}

void * memchr(const void *s, int c, size_t n)
{
    const unsigned char *pc = (unsigned char *) s;
    for (;n--;pc++)
        if (*pc == c)
            return ((void *) pc);
    return 0;
}

void * memccpy(void *dst, const void *src, int c, size_t count)
{
    char *a = dst;
    const char *b = src;
    while (count--)
    {
        *a++ = *b;
        if (*b==c)
        {
            return (void *)a;
        }
        b++;
    }
    return 0;
}

#define DEFAULT_STRING_CLASSPATH "CLASSPATH"
#define DEFAULT_STRING_BOOTCLASSPATH "BOOTCLASSPATH"
#define DEFAULT_STRING_HOME "HOME"
#define DEFAULT_STRING_USER "USR"
#define DEFAULT_STRING_LD_LIBRARY_PATH "LD_LIBRARY_PATH"
#define DEFAULT_STRING_LC_CTYPE "LC_CTYPE"
#define DEFAULT_STRING_LC_ALL "LC_ALL"
#define	M	((1U<<31) -1)
#define	A	48271
#define	Q	44488
#define	R	3399
#if __WORDSIZE == 64
#define ABS_LONG_MIN 9223372036854775808UL
#else
#define ABS_LONG_MIN 2147483648UL
#endif

extern int
munmap(void *start, size_t length)
{
#ifdef DEBUG_ME
    printk("munmap: starting address is %p, length is 0x%lx\n", start, length);
#endif
    int order;
    order = get_order(length);
    free_pages(start, order);
    return 0;
}

extern void *
mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
#ifdef DEBUG_ME
    printk("mmap: goal address is %p, length is 0x%lx, protection mode is 0x%x, flags are 0x%x\n", addr, length, prot, flags);
#endif
    mem_region region;
    int order_num_pages;
    order_num_pages = get_order(length);
    region.address = alloc_pages(order_num_pages);
    region.pages = 1 << order_num_pages;
    region.end = region.address + (region.pages << PAGE_SHIFT);
#ifdef DEBUG_ME
    printk("mmap: allocating memory at %x\n", region.address);
#endif
    return (void *) region.address;
}

void clearerr(FILE *stream)
{
    errno = ENOSYS;
}

int feof(FILE *stream)
{
    errno = ENOSYS;
    return -1;
}

int ferror(FILE *stream)
{
    errno = ENOSYS;
    return 0;
}

int fflush(FILE *stream)
{
    errno = ENOSYS;
    return 0;
}

int fgetc(FILE *stream)
{
    errno = ENOSYS;
    return 0;
}

int fgetpos(FILE *restrict stream, fpos_t *restrict pos)
{
    errno = ENOSYS;
    return -1;
}

char * fgets(char *s, int size, FILE *stream)
{
    errno = ENOSYS;
    return NULL;
}

int setvbuf(FILE *restrict stream, char *restrict buf, int type, size_t size) 
{

    errno = EBADF;
    return -1;
}

int fileno(FILE *stream)
{
    if (stream == stdin)
    {
        return 0;
    }
    else if (stream == stdout)
    {
        return 1;
    }
    else if (stream == stderr)
    {
        return 2;
    }
    errno = EBADF;
    return -1;
}

int fputc(int c, FILE *stream)
{
    errno = ENOSYS;
    return -1;
}

int ungetc(int c, FILE *stream) {
    printk("called ungetc\n");
    return 0;
}

int fputs(const char *s, FILE *stream)
{
    int len;
    len = sizeof s;
    int res = 0;
    if ((int *) stream == (int *) 1)
    {
        /* Standard output stream */
        res = write((int) 1, s, len);
    }
    else if ((int *) stream == (int *) 2)
    {
        /* Standard error stream */
        res = write((int) 2, s, len);
    }
    if (res == len)
    {
        return res;
    }
    return -1; // EOF
}

FILE * freopen(const char *restrict pathname, const char *restrict mode, FILE *restrict stream)
{
    errno = ENOSYS;
    return NULL;
}

int fsetpos(FILE *stream, const fpos_t *pos)
{
    errno = ENOSYS;
    return -1;
}


int getc(FILE *stream)
{
    errno = ENOSYS;
    return -1;
}

int getchar(void)
{
    errno = ENOSYS;
    return -1;
}

int remove(const char *pathname)
{
    errno = ENOSYS;
    return -1;
}

void rewind(FILE *stream)
{
    errno = ENOSYS;
}

int rename(const char *oldpath, const char *newpath)
{
    errno = EINVAL;
    return -1;
}

int putc(int c, FILE *stream) {

    printk("called putc\n");
    return 0;
}

void perror (const char* prepend)
{
    char message[50];
    sprintf(message, "%d.\n", errno);
    if (prepend)
    {
        write ((int) 2, prepend, strlen(prepend));
        write ((int) 2, ": ", 2 );
    }
    write ((int) 2, message, strlen(message));
    write ((int) 2, "\n", 1 );
}

int strcasecmp (const char* s1, const char* s2)
{
	register unsigned int  x2;
	register unsigned int  x1;
	while (1)
	{
		x2 = *s2 - 'A'; if (__unlikely(x2 < 26u)) x2 += 32;
		x1 = *s1 - 'A'; if (__unlikely(x1 < 26u)) x1 += 32;
		s1++; s2++;
		if ( __unlikely(x2 != x1) )
			break;
		if ( __unlikely(x1 == (unsigned int)-'A') )
			break;
	}
	return x1 - x2;
}

int ffs(int i)
{
	int plus=0;
	if ((i&0xffff)==0)
	{
		plus+=16;
		i>>=16;
	}
	if ((i&0xff)==0)
	{
		plus+=8;
		i>>=8;
	}
	if ((i&0xf)==0)
	{
		plus+=4;
		i>>=4;
	}
	if (i&1) return plus+1;
	if (i&2) return plus+2;
	if (i&4) return plus+3;
	if (i&8) return plus+4;
	return 0;
}

int write (int file, const char *ptr, int len)
{
    int i;

    if ((file != (int) 0) && (file != (int) 1) && (file != (int) 2))
    {
        errno = EBADF;
        return -1;
    }

    for (i = 0; i < len; i++)
    {
        printf("%c", ptr[i]);
    }
    return len;
}

void * malloc(size_t size) 
{ 
    return xmalloc_align(size, DEFAULT_ALIGN); 
}

void * realloc(void *ptr, size_t size)
{
    return xrealloc(ptr, size, DEFAULT_ALIGN);
}

void * calloc(size_t nelem, size_t elsize)
{
    return xcalloc(nelem, elsize); 
}

void free(void *ptr)
{
    xfree(ptr);
}

void * malloc_at(const void *p, size_t size)
{
    return xmalloc_at(p, size);
}

void free_at(void *start, size_t length) {
    xfree_at(start, length);
}

unsigned long int strtoul(const char *ptr, char **endptr, int base)
{
    int neg = 0, overflow = 0;
    unsigned long int v=0;
    const char* orig;
    const char* nptr=ptr;

    while(isspace(*nptr))
    {
        ++nptr;
    }
    if (*nptr == '-')
    {
        neg=1;
        nptr++;
    }
    else if (*nptr == '+')
    {
        ++nptr;
    }
    orig=nptr;
    if (base==16 && nptr[0]=='0')
    {
        goto skip0x;
    }
    if (base)
    {
        register unsigned int b=base-2;
        if (__unlikely(b>34))
        {
            errno=EINVAL;
            return 0;
        }
    }
    else
        {
        if (*nptr=='0')
        {
            base=8;
            skip0x:
            if ((nptr[1]=='x'||nptr[1]=='X') && isxdigit(nptr[2]))
            {
                nptr+=2;
                base=16;
            }
        }
        else
        {
            base = 10;
        }
    }
    while(__likely(*nptr))
    {
        register unsigned char c=*nptr;
        c=(c>='a'?c-'a'+10:c>='A'?c-'A'+10:c<='9'?c-'0':0xff);
        if (__unlikely(c>=base)) break;	/* out of base */
        {
            register unsigned long x=(v&0xff)*base+c;
            register unsigned long w=(v>>8)*base+(x>>8);
            if (w>(ULONG_MAX>>8))
            {
                overflow=1;
            }
            v=(w<<8)+(x&0xff);
        }
        ++nptr;
    }
    if (__unlikely(nptr==orig))
    {		/* no conversion done */
        nptr=ptr;
        errno=EINVAL;
        v=0;
    }
    if (endptr)
    {
        *endptr=(char *)nptr;
    }
    if (overflow)
    {
        errno=ERANGE;
        return ULONG_MAX;
    }
    return (neg?-v:v);
}

long int strtol(const char *nptr, char **endptr, int base)
{
    int neg=0;
    unsigned long int v;
    const char*orig=nptr;

    while(isspace(*nptr))
    {
        nptr++;
    }
    if (*nptr == '-' && isalnum(nptr[1]))
    {
        neg=-1;
        ++nptr;
    }
    v=strtoul(nptr,endptr,base);
    if (endptr && *endptr==nptr)
    {
        *endptr=(char *)orig;
    }
    if (v>=ABS_LONG_MIN)
    {
        if (v==ABS_LONG_MIN && neg)
        {
            errno=0;
            return v;
        }
        errno=ERANGE;
        return (neg?LONG_MIN:LONG_MAX);
    }
    return (neg?-v:v);
}

double strtod(const char* s, char** endptr)
{
    register const char*  p     = s;
    register long double  value = 0.L;
    int                   sign  = +1;
    long double           factor;
    unsigned int          expo;

    while ( isspace(*p) )
    {
        p++;
    }
    switch (*p)
    {
        case '-': sign = -1;
        case '+': p++;
        default : break;
    }
    while ( (unsigned int)(*p - '0') < 10u )
    {
        value = value * 10 + (*p++ - '0');
    }
    if ( *p == '.' )
    {
        factor = 1.;
        p++;
        while ( (unsigned int)(*p - '0') < 10u )
        {
            factor *= 0.1;
            value  += (*p++ - '0') * factor;
        }
    }
    if ( (*p | 32) == 'e' )
    {
        expo   = 0;
        factor = 10.L;
        switch (*++p)
        {                 // ja hier wei� ich nicht, was mindestens nach einem 'E' folgenden MUSS.
            case '-': factor = 0.1;
            case '+': p++;
                break;
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                break;
            default : value = 0.L;
                p     = s;
                goto done;
        }
        while ( (unsigned int)(*p - '0') < 10u )
        {
            expo = 10 * expo + (*p++ - '0');
        }
        while ( 1 )
        {
            if ( expo & 1 )
            {
                value *= factor;
            }
            if ( (expo >>= 1) == 0 )
            {
                break;
            }
            factor *= factor;
        }
    }

    done:
    if ( endptr != NULL )
    {
        *endptr = (char *) p;
    }
    return value * sign;
}

void srand(unsigned int i)
{
	unsigned int seed=1;
    seed=i?i:23;
}

int rand_r(unsigned int* seed)
{
    int32_t X;
    X = *seed;
    X = A*(X%Q) - R * (int32_t) (X/Q);
    if (X < 0)
        X += M;
    *seed = X;
    return X;
}

int rand(void)
{
	unsigned int seed=1;
    return rand_r(&seed);
}

static void
exch(char* base,size_t size,size_t a,size_t b)
{
    char* x=base+a*size;
    char* y=base+b*size;
    while (size)
    {
        char z=*x;
        *x=*y;
        *y=z;
        --size; ++x; ++y;
    }
}

static void quicksort(char* base,size_t size,ssize_t l,ssize_t r, int (*compar)(const void*,const void*))
{

    /* Quicksort with 3-way partitioning, ala Sedgewick */
    /* Blame him for the scary variable names */
    /* http://www.cs.princeton.edu/~rs/talks/QuicksortIsOptimal.pdf */

    ssize_t i=l-1, j=r, p=l-1, q=r, k;
    char* v=base+r*size;
    if (r<=l) return;

#ifdef RAND
    /*
     We chose the rightmost element in the array to be sorted as pivot,
     which is OK if the data is random, but which is horrible if the
     data is already sorted.  Try to improve by exchanging it with a
     random other pivot.
	 */
	exch(base,size,l+(rand()%(r-l)),r);
#elif defined MID
    /*
     We chose the rightmost element in the array to be sorted as pivot,
     which is OK if the data is random, but which is horrible if the
     data is already sorted.  Try to improve by chosing the middle
     element instead.
	 */
	exch(base,size,l+(r-l)/2,r);
#endif

    for (;;)
    {
        while (++i != r && compar(base+i*size,v)<0) ;
        while (compar(v,base+(--j)*size)<0) if (j == l) break;
        if (i >= j) break;
        exch(base,size,i,j);
        if (compar(base+i*size,v)==0) exch(base,size,++p,i);
        if (compar(v,base+j*size)==0) exch(base,size,j,--q);
    }
    exch(base,size,i,r); j = i-1; ++i;
    for (k=l; k<p; k++, j--) exch(base,size,k,j);
    for (k=r-1; k>q; k--, i++) exch(base,size,i,k);
    quicksort(base,size,l,j,compar);
    quicksort(base,size,i,r,compar);
}

void qsort(void* base,size_t nmemb,size_t size,int (*compar)(const void*,const void*))
{
    if (nmemb >= (((size_t)-1)>>1) || size >= (((size_t)-1)>>1))
    {
        return;
    }
#if 0
    if (sizeof(size_t) < sizeof(unsigned long long))
    {
		if ((unsigned long long)size * nmemb > (size_t)-1)
		{
		    return;
		}
	}
	else
	{
		if (size*nmemb/nmemb != size)
		{
		    return;
		}
	}
#endif
    if (nmemb>1)
    {
        quicksort(base,size,0,nmemb-1,compar);
    }
}

#ifdef ENABLE_JAVA
int getloadavg(double loadavg[], int nelem)
{
    return -1;
}

char *getenv(const char *name)
{
#ifdef ENABLE_CLASSPATH
    if (strcmp(name, DEFAULT_STRING_CLASSPATH) == 0)
    {
        return (char *) DEFAULT_CLASSPATH;
    }
    if (strcmp(name, DEFAULT_STRING_BOOTCLASSPATH) == 0)
    {
        return DEFAULT_BOOTCLASSPATH;
    }
    if (strcmp(name, DEFAULT_STRING_HOME) == 0)
    {
        return (char *) DEFAULT_HOME;
    }
    if (strcmp(name, DEFAULT_STRING_USER) == 0)
    {
        return (char *) DEFAULT_USER;
    }
    if (strcmp(name, DEFAULT_STRING_LD_LIBRARY_PATH) == 0)
    {
        return NULL;
    }
    if (strcmp(name, DEFAULT_STRING_LC_CTYPE))
    {
        return LOCALE_LC_CTYPE;
    }
    if (strcmp(name, DEFAULT_STRING_LC_ALL))
    {
        /* This is not applicable in this environment */
        return NULL;
    }
#endif
    return NULL;
}
#endif

div_t div(int numerator, int denominator)
{
    div_t x;
    x.quot=numerator/denominator;
    x.rem=numerator-x.quot*denominator;
    return x;
}

void exit(int status)
{
    exit_current_thread();
}

#ifdef ENABLE_JAVA
void * bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void* , const void* ))
{
    size_t m;
    while (__likely(nmemb))
    {
        int tmp;
        void *p;
        m=nmemb/2;
        p=(void *) (((const char *) base) + (m * size));
        if ((tmp=(*compar)(key,p))<0)
        {
            nmemb=m;
        }
        else if (tmp>0)
        {
            base=p+size;
            nmemb-=m+1;
        }
        else {
            return p;
        }
    }
    return 0;
}
#endif

long long int atoll(const char* s) __attribute__((alias("atol")));

#if __WORDSIZE == 64
long int labs(long int i)
{
    return i>=0?i:-i;
}
long long int llabs(long long int i) __attribute__((alias("labs")));
#endif

#if __WORDSIZE != 64
long long int atoll(const char* s)
{
    long long int v=0;
    int sign=1;
    while ( *s == ' '  ||  (unsigned int)(*s - 9) < 5u) ++s;
    switch (*s)
    {
        case '-': sign=-1;
        case '+': ++s;
    }
    while ((unsigned int) (*s - '0') < 10u)
    {
        v=v*10+*s-'0'; ++s;
    }
    return sign==-1?-v:v;
}
#endif

long int atol(const char* s)
{
    long int v=0;
    int sign=0;
    while ( *s == ' '  ||  (unsigned int)(*s - 9) < 5u) ++s;
    switch (*s)
    {
        case '-': sign=-1;
        case '+': ++s;
    }
    while ((unsigned int) (*s - '0') < 10u)
    {
      v=v*10+*s-'0'; ++s;
    }
    return sign?-v:v;
}

int atoi(const char* s)
{
    long int v=0;
    int sign=1;
    while ( *s == ' '  ||  (unsigned int)(*s - 9) < 5u) s++;
    switch (*s)
    {
        case '-': sign=-1;
        case '+': ++s;
    }
    while ((unsigned int) (*s - '0') < 10u)
    {
        v=v*10+*s-'0'; ++s;
    }
    return sign==-1?-v:v;
}


void abort(void)
{
    /* This function causes abnormal termination of a process in monolithic systems but in this 
    environment it instructs the scheduler to exit the current thread's execution */
    exit_current_thread();
}

#if __WORDSIZE == 64
#define ABS_LONG_MIN 9223372036854775808UL
#else
#define ABS_LONG_MIN 2147483648UL
#endif

int abs(int i)
{
    return i>=0?i:-i;
}
#if __WORDSIZE == 32
long labs(long i) __attribute__((alias("abs")));
#endif

#ifdef ENABLE_JAVA

static void num2str(char *c,int i)
{
    c[0]=i/10+'0';
    c[1]=i%10+'0';
}

char * asctime_r(const struct tm *t, char *buf)
{
    *(int*)buf=*(int*)(days+(t->tm_wday<<2));
    *(int*)(buf+4)=*(int*)(months+(t->tm_mon<<2));
    num2str(buf+8,t->tm_mday);
    if (buf[8]=='0')
    {
        buf[8]=' ';
    }
    buf[10]=' ';
    num2str(buf+11,t->tm_hour);
    buf[13]=':';
    num2str(buf+14,t->tm_min);
    buf[16]=':';
    num2str(buf+17,t->tm_sec);
    buf[19]=' ';
    num2str(buf+20,(t->tm_year+1900)/100);
    num2str(buf+22,(t->tm_year+1900)%100);
    buf[24]='\n';
    return buf;
}

char * asctime(const struct tm *timeptr)
{
    return asctime_r(timeptr,buf);
}

char * ctime(const time_t *timep)
{
    return asctime(localtime(timep));
}

char * ctime_r(const time_t *timep, char* buf)
{
    return asctime_r(localtime(timep),buf);
}

#endif

clock_t clock(void)
{
    // TODO Fix me
    return 0;
}

double difftime(time_t time1, time_t time2)
{
    return (double)time1 - (double)time2;
}

struct tm * localtime(const time_t* t)
{
    errno = EOVERFLOW;
    return NULL;
}

time_t time(time_t *t)
{
    struct timeval tv;
    gettimeofday(&tv);
    time_t result = tv.tv_sec;
    return result;
}

int ftime(struct timeb *tb)
{
    struct timeval tv;
    gettimeofday(&tv);
    tb->time = tv.tv_sec;
    tb->millitm = tv.tv_usec / 1000;
    tb->timezone = 0;
    tb->dstflag = 0;
    return 0;
}

#define ZEROPAD 1               /* pad with zero */
#define SIGN    2               /* unsigned/signed long */
#define PLUS    4               /* show plus */
#define SPACE   8               /* space if plus */
#define LEFT    16              /* left justified */
#define SPECIAL 32              /* 0x */
#define LARGE   64              /* use 'ABCDEF' instead of 'abcdef' */

#if (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
int __signbitl(long double x)
{
	union ldshape u = {x};
	return u.i.se >> 15;
}
#elif LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
int __signbitl(long double x)
{
	return __signbit(x);
}
#endif

#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
int __fpclassifyl(long double x)
{
	return __fpclassify(x);
}
#elif LDBL_MANT_DIG == 64 && LDBL_MAX_EXP == 16384
int __fpclassifyl(long double x)
{
	union ldshape u = {x};
	int e = u.i.se & 0x7fff;
	int msb = u.i.m>>63;
	if (!e && !msb)
		return u.i.m ? FP_SUBNORMAL : FP_ZERO;
	if (!msb)
		return FP_NAN;
	if (e == 0x7fff)
		return u.i.m << 1 ? FP_NAN : FP_INFINITE;
	return FP_NORMAL;
}
#elif LDBL_MANT_DIG == 113 && LDBL_MAX_EXP == 16384
int __fpclassifyl(long double x)
{
	union ldshape u = {x};
	int e = u.i.se & 0x7fff;
	u.i.se = 0;
	if (!e)
		return u.i2.lo | u.i2.hi ? FP_SUBNORMAL : FP_ZERO;
	if (e == 0x7fff)
		return u.i2.lo | u.i2.hi ? FP_NAN : FP_INFINITE;
	return FP_NORMAL;
}
#endif

#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
long double frexpl(long double x, int *e)
{
	return frexp(x, e);
}
#elif (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
long double frexpl(long double x, int *e)
{
	union ldshape u = {x};
	int ee = u.i.se & 0x7fff;

	if (!ee) {
		if (x) {
			x = frexpl(x*0x1p120, e);
			*e -= 120;
		} else *e = 0;
		return x;
	} else if (ee == 0x7fff) {
		return x;
	}

	*e = ee - 0x3ffe;
	u.i.se &= 0x8000;
	u.i.se |= 0x3ffe;
	return u.f;
}
#endif

int __rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldsetm, long nr);

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) 
{
	return __rt_sigprocmask(how, set, oldset, _NSIG/8);
}

int __sigjmp_save(sigjmp_buf env,int savemask);

int __sigjmp_save(sigjmp_buf env,int savemask) 
{
	env[0].__mask_was_saved = 0;
	return 0;
}

void __longjmp(void*env,int val);

void __libc_longjmp(sigjmp_buf env,int val);

void __libc_longjmp(sigjmp_buf env,int val) 
{
	if (env[0].__mask_was_saved) 
	{
		sigprocmask(SIG_SETMASK,(sigset_t*)&env[0].__saved_mask,0);
	}

	if (val==0) val=1;
	__longjmp(env[0].__jmpbuf,val);
}

void longjmp(sigjmp_buf env,int val) __attribute__((weak,alias("__libc_longjmp")));

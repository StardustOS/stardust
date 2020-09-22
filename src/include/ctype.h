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
 * Code ported and adapted from Xen/MiniOS
 */

#ifndef	_CTYPE_H_
#define _CTYPE_H_

#define _U	0x01
#define _L	0x02
#define _D	0x04
#define _C	0x08
#define _P	0x10
#define _S	0x20
#define _X	0x40
#define _SP	0x80

unsigned char _ctype[] = {
_C,_C,_C,_C,_C,_C,_C,_C,
_C,_C|_S,_C|_S,_C|_S,_C|_S,_C|_S,_C,_C,
_C,_C,_C,_C,_C,_C,_C,_C,
_C,_C,_C,_C,_C,_C,_C,_C,
_S|_SP,_P,_P,_P,_P,_P,_P,_P,
_P,_P,_P,_P,_P,_P,_P,_P,
_D,_D,_D,_D,_D,_D,_D,_D,
_D,_D,_P,_P,_P,_P,_P,_P,
_P,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U,
_U,_U,_U,_U,_U,_U,_U,_U,
_U,_U,_U,_U,_U,_U,_U,_U,
_U,_U,_U,_P,_P,_P,_P,_P,
_P,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L,
_L,_L,_L,_L,_L,_L,_L,_L,
_L,_L,_L,_L,_L,_L,_L,_L, 
_L,_L,_L,_P,_P,_P,_P,_C,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
_S|_SP,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,
_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,
_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,
_U,_U,_U,_U,_U,_U,_U,_P,_U,_U,_U,_U,_U,_U,_U,_L,
_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,
_L,_L,_L,_L,_L,_L,_L,_P,_L,_L,_L,_L,_L,_L,_L,_L};

#define __ismask(x) (_ctype[(int)(unsigned char)(x)])

#define isalnum(c)	((__ismask(c)&(_U|_L|_D)) != 0)
#define isalpha(c)	((__ismask(c)&(_U|_L)) != 0)
#define iscntrl(c)	((__ismask(c)&(_C)) != 0)
#define isdigit(c)	((__ismask(c)&(_D)) != 0)
#define isgraph(c)	((__ismask(c)&(_P|_U|_L|_D)) != 0)
#define islower(c)	((__ismask(c)&(_L)) != 0)
#define isprint(c)	((__ismask(c)&(_P|_U|_L|_D|_SP)) != 0)
#define ispunct(c)	((__ismask(c)&(_P)) != 0)
#define isspace(c)	((__ismask(c)&(_S)) != 0)
#define isupper(c)	((__ismask(c)&(_U)) != 0)
#define isxdigit(c)	((__ismask(c)&(_D|_X)) != 0)

#define isascii(c) (((unsigned char)(c))<=0x7f)
#define toascii(c) (((unsigned char)(c))&0x7f)

static inline unsigned char __tolower(unsigned char c)
{
	if (isupper(c))
		c -= 'A'-'a';
	return c;
}

static inline unsigned char __toupper(unsigned char c)
{
	if (islower(c))
		c -= 'a'-'A';
	return c;
}

#define tolower(c) __tolower(c)
#define toupper(c) __toupper(c)

#include <sys/cdefs.h>

typedef void * locale_t;

#endif

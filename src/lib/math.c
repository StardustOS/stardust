/* Copyright (C) 2017, Ward Jaradat
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
 */

/****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: math.c
 *      Author: Rolf Neugebauer (neugebar@dcs.gla.ac.uk)
 *     Changes: 
 *              
 *        Date: Aug 2003
 * 
 * Environment: Xen Minimal OS
 * Description:  Library functions for 64bit arith and other
 *               from freebsd, files in sys/libkern/ (qdivrem.c, etc)
 *
 ****************************************************************************
 * $Id: c-insert.c,v 1.7 2002/11/08 16:04:34 rn Exp $
 ****************************************************************************
 *-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/libkern/divdi3.c,v 1.6 1999/08/28 00:46:31 peter Exp $
 */

#include <os/types.h>

#if !defined(__ia64__)

union uu {
	s64            q;              /* as a (signed) quad */
	s64            uq;             /* as an unsigned quad */
	long           sl[2];          /* as two signed longs */
	unsigned long  ul[2];          /* as two unsigned longs */
};

#define _QUAD_HIGHWORD 1
#define _QUAD_LOWWORD 0

#define H               _QUAD_HIGHWORD
#define L               _QUAD_LOWWORD

#define QUAD_BITS       (sizeof(s64) * CHAR_BIT)
#define LONG_BITS       (sizeof(long) * CHAR_BIT)
#define HALF_BITS       (sizeof(long) * CHAR_BIT / 2)

#define HHALF(x)        ((x) >> HALF_BITS)
#define LHALF(x)        ((x) & ((1UL << HALF_BITS) - 1))
#define LHUP(x)         ((x) << HALF_BITS)

#define	B	(1UL << HALF_BITS)	/* digit base */

#define	COMBINE(a, b) (((u_long)(a) << HALF_BITS) | (b))

#if ULONG_MAX == 0xffffffff && USHRT_MAX >= 0xffff
typedef unsigned short digit;
#else
typedef u_long digit;
#endif

static void shl(register digit *p, register int len, register int sh)
{
	register int i;

	for (i = 0; i < len; i++)
		p[i] = LHALF(p[i] << sh) | (p[i + 1] >> (HALF_BITS - sh));
	p[i] = LHALF(p[i] << sh);
}

u64 __qdivrem(u64 uq, u64 vq, u64 *arq)
{
	union uu tmp;
	digit *u, *v, *q;
	register digit v1, v2;
	u_long qhat, rhat, t;
	int m, n, d, j, i;
	digit uspace[5], vspace[5], qspace[5];

	if (vq == 0) {
		static volatile const unsigned int zero = 0;

		tmp.ul[H] = tmp.ul[L] = 1 / zero;
		if (arq)
			*arq = uq;
		return (tmp.q);
	}
	if (uq < vq) {
		if (arq)
			*arq = uq;
		return (0);
	}
	u = &uspace[0];
	v = &vspace[0];
	q = &qspace[0];

	tmp.uq = uq;
	u[0] = 0;
	u[1] = HHALF(tmp.ul[H]);
	u[2] = LHALF(tmp.ul[H]);
	u[3] = HHALF(tmp.ul[L]);
	u[4] = LHALF(tmp.ul[L]);
	tmp.uq = vq;
	v[1] = HHALF(tmp.ul[H]);
	v[2] = LHALF(tmp.ul[H]);
	v[3] = HHALF(tmp.ul[L]);
	v[4] = LHALF(tmp.ul[L]);
	for (n = 4; v[1] == 0; v++) {
		if (--n == 1) {
			u_long rbj;
			digit q1, q2, q3, q4;

			t = v[2];
			q1 = u[1] / t;
			rbj = COMBINE(u[1] % t, u[2]);
			q2 = rbj / t;
			rbj = COMBINE(rbj % t, u[3]);
			q3 = rbj / t;
			rbj = COMBINE(rbj % t, u[4]);
			q4 = rbj / t;
			if (arq)
				*arq = rbj % t;
			tmp.ul[H] = COMBINE(q1, q2);
			tmp.ul[L] = COMBINE(q3, q4);
			return (tmp.q);
		}
	}

	for (m = 4 - n; u[1] == 0; u++)
		m--;
	for (i = 4 - m; --i >= 0;)
		q[i] = 0;
	q += 4 - m;

	d = 0;
	for (t = v[1]; t < B / 2; t <<= 1)
		d++;
	if (d > 0) {
		shl(&u[0], m + n, d);		/* u <<= d */
		shl(&v[1], n - 1, d);		/* v <<= d */
	}

	j = 0;
	v1 = v[1];
	v2 = v[2];
	do {
		register digit uj0, uj1, uj2;

		uj0 = u[j + 0];	
		uj1 = u[j + 1];	
		uj2 = u[j + 2];
		if (uj0 == v1) {
			qhat = B;
			rhat = uj1;
			goto qhat_too_big;
		} else {
			u_long nn = COMBINE(uj0, uj1);
			qhat = nn / v1;
			rhat = nn % v1;
		}
		while (v2 * qhat > COMBINE(rhat, uj2)) {
			qhat_too_big:
			qhat--;
			if ((rhat += v1) >= B)
				break;
		}

		for (t = 0, i = n; i > 0; i--) {
			t = u[i + j] - v[i] * qhat - t;
			u[i + j] = LHALF(t);
			t = (B - HHALF(t)) & (B - 1);
		}
		t = u[j] - t;
		u[j] = LHALF(t);

		if (HHALF(t)) {
			qhat--;
			for (t = 0, i = n; i > 0; i--) { /* D6: add back. */
				t += u[i + j] + v[i];
				u[i + j] = LHALF(t);
				t = HHALF(t);
			}
			u[j] = LHALF(u[j] + t);
		}
		q[j] = qhat;
	} while (++j <= m);		/* D7: loop on j. */

	if (arq) {
		if (d) {
			for (i = m + n; i > m; --i)
				u[i] = (u[i] >> d) |
				LHALF(u[i - 1] << (HALF_BITS - d));
			u[i] = 0;
		}
		tmp.ul[H] = COMBINE(uspace[1], uspace[2]);
		tmp.ul[L] = COMBINE(uspace[3], uspace[4]);
		*arq = tmp.q;
	}

	tmp.ul[H] = COMBINE(qspace[1], qspace[2]);
	tmp.ul[L] = COMBINE(qspace[3], qspace[4]);
	return (tmp.q);
}

s64 __divdi3(s64 a, s64 b)
{
	u64 ua, ub, uq;
	int neg;

	if (a < 0)
		ua = -(u64)a, neg = 1;
	else
		ua = a, neg = 0;
	if (b < 0)
		ub = -(u64)b, neg ^= 1;
	else
		ub = b;
	uq = __qdivrem(ua, ub, (u64 *)0);
	return (neg ? -uq : uq);
}

u64 __udivdi3(u64 a, u64 b)
{
	return (__qdivrem(a, b, (u64 *)0));
}

u_quad_t __umoddi3(u_quad_t a, u_quad_t b)
{
	u_quad_t r;

	(void)__qdivrem(a, b, &r);
	return (r);
}

#endif

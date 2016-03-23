/*
 *	Multi-precision integer library
 *
 *	Based on XySSL: Copyright (C) 2006-2008	 Christophe Devine
 *
 *	Copyright (C) 2009	Paul Bakker <polarssl_maintainer at polarssl dot org>
 *	Copyright (C) 2010 StackFoundry LLC <yann@stackfoundry.com>
 *	All rights reserved.
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions
 *	are met:
 *
 *	  * Redistributions of source code must retain the above copyright
 *		notice, this list of conditions and the following disclaimer.
 *	  * Redistributions in binary form must reproduce the above copyright
 *		notice, this list of conditions and the following disclaimer in the
 *		documentation and/or other materials provided with the distribution.
 *	  * Neither the names of Tropic SSL,
 *		PolarSSL or XySSL nor the names of its contributors
 *		may be used to endorse or promote products derived from this software
 *		without specific prior written permission.
 *
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *	OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *	This MPI implementation is based on:
 *
 *	http://www.cacr.math.uwaterloo.ca/hac/about/chap14.pdf
 *	http://www.stillhq.com/extracted/gnupg-api/mpi/
 *	http://math.libtomcrypt.com/files/tommath.pdf
 */

#include "secconfig.h"

#if defined(TROPICSSL_BIGNUM_C)

#include "bignum.h"
#include "bn_mul.h"

//#include <string.h>
//#include <stdlib.h>
//#include <stdarg.h>
#include <zc_common.h>
#include "user_config.h"

#define ciL	   ((int) sizeof(t_int))	/* chars in limb  */
#define biL	   (ciL << 3)	/* bits  in limb  */
#define biH	   (ciL << 2)	/* half limb size */

/*
 * Convert between bits/chars and number of limbs
 */
#define BITS_TO_LIMBS(i)  (((i) + biL - 1) / biL)
#define CHARS_TO_LIMBS(i) (((i) + ciL - 1) / ciL)

/*
 * Initialize one MPI
 */
void //ICACHE_FLASH_ATTR
mpi_init(mpi * X)
{
	if (X == NULL) {
		return;
	}
	X->s = 1;
	X->n = 0;
    memset(X->p, 0, MAX_BIGNUM_DIGITS * ciL);
}

/*
 * Unallocate one MPI
 */
void //ICACHE_FLASH_ATTR
mpi_free(mpi * X)
{
	if (X == NULL) {
		return;
	}

	//if (X->p != NULL) {
		memset(X->p, 0, MAX_BIGNUM_DIGITS * ciL);
		//free(X->p);
	//}

	X->s = 1;
	X->n = 0;
	//X->p = NULL;
}
/*
 * Enlarge to the specified number of limbs
 */
int //ICACHE_FLASH_ATTR
mpi_grow(mpi * X, int nblimbs)
{
	/*t_int *p;*/
    if (MAX_BIGNUM_DIGITS < nblimbs)
    {
        return 1;
    }

	if (X->n < nblimbs) {
		/*if ((p = (t_int *) malloc(nblimbs * ciL)) == NULL)
			return (1);*/

		//memset(p, 0, nblimbs * ciL);

		//if (X->p != NULL) {
		//	memcpy(p, X->p, X->n * ciL);
		//	memset(X->p, 0, X->n * ciL);
		//	free(X->p);
		//}
        memset((X->p + X->n), 0, (nblimbs - X->n) * ciL);
		X->n = nblimbs;
	}

	return (0);
}

/*
 * Copy the contents of Y into X
 */
int //ICACHE_FLASH_ATTR
mpi_copy(mpi * X, const mpi * Y)
{
	int ret, i;

	if (X == Y)
		return (0);

	for (i = Y->n - 1; i > 0; i--)
		if (Y->p[i] != 0)
			break;
	i++;

	X->s = Y->s;

	MPI_CHK(mpi_grow(X, i));

	memset(X->p, 0, X->n * ciL);
	memcpy(X->p, Y->p, i * ciL);

cleanup:

	return (ret);
}

/*
 * Swap the contents of X and Y
 */
void //ICACHE_FLASH_ATTR
mpi_swap(mpi * X, mpi * Y)
{
	mpi T;

	memcpy(&T, X, sizeof(mpi));
	memcpy(X, Y, sizeof(mpi));
	memcpy(Y, &T, sizeof(mpi));
}

/*
 * Set value from integer
 */
int //ICACHE_FLASH_ATTR
mpi_lset(mpi * X, int z)
{
	int ret;

	MPI_CHK(mpi_grow(X, 1));
	memset(X->p, 0, X->n * ciL);

	X->p[0] = (z < 0) ? -z : z;
	X->s = (z < 0) ? -1 : 1;

cleanup:

	return (ret);
}

/*
 * Return the number of least significant bits
 */
int //ICACHE_FLASH_ATTR
mpi_lsb(const mpi * X)
{
	int i, j, count = 0;

	for (i = 0; i < X->n; i++)
		for (j = 0; j < (int)biL; j++, count++)
			if (((X->p[i] >> j) & 1) != 0)
				return (count);

	return (0);
}

/*
 * Return the number of most significant bits
 */
int //ICACHE_FLASH_ATTR
mpi_msb(const mpi * X)
{
	int i, j;

	for (i = X->n - 1; i > 0; i--)
		if (X->p[i] != 0)
			break;

	for (j = biL - 1; j >= 0; j--)
		if (((X->p[i] >> j) & 1) != 0)
			break;

	return ((i * biL) + j + 1);
}

/*
 * Return the total size in bytes
 */
int //ICACHE_FLASH_ATTR
mpi_size(const mpi * X)
{
	return ((mpi_msb(X) + 7) >> 3);
}
#ifdef ZC_OFF_LINETEST
/*
 * Convert an ASCII character to digit value
 */
static int //ICACHE_FLASH_ATTR
mpi_get_digit(t_int * d, int radix, char c)
{
	*d = 255;

	if (c >= 0x30 && c <= 0x39)
		*d = c - 0x30;
	if (c >= 0x41 && c <= 0x46)
		*d = c - 0x37;
	if (c >= 0x61 && c <= 0x66)
		*d = c - 0x57;

	if (*d >= (t_int) radix)
		return (TROPICSSL_ERR_MPI_INVALID_CHARACTER);

	return (0);
}


/*
 * Import from an ASCII string
 */
int //ICACHE_FLASH_ATTR
mpi_read_string(mpi * X, int radix, const char *s)
{
	int ret, i, j, n;
	t_int d;
	mpi T;

	if (radix < 2 || radix > 16)
		return (TROPICSSL_ERR_MPI_BAD_INPUT_DATA);

	mpi_init(&T);

	if (radix == 16) {
		n = BITS_TO_LIMBS(strlen(s) << 2);

		MPI_CHK(mpi_grow(X, n));
		MPI_CHK(mpi_lset(X, 0));

		for (i = strlen(s) - 1, j = 0; i >= 0; i--, j++) {
			if (i == 0 && s[i] == '-') {
				X->s = -1;
				break;
			}

			MPI_CHK(mpi_get_digit(&d, radix, s[i]));
			X->p[j / (2 * ciL)] |= d << ((j % (2 * ciL)) << 2);
		}
	} else {
		MPI_CHK(mpi_lset(X, 0));

		for (i = 0; i < (int)strlen(s); i++) {
			if (i == 0 && s[i] == '-') {
				X->s = -1;
				continue;
			}

			MPI_CHK(mpi_get_digit(&d, radix, s[i]));
			MPI_CHK(mpi_mul_int(&T, X, radix));
			MPI_CHK(mpi_add_int(X, &T, d));
		}
	}

cleanup:

	mpi_free(&T);

	return (ret);
}
/*
 * Helper to write the digits high-order first
 */
static int //ICACHE_FLASH_ATTR
mpi_write_hlp(mpi * X, int radix, char **p)
{
	int ret;
	t_int r;

	if (radix < 2 || radix > 16)
		return (TROPICSSL_ERR_MPI_BAD_INPUT_DATA);

	MPI_CHK(mpi_mod_int(&r, X, radix));
	MPI_CHK(mpi_div_int(X, NULL, X, radix));

	if (mpi_cmp_int(X, 0) != 0)
		MPI_CHK(mpi_write_hlp(X, radix, p));

	if (r < 10)
		*(*p)++ = (char)(r + 0x30);
	else
		*(*p)++ = (char)(r + 0x37);

cleanup:

	return (ret);
}

/*
 * Export into an ASCII string
 */
int //ICACHE_FLASH_ATTR
mpi_write_string(const mpi * X, int radix, char *s, int *slen)
{
	int ret = 0, n;
	char *p;
	mpi T;

	if (radix < 2 || radix > 16)
		return (TROPICSSL_ERR_MPI_BAD_INPUT_DATA);

	n = mpi_msb(X);
	if (radix >= 4)
		n >>= 1;
	if (radix >= 16)
		n >>= 1;
	n += 3;

	if (*slen < n) {
		*slen = n;
		return (TROPICSSL_ERR_MPI_BUFFER_TOO_SMALL);
	}

	p = s;
	mpi_init(&T);

	if (X->s == -1)
		*p++ = '-';

	if (radix == 16) {
		int c, i, j, k;

		for (i = X->n - 1, k = 0; i >= 0; i--) {
			for (j = ciL - 1; j >= 0; j--) {
				c = (X->p[i] >> (j << 3)) & 0xFF;

				if (c == 0 && k == 0 && (i + j) != 0)
					continue;

				p += sprintf(p, "%02X", c);
				k = 1;
			}
		}
	} else {
		MPI_CHK(mpi_copy(&T, X));
		MPI_CHK(mpi_write_hlp(&T, radix, &p));
	}

	*p++ = '\0';
	*slen = p - s;

cleanup:

	mpi_free(&T);

	return (ret);
}
#endif
/*
 * Import X from unsigned binary data, big endian
 */
int //ICACHE_FLASH_ATTR
mpi_read_binary(mpi * X, const unsigned char *buf, int buflen)
{
	int ret, i, j, n;

	for (n = 0; n < buflen; n++)
		if (buf[n] != 0)
			break;

	MPI_CHK(mpi_grow(X, CHARS_TO_LIMBS(buflen - n)));
	MPI_CHK(mpi_lset(X, 0));

	for (i = buflen - 1, j = 0; i >= n; i--, j++)
		X->p[j / ciL] |= ((t_int) buf[i]) << ((j % ciL) << 3);

cleanup:

	return (ret);
}

/*
 * Export X into unsigned binary data, big endian
 */
int //ICACHE_FLASH_ATTR
mpi_write_binary(const mpi * X, unsigned char *buf, int buflen)
{
	int i, j, n;

	n = mpi_size(X);

	if (buflen < n)
		return (TROPICSSL_ERR_MPI_BUFFER_TOO_SMALL);

	memset(buf, 0, buflen);

	for (i = buflen - 1, j = 0; n > 0; i--, j++, n--)
		buf[i] = (unsigned char)(X->p[j / ciL] >> ((j % ciL) << 3));

	return (0);
}
/*
 * Left-shift: X <<= count
 */
int ICACHE_FLASH_ATTR
mpi_shift_l(mpi * X, int count)
{
	int ret, i, v0, t1;
	t_int r0 = 0, r1;

	v0 = count / (biL);
	t1 = count & (biL - 1);

	i = mpi_msb(X) + count;

	if (X->n * (int)biL < i)
		MPI_CHK(mpi_grow(X, BITS_TO_LIMBS(i)));

	ret = 0;

	/*
	 * shift by count / limb_size
	 */
	if (v0 > 0) {
		for (i = X->n - 1; i >= v0; i--)
			X->p[i] = X->p[i - v0];

		for (; i >= 0; i--)
			X->p[i] = 0;
	}

	/*
	 * shift by count % limb_size
	 */
	if (t1 > 0) {
		for (i = v0; i < X->n; i++) {
			r1 = X->p[i] >> (biL - t1);
			X->p[i] <<= t1;
			X->p[i] |= r0;
			r0 = r1;
		}
	}

cleanup:

	return (ret);
}

/*
 * Right-shift: X >>= count
 */
int ICACHE_FLASH_ATTR
mpi_shift_r(mpi * X, int count)
{
	int i, v0, v1;
	t_int r0 = 0, r1;

	v0 = count / biL;
	v1 = count & (biL - 1);

	/*
	 * shift by count / limb_size
	 */
	if (v0 > 0) {
		for (i = 0; i < X->n - v0; i++)
			X->p[i] = X->p[i + v0];

		for (; i < X->n; i++)
			X->p[i] = 0;
	}

	/*
	 * shift by count % limb_size
	 */
	if (v1 > 0) {
		for (i = X->n - 1; i >= 0; i--) {
			r1 = X->p[i] << (biL - v1);
			X->p[i] >>= v1;
			X->p[i] |= r0;
			r0 = r1;
		}
	}

	return (0);
}

/*
 * Compare unsigned values
 */
int //ICACHE_FLASH_ATTR
mpi_cmp_abs(const mpi * X, const mpi * Y)
{
	int i, j;

	for (i = X->n - 1; i >= 0; i--)
		if (X->p[i] != 0)
			break;

	for (j = Y->n - 1; j >= 0; j--)
		if (Y->p[j] != 0)
			break;

	if (i < 0 && j < 0)
		return (0);

	if (i > j)
		return (1);
	if (j > i)
		return (-1);

	for (; i >= 0; i--) {
		if (X->p[i] > Y->p[i])
			return (1);
		if (X->p[i] < Y->p[i])
			return (-1);
	}

	return (0);
}

/*
 * Compare signed values
 */
int //ICACHE_FLASH_ATTR
mpi_cmp_mpi(const mpi * X, const mpi * Y)
{
	int i, j;

	for (i = X->n - 1; i >= 0; i--)
		if (X->p[i] != 0)
			break;

	for (j = Y->n - 1; j >= 0; j--)
		if (Y->p[j] != 0)
			break;

	if (i < 0 && j < 0)
		return (0);

	if (i > j)
		return (X->s);
	if (j > i)
		return (-X->s);

	if (X->s > 0 && Y->s < 0)
		return (1);
	if (Y->s > 0 && X->s < 0)
		return (-1);

	for (; i >= 0; i--) {
		if (X->p[i] > Y->p[i])
			return (X->s);
		if (X->p[i] < Y->p[i])
			return (-X->s);
	}

	return (0);
}

/*
 * Compare signed values
 */
int //ICACHE_FLASH_ATTR
mpi_cmp_int(const mpi * X, int z)
{
	mpi Y;
	//t_int p[1];

	//*p = (z < 0) ? -z : z;
	Y.s = (z < 0) ? -1 : 1;
	Y.n = 1;
	Y.p[0] = (z < 0) ? -z : z;

	return (mpi_cmp_mpi(X, &Y));
}

/*
 * Unsigned addition: X = |A| + |B|	 (HAC 14.7)
 */
int //ICACHE_FLASH_ATTR
mpi_add_abs(mpi * X, const mpi * A, const mpi * B)
{
	int ret, i, j;
	const t_int *o; 
	t_int *p, c;

	if (X == B) {
		const mpi *T = A;
		A = X;
		B = T;
	}

	if (X != A)
		MPI_CHK(mpi_copy(X, A));

	for (j = B->n - 1; j >= 0; j--)
		if (B->p[j] != 0)
			break;

	MPI_CHK(mpi_grow(X, j + 1));

	o = B->p;
	p = X->p;
	c = 0;

	for (i = 0; i <= j; i++, o++, p++) {
		*p += c;
		c = (*p < c);
		*p += *o;
		c += (*p < *o);
	}

	while (c != 0) {
		if (i >= X->n) {
			MPI_CHK(mpi_grow(X, i + 1));
			p = X->p + i;
		}

		*p += c;
		c = (*p < c);
		i++;
	}

cleanup:

	return (ret);
}

/*
 * Helper for mpi substraction
 */
static void //ICACHE_FLASH_ATTR
mpi_sub_hlp(int n, t_int * s, t_int * d)
{
	int i;
	t_int c, z;

	for (i = c = 0; i < n; i++, s++, d++) {
		z = (*d < c);
		*d -= c;
		c = (*d < *s) + z;
		*d -= *s;
	}

	while (c != 0) {
		z = (*d < c);
		*d -= c;
		c = z;
		i++;
		d++;
	}
}

/*
 * Unsigned substraction: X = |A| - |B|	 (HAC 14.9)
 */
int //ICACHE_FLASH_ATTR
mpi_sub_abs(mpi * X, const mpi * A, const mpi * B)
{
	mpi TB;
	int ret, n;

	if (mpi_cmp_abs(A, B) < 0)
		return (TROPICSSL_ERR_MPI_NEGATIVE_VALUE);

	mpi_init(&TB);

	if (X == B) {
		MPI_CHK(mpi_copy(&TB, B));
		B = &TB;
	}

	if (X != A)
		MPI_CHK(mpi_copy(X, A));

	ret = 0;

	for (n = B->n - 1; n >= 0; n--)
		if (B->p[n] != 0)
			break;

	mpi_sub_hlp(n + 1, (t_int *)B->p, (t_int *)X->p);

cleanup:

	mpi_free(&TB);

	return (ret);
}

/*
 * Signed addition: X = A + B
 */
int //ICACHE_FLASH_ATTR
mpi_add_mpi(mpi * X, const mpi * A, const mpi * B)
{
	int ret, s = A->s;

	if (A->s * B->s < 0) {
		if (mpi_cmp_abs(A, B) >= 0) {
			MPI_CHK(mpi_sub_abs(X, A, B));
			X->s = s;
		} else {
			MPI_CHK(mpi_sub_abs(X, B, A));
			X->s = -s;
		}
	} else {
		MPI_CHK(mpi_add_abs(X, A, B));
		X->s = s;
	}

cleanup:

	return (ret);
}

/*
 * Signed substraction: X = A - B
 */
int //ICACHE_FLASH_ATTR
mpi_sub_mpi(mpi * X, const mpi * A, const mpi * B)
{
	int ret, s = A->s;

	if (A->s * B->s > 0) {
		if (mpi_cmp_abs(A, B) >= 0) {
			MPI_CHK(mpi_sub_abs(X, A, B));
			X->s = s;
		} else {
			MPI_CHK(mpi_sub_abs(X, B, A));
			X->s = -s;
		}
	} else {
		MPI_CHK(mpi_add_abs(X, A, B));
		X->s = s;
	}

cleanup:

	return (ret);
}

/*
 * Signed addition: X = A + b
 */
int //ICACHE_FLASH_ATTR
mpi_add_int(mpi * X, const mpi * A, int b)
{
	mpi _B;
	//t_int p[1];

	//p[0] = (b < 0) ? -b : b;
	_B.s = (b < 0) ? -1 : 1;
	_B.n = 1;
	_B.p[0] = (b < 0) ? -b : b;

	return (mpi_add_mpi(X, A, &_B));
}

/*
 * Signed substraction: X = A - b
 */
int //ICACHE_FLASH_ATTR
mpi_sub_int(mpi * X, const mpi * A, int b)
{
	mpi _B;
	//t_int p[1];

	//p[0] = (b < 0) ? -b : b;
	_B.s = (b < 0) ? -1 : 1;
	_B.n = 1;
	_B.p[0] = (b < 0) ? -b : b;

	return (mpi_sub_mpi(X, A, &_B));
}

/*
 * Helper for mpi multiplication
 */
static void //ICACHE_FLASH_ATTR
mpi_mul_hlp(int i, t_int * s, t_int * d, t_int b)
{
	t_int c = 0, t = 0;

#if defined(MULADDC_HUIT)
	for (; i >= 8; i -= 8) {
	MULADDC_INIT MULADDC_HUIT MULADDC_STOP} for (; i > 0; i--) {
	MULADDC_INIT MULADDC_CORE MULADDC_STOP}
#else
	for (; i >= 16; i -= 16) {
	MULADDC_INIT
		    MULADDC_CORE MULADDC_CORE
		    MULADDC_CORE MULADDC_CORE
		    MULADDC_CORE MULADDC_CORE
		    MULADDC_CORE MULADDC_CORE
		    MULADDC_CORE MULADDC_CORE
		    MULADDC_CORE MULADDC_CORE
		    MULADDC_CORE MULADDC_CORE
		    MULADDC_CORE MULADDC_CORE MULADDC_STOP}

	for (; i >= 8; i -= 8) {
	MULADDC_INIT
		    MULADDC_CORE MULADDC_CORE
		    MULADDC_CORE MULADDC_CORE
		    MULADDC_CORE MULADDC_CORE
		    MULADDC_CORE MULADDC_CORE MULADDC_STOP}

	for (; i > 0; i--) {
	MULADDC_INIT MULADDC_CORE MULADDC_STOP}
#endif
	t++;

	do {
		*d += c;
		c = (*d < c);
		d++;
	} while (c != 0);
}

/*
 * Baseline multiplication: X = A * B  (HAC 14.12)
 */
int ICACHE_FLASH_ATTR
mpi_mul_mpi(mpi * X, const mpi * A, const mpi * B)
{
	int ret, i, j;
	mpi TA, TB;

	mpi_init(&TA); mpi_init(&TB);

	if (X == A) {
		MPI_CHK(mpi_copy(&TA, A));
		A = &TA;
	}
	if (X == B) {
		MPI_CHK(mpi_copy(&TB, B));
		B = &TB;
	}

	for (i = A->n - 1; i >= 0; i--)
		if (A->p[i] != 0)
			break;

	for (j = B->n - 1; j >= 0; j--)
		if (B->p[j] != 0)
			break;

	MPI_CHK(mpi_grow(X, i + j + 2));
	MPI_CHK(mpi_lset(X, 0));

	for (i++; j >= 0; j--)
		mpi_mul_hlp(i, (t_int *)A->p, X->p + j, B->p[j]);

	X->s = A->s * B->s;

cleanup:

	mpi_free(&TB); mpi_free(&TA);

	return (ret);
}

/*
 * Baseline multiplication: X = A * b
 */
int //ICACHE_FLASH_ATTR
mpi_mul_int(mpi * X, const mpi * A, t_int b)
{
	mpi _B;
	//t_int p[1];

	_B.s = 1;
	_B.n = 1;
	_B.p[0] = b;
	//p[0] = b;

	return (mpi_mul_mpi(X, A, &_B));
}

/*
 * Division by mpi: A = Q * B + R  (HAC 14.20)
 */
mpi X, Y, Z, O_T1, O_T2;

int //ICACHE_FLASH_ATTR
mpi_div_mpi(mpi * Q, mpi * R, const mpi * A, const mpi * B)
{
	int ret, i, n, t, k;

	if (mpi_cmp_int(B, 0) == 0)
		return (TROPICSSL_ERR_MPI_DIVISION_BY_ZERO);

	mpi_init(&X); mpi_init(&Y); mpi_init(&Z);
	mpi_init(&O_T1); mpi_init(&O_T2);

	if (mpi_cmp_abs(A, B) < 0) {
		if (Q != NULL)
			MPI_CHK(mpi_lset(Q, 0));
		if (R != NULL)
			MPI_CHK(mpi_copy(R, A));
		return (0);
	}

	MPI_CHK(mpi_copy(&X, A));
	MPI_CHK(mpi_copy(&Y, B));
	X.s = Y.s = 1;

	MPI_CHK(mpi_grow(&Z, A->n + 2));
	MPI_CHK(mpi_lset(&Z, 0));
	MPI_CHK(mpi_grow(&O_T1, 2));
	MPI_CHK(mpi_grow(&O_T2, 3));

	k = mpi_msb(&Y) % biL;
	if (k < (int)biL - 1) {
		k = biL - 1 - k;
		MPI_CHK(mpi_shift_l(&X, k));
		MPI_CHK(mpi_shift_l(&Y, k));
	} else
		k = 0;

	n = X.n - 1;
	t = Y.n - 1;
	mpi_shift_l(&Y, biL * (n - t));

	while (mpi_cmp_mpi(&X, &Y) >= 0) {
		Z.p[n - t]++;
		mpi_sub_mpi(&X, &X, &Y);
	}
	mpi_shift_r(&Y, biL * (n - t));

	for (i = n; i > t; i--) {
		if (X.p[i] >= Y.p[t])
			Z.p[i - t - 1] = ~0;
		else {
#if defined(TROPICSSL_HAVE_LONGLONG)
			t_dbl r;

			r = (t_dbl) X.p[i] << biL;
			r |= (t_dbl) X.p[i - 1];
			r /= Y.p[t];
			if (r > ((t_dbl) 1 << biL) - 1)
				r = ((t_dbl) 1 << biL) - 1;

			Z.p[i - t - 1] = (t_int) r;
#else
			/*
			 * __udiv_qrnnd_c, from gmp/longlong.h
			 */
			t_int q0, q1, r0, r1;
			t_int d0, d1, d, m;

			d = Y.p[t];
			d0 = (d << biH) >> biH;
			d1 = (d >> biH);

			q1 = X.p[i] / d1;
			r1 = X.p[i] - d1 * q1;
			r1 <<= biH;
			r1 |= (X.p[i - 1] >> biH);

			m = q1 * d0;
			if (r1 < m) {
				q1--, r1 += d;
				while (r1 >= d && r1 < m)
					q1--, r1 += d;
			}
			r1 -= m;

			q0 = r1 / d1;
			r0 = r1 - d1 * q0;
			r0 <<= biH;
			r0 |= (X.p[i - 1] << biH) >> biH;

			m = q0 * d0;
			if (r0 < m) {
				q0--, r0 += d;
				while (r0 >= d && r0 < m)
					q0--, r0 += d;
			}
			r0 -= m;

			Z.p[i - t - 1] = (q1 << biH) | q0;
#endif
		}

		Z.p[i - t - 1]++;
		do {
			Z.p[i - t - 1]--;

			MPI_CHK(mpi_lset(&O_T1, 0));
			O_T1.p[0] = (t < 1) ? 0 : Y.p[t - 1];
			O_T1.p[1] = Y.p[t];
			MPI_CHK(mpi_mul_int(&O_T1, &O_T1, Z.p[i - t - 1]));

			MPI_CHK(mpi_lset(&O_T2, 0));
			O_T2.p[0] = (i < 2) ? 0 : X.p[i - 2];
			O_T2.p[1] = (i < 1) ? 0 : X.p[i - 1];
			O_T2.p[2] = X.p[i];
		} while (mpi_cmp_mpi(&O_T1, &O_T2) > 0);

		MPI_CHK(mpi_mul_int(&O_T1, &Y, Z.p[i - t - 1]));
		MPI_CHK(mpi_shift_l(&O_T1, biL * (i - t - 1)));
		MPI_CHK(mpi_sub_mpi(&X, &X, &O_T1));

		if (mpi_cmp_int(&X, 0) < 0) {
			MPI_CHK(mpi_copy(&O_T1, &Y));
			MPI_CHK(mpi_shift_l(&O_T1, biL * (i - t - 1)));
			MPI_CHK(mpi_add_mpi(&X, &X, &O_T1));
			Z.p[i - t - 1]--;
		}
	}

	if (Q != NULL) {
		mpi_copy(Q, &Z);
		Q->s = A->s * B->s;
	}

	if (R != NULL) {
		mpi_shift_r(&X, k);
		mpi_copy(R, &X);

		R->s = A->s;
		if (mpi_cmp_int(R, 0) == 0)
			R->s = 1;
	}

cleanup:

	mpi_free(&X); mpi_free(&Y); mpi_free(&Z);
	mpi_free(&O_T1); mpi_free(&O_T2);

	return (ret);
}

/*
 * Division by int: A = Q * b + R
 *
 * Returns 0 if successful
 *		   1 if memory allocation failed
 *		   TROPICSSL_ERR_MPI_DIVISION_BY_ZERO if b == 0
 */
int //ICACHE_FLASH_ATTR
mpi_div_int(mpi * Q, mpi * R, const mpi * A, int b)
{
	mpi _B;
	//t_int p[1];

	//p[0] = (b < 0) ? -b : b;
	_B.s = (b < 0) ? -1 : 1;
	_B.n = 1;
	_B.p[0] = (b < 0) ? -b : b;

	return (mpi_div_mpi(Q, R, A, &_B));
}

/*
 * Modulo: R = A mod B
 */
int //ICACHE_FLASH_ATTR
mpi_mod_mpi(mpi * R, const mpi * A, const mpi * B)
{
	int ret;

	MPI_CHK(mpi_div_mpi(NULL, R, A, B));

	while (mpi_cmp_int(R, 0) < 0)
		MPI_CHK(mpi_add_mpi(R, R, B));

	while (mpi_cmp_mpi(R, B) >= 0)
		MPI_CHK(mpi_sub_mpi(R, R, B));

cleanup:

	return (ret);
}

/*
 * Modulo: r = A mod b
 */
int //ICACHE_FLASH_ATTR
mpi_mod_int(t_int * r, const mpi * A, int b)
{
	int i;
	t_int x, y, z;

	if (b == 0)
		return (TROPICSSL_ERR_MPI_DIVISION_BY_ZERO);

	if (b < 0)
		b = -b;

	/*
	 * handle trivial cases
	 */
	if (b == 1) {
		*r = 0;
		return (0);
	}

	if (b == 2) {
		*r = A->p[0] & 1;
		return (0);
	}

	/*
	 * general case
	 */
	for (i = A->n - 1, y = 0; i >= 0; i--) {
		x = A->p[i];
		y = (y << biH) | (x >> biH);
		z = y / b;
		y -= z * b;

		x <<= biH;
		y = (y << biH) | (x >> biH);
		z = y / b;
		y -= z * b;
	}

	*r = y;

	return (0);
}

/*
 * Fast Montgomery initialization (thanks to Tom St Denis)
 */
static void //ICACHE_FLASH_ATTR
mpi_montg_init(t_int * mm, const mpi * N)
{
	t_int x, m0 = N->p[0];

	x = m0;
	x += ((m0 + 2) & 4) << 1;
	x *= (2 - (m0 * x));

	if (biL >= 16)
		x *= (2 - (m0 * x));
	if (biL >= 32)
		x *= (2 - (m0 * x));
	if (biL >= 64)
		x *= (2 - (m0 * x));

	*mm = ~x + 1;
}

/*
 * Montgomery multiplication: A = A * B * R^-1 mod N  (HAC 14.36)
 */
static void //ICACHE_FLASH_ATTR
mpi_montmul(mpi * A, const mpi * B, const mpi * N, t_int mm, mpi * T)
{
	int i, n, m;
	t_int u0, u1, *d;

	memset(T->p, 0, T->n * ciL);

	d = T->p;
	n = N->n;
	m = (B->n < n) ? B->n : n;

	for (i = 0; i < n; i++) {
		/*
		 * T = (T + u0*B + u1*N) / 2^biL
		 */
		u0 = A->p[i];
		u1 = (d[0] + u0 * B->p[0]) * mm;

		mpi_mul_hlp(m, (t_int *)B->p, d, u0);
		mpi_mul_hlp(n, (t_int *)N->p, d, u1);

		*d++ = u0;
		d[n + 1] = 0;
	}

	memcpy(A->p, d, (n + 1) * ciL);

	if (mpi_cmp_abs(A, N) >= 0)
		mpi_sub_hlp(n, (t_int *)N->p, A->p);
	else
		/* prevent timing attacks */
		mpi_sub_hlp(n, A->p, T->p);
}

/*
 * Montgomery reduction: A = A * R^-1 mod N
 */
static void //ICACHE_FLASH_ATTR
mpi_montred(mpi * A, const mpi * N, t_int mm, mpi * T)
{
	t_int z = 1;
	mpi U;

	U.n = U.s = z;
	//U.p = &z;
	U.p[0] = z;

	mpi_montmul(A, &U, N, mm, T);
}

/*
 * Sliding-window exponentiation: X = A^E mod N	 (HAC 14.85)
 */
mpi O_RR, O_T, O_W[64];

int //ICACHE_FLASH_ATTR
mpi_exp_mod(mpi * X, const mpi * A, const mpi * E, const mpi * N, mpi * _RR)
{
	int ret, i, j, wsize, wbits;
	int bufsize, nblimbs, nbits;
	t_int ei, mm, state;

	if (mpi_cmp_int(N, 0) < 0 || (N->p[0] & 1) == 0)
		return (TROPICSSL_ERR_MPI_BAD_INPUT_DATA);

	/*
	 * Init temps and window size
	 */
	mpi_montg_init(&mm, N);
	mpi_init(&O_RR); mpi_init(&O_T);
	memset(O_W, 0, sizeof(O_W));

	i = mpi_msb(E);

	wsize = (i > 671) ? 6 : (i > 239) ? 5 : (i > 79) ? 4 : (i > 23) ? 3 : 1;

	j = N->n + 1;
	MPI_CHK(mpi_grow(X, j));
	
	MPI_CHK(mpi_grow(&O_W[1], j));

	MPI_CHK(mpi_grow(&O_T, j * 2));

	/*
	 * If 1st call, pre-compute R^2 mod N
	 */
	if (_RR == NULL || _RR->n == 0/*_RR->p == NULL*/) {
		MPI_CHK(mpi_lset(&O_RR, 1));
		MPI_CHK(mpi_shift_l(&O_RR, N->n * 2 * biL));
		MPI_CHK(mpi_mod_mpi(&O_RR, &O_RR, N));

		if (_RR != NULL)
		{
			memcpy(_RR, &O_RR, sizeof(mpi));
		}
	} else
	{
       	memcpy(&O_RR, _RR, sizeof(mpi));
	}
	

	/*
	 * W[1] = A * R^2 * R^-1 mod N = A * R mod N
	 */
	if (mpi_cmp_mpi(A, N) >= 0)
		mpi_mod_mpi(&O_W[1], A, N);
	else
		mpi_copy(&O_W[1], A);

	mpi_montmul(&O_W[1], &O_RR, N, mm, &O_T);
	/*
	 * X = R^2 * R^-1 mod N = R mod N
	 */
	MPI_CHK(mpi_copy(X, &O_RR));
	mpi_montred(X, N, mm, &O_T);

	if (wsize > 1) {
		/*
		 * W[1 << (wsize - 1)] = W[1] ^ (wsize - 1)
		 */
		j = 1 << (wsize - 1);

		MPI_CHK(mpi_grow(&O_W[j], N->n + 1));
		MPI_CHK(mpi_copy(&O_W[j], &O_W[1]));

		for (i = 0; i < wsize - 1; i++)
			mpi_montmul(&O_W[j], &O_W[j], N, mm, &O_T);

		/*
		 * W[i] = W[i - 1] * W[1]
		 */
		for (i = j + 1; i < (1 << wsize); i++) {
			MPI_CHK(mpi_grow(&O_W[i], N->n + 1));
			MPI_CHK(mpi_copy(&O_W[i], &O_W[i - 1]));

			mpi_montmul(&O_W[i], &O_W[1], N, mm, &O_T);
		}
	}

	nblimbs = E->n;
	bufsize = 0;
	nbits = 0;
	wbits = 0;
	state = 0;

	while (1) {
		if (bufsize == 0) {
			if (nblimbs-- == 0)
				break;

			bufsize = sizeof(t_int) << 3;
		}

		bufsize--;

		ei = (E->p[nblimbs] >> bufsize) & 1;

		/*
		 * skip leading 0s
		 */
		if (ei == 0 && state == 0)
			continue;

		if (ei == 0 && state == 1) {
			/*
			 * out of window, square X
			 */
			mpi_montmul(X, X, N, mm, &O_T);
			continue;
		}

		/*
		 * add ei to current window
		 */
		state = 2;

		nbits++;
		wbits |= (ei << (wsize - nbits));

		if (nbits == wsize) {
			/*
			 * X = X^wsize R^-1 mod N
			 */
			for (i = 0; i < wsize; i++)
				mpi_montmul(X, X, N, mm, &O_T);

			/*
			 * X = X * W[wbits] R^-1 mod N
			 */
			mpi_montmul(X, &O_W[wbits], N, mm, &O_T);

			state--;
			nbits = 0;
			wbits = 0;
		}
	}

	/*
	 * process the remaining bits
	 */
	for (i = 0; i < nbits; i++) {
		mpi_montmul(X, X, N, mm, &O_T);

		wbits <<= 1;

		if ((wbits & (1 << wsize)) != 0)
			mpi_montmul(X, &O_W[1], N, mm, &O_T);
	}

	/*
	 * X = A^E * R * R^-1 mod N = A^E mod N
	 */
	mpi_montred(X, N, mm, &O_T);

cleanup:

	for (i = (1 << (wsize - 1)); i < (1 << wsize); i++)
		mpi_free(&O_W[i]);

	mpi_free(&O_W[1]); mpi_free(&O_T);
	if (_RR == NULL) {
		mpi_free(&O_RR);
	}

	return (ret);
}


/*
 * Greatest common divisor: G = gcd(A, B)  (HAC 14.54)
 */
int //ICACHE_FLASH_ATTR
mpi_gcd(mpi * G, const mpi * A, const mpi * B)
{
	int ret, lz, lzt;
	mpi TG, TA, TB;

	mpi_init(&TG); mpi_init(&TA); mpi_init(&TB);

	MPI_CHK(mpi_copy(&TA, A));
	MPI_CHK(mpi_copy(&TB, B));

	lz = mpi_lsb(&TA);
	lzt = mpi_lsb(&TB);

	if (lzt < lz)
		lz = lzt;

	MPI_CHK(mpi_shift_r(&TA, lz));
	MPI_CHK(mpi_shift_r(&TB, lz));

	TA.s = TB.s = 1;

	while (mpi_cmp_int(&TA, 0) != 0) {
		MPI_CHK(mpi_shift_r(&TA, mpi_lsb(&TA)));
		MPI_CHK(mpi_shift_r(&TB, mpi_lsb(&TB)));

		if (mpi_cmp_mpi(&TA, &TB) >= 0) {
			MPI_CHK(mpi_sub_abs(&TA, &TA, &TB));
			MPI_CHK(mpi_shift_r(&TA, 1));
		} else {
			MPI_CHK(mpi_sub_abs(&TB, &TB, &TA));
			MPI_CHK(mpi_shift_r(&TB, 1));
		}
	}

	MPI_CHK(mpi_shift_l(&TB, lz));
	MPI_CHK(mpi_copy(G, &TB));

cleanup:

	mpi_free(&TB); mpi_free(&TA); mpi_free(&TG);

	return (ret);
}

#if defined(TROPICSSL_GENPRIME)

/*
 * Modular inverse: X = A^-1 mod N	(HAC 14.61 / 14.64)
 */
int //ICACHE_FLASH_ATTR
mpi_inv_mod(mpi * X, const mpi * A, const mpi * N)
{
	int ret;
	mpi G, TA, TU, U1, U2, TB, TV, V1, V2;

	if (mpi_cmp_int(N, 0) <= 0)
		return (TROPICSSL_ERR_MPI_BAD_INPUT_DATA);

	mpi_init(&TA); mpi_init(&TU); mpi_init(&U1); mpi_init(&U2);
	mpi_init(&G); mpi_init(&TB); mpi_init(&TV);
	mpi_init(&V1); mpi_init(&V2);

	MPI_CHK(mpi_gcd(&G, A, N));

	if (mpi_cmp_int(&G, 1) != 0) {
		ret = TROPICSSL_ERR_MPI_NOT_ACCEPTABLE;
		goto cleanup;
	}

	MPI_CHK(mpi_mod_mpi(&TA, A, N));
	MPI_CHK(mpi_copy(&TU, &TA));
	MPI_CHK(mpi_copy(&TB, N));
	MPI_CHK(mpi_copy(&TV, N));

	MPI_CHK(mpi_lset(&U1, 1));
	MPI_CHK(mpi_lset(&U2, 0));
	MPI_CHK(mpi_lset(&V1, 0));
	MPI_CHK(mpi_lset(&V2, 1));

	do {
		while ((TU.p[0] & 1) == 0) {
			MPI_CHK(mpi_shift_r(&TU, 1));

			if ((U1.p[0] & 1) != 0 || (U2.p[0] & 1) != 0) {
				MPI_CHK(mpi_add_mpi(&U1, &U1, &TB));
				MPI_CHK(mpi_sub_mpi(&U2, &U2, &TA));
			}

			MPI_CHK(mpi_shift_r(&U1, 1));
			MPI_CHK(mpi_shift_r(&U2, 1));
		}

		while ((TV.p[0] & 1) == 0) {
			MPI_CHK(mpi_shift_r(&TV, 1));

			if ((V1.p[0] & 1) != 0 || (V2.p[0] & 1) != 0) {
				MPI_CHK(mpi_add_mpi(&V1, &V1, &TB));
				MPI_CHK(mpi_sub_mpi(&V2, &V2, &TA));
			}

			MPI_CHK(mpi_shift_r(&V1, 1));
			MPI_CHK(mpi_shift_r(&V2, 1));
		}

		if (mpi_cmp_mpi(&TU, &TV) >= 0) {
			MPI_CHK(mpi_sub_mpi(&TU, &TU, &TV));
			MPI_CHK(mpi_sub_mpi(&U1, &U1, &V1));
			MPI_CHK(mpi_sub_mpi(&U2, &U2, &V2));
		} else {
			MPI_CHK(mpi_sub_mpi(&TV, &TV, &TU));
			MPI_CHK(mpi_sub_mpi(&V1, &V1, &U1));
			MPI_CHK(mpi_sub_mpi(&V2, &V2, &U2));
		}
	} while (mpi_cmp_int(&TU, 0) != 0);

	while (mpi_cmp_int(&V1, 0) < 0)
		MPI_CHK(mpi_add_mpi(&V1, &V1, N));

	while (mpi_cmp_mpi(&V1, N) >= 0)
		MPI_CHK(mpi_sub_mpi(&V1, &V1, N));

	MPI_CHK(mpi_copy(X, &V1));

cleanup:

	mpi_free(&V2); mpi_free(&V1); mpi_free(&TV); mpi_free(&TB);
	mpi_free(&G); mpi_free(&U2); mpi_free(&U1);
	mpi_free(&TU); mpi_free(&TA);

	return (ret);
}

static const int small_prime[] = {
	3, 5, 7, 11, 13, 17, 19, 23,
	29, 31, 37, 41, 43, 47, 53, 59,
	61, 67, 71, 73, 79, 83, 89, 97,
	101, 103, 107, 109, 113, 127, 131, 137,
	139, 149, 151, 157, 163, 167, 173, 179,
	181, 191, 193, 197, 199, 211, 223, 227,
	229, 233, 239, 241, 251, 257, 263, 269,
	271, 277, 281, 283, 293, 307, 311, 313,
	317, 331, 337, 347, 349, 353, 359, 367,
	373, 379, 383, 389, 397, 401, 409, 419,
	421, 431, 433, 439, 443, 449, 457, 461,
	463, 467, 479, 487, 491, 499, 503, 509,
	521, 523, 541, 547, 557, 563, 569, 571,
	577, 587, 593, 599, 601, 607, 613, 617,
	619, 631, 641, 643, 647, 653, 659, 661,
	673, 677, 683, 691, 701, 709, 719, 727,
	733, 739, 743, 751, 757, 761, 769, 773,
	787, 797, 809, 811, 821, 823, 827, 829,
	839, 853, 857, 859, 863, 877, 881, 883,
	887, 907, 911, 919, 929, 937, 941, 947,
	953, 967, 971, 977, 983, 991, 997, -103
};

/*
 * Miller-Rabin primality test	(HAC 4.24)
 */
int //ICACHE_FLASH_ATTR
mpi_is_prime(mpi * X, int (*f_rng) (void *), void *p_rng)
{
	int ret, i, j, n, s, xs;
	mpi W, R, T, A, RR;
	unsigned char *p;

	if (mpi_cmp_int(X, 0) == 0)
		return (0);

	mpi_init(&W); mpi_init(&R); mpi_init(&T);
	mpi_init(&A); mpi_init(&RR);

	xs = X->s;
	X->s = 1;

	/*
	 * test trivial factors first
	 */
	if ((X->p[0] & 1) == 0)
		return (TROPICSSL_ERR_MPI_NOT_ACCEPTABLE);

	for (i = 0; small_prime[i] > 0; i++) {
		t_int r;

		if (mpi_cmp_int(X, small_prime[i]) <= 0)
			return (0);

		MPI_CHK(mpi_mod_int(&r, X, small_prime[i]));

		if (r == 0)
			return (TROPICSSL_ERR_MPI_NOT_ACCEPTABLE);
	}

	/*
	 * W = |X| - 1
	 * R = W >> lsb( W )
	 */
	s = mpi_lsb(&W);
	MPI_CHK(mpi_sub_int(&W, X, 1));
	MPI_CHK(mpi_copy(&R, &W));
	MPI_CHK(mpi_shift_r(&R, s));

	i = mpi_msb(X);
	/*
	 * HAC, table 4.4
	 */
	n = ((i >= 1300) ? 2 : (i >= 850) ? 3 :
	     (i >= 650) ? 4 : (i >= 350) ? 8 :
	     (i >= 250) ? 12 : (i >= 150) ? 18 : 27);

	for (i = 0; i < n; i++) {
		/*
		 * pick a random A, 1 < A < |X| - 1
		 */
		MPI_CHK(mpi_grow(&A, X->n));

		p = (unsigned char *)A.p;
		for (j = 0; j < A.n * ciL; j++)
			*p++ = (unsigned char)f_rng(p_rng);

		j = mpi_msb(&A) - mpi_msb(&W);
		MPI_CHK(mpi_shift_r(&A, j + 1));
		A.p[0] |= 3;

		/*
		 * A = A^R mod |X|
		 */
		MPI_CHK(mpi_exp_mod(&A, &A, &R, X, &RR));

		if (mpi_cmp_mpi(&A, &W) == 0 || mpi_cmp_int(&A, 1) == 0)
			continue;

		j = 1;
		while (j < s && mpi_cmp_mpi(&A, &W) != 0) {
			/*
			 * A = A * A mod |X|
			 */
			MPI_CHK(mpi_mul_mpi(&T, &A, &A));
			MPI_CHK(mpi_mod_mpi(&A, &T, X));

			if (mpi_cmp_int(&A, 1) == 0)
				break;

			j++;
		}

		/*
		 * not prime if A != |X| - 1 or A == 1
		 */
		if (mpi_cmp_mpi(&A, &W) != 0 || mpi_cmp_int(&A, 1) == 0) {
			ret = TROPICSSL_ERR_MPI_NOT_ACCEPTABLE;
			break;
		}
	}

cleanup:

	X->s = xs;

	mpi_free(&RR); mpi_free(&A); mpi_free(&T);
	mpi_free(&R); mpi_free(&W);

	return (ret);
}

/*
 * Prime number generation
 */
int //ICACHE_FLASH_ATTR
mpi_gen_prime(mpi * X, int nbits, int dh_flag,
		  int (*f_rng) (void *), void *p_rng)
{
	int ret, k, n;
	unsigned char *p;
	mpi Y;

	if (nbits < 3)
		return (TROPICSSL_ERR_MPI_BAD_INPUT_DATA);

	mpi_init(&Y);

	n = BITS_TO_LIMBS(nbits);

	MPI_CHK(mpi_grow(X, n));
	MPI_CHK(mpi_lset(X, 0));

	p = (unsigned char *)X->p;
	for (k = 0; k < X->n * ciL; k++)
		*p++ = (unsigned char)f_rng(p_rng);

	k = mpi_msb(X);
	if (k < nbits)
		MPI_CHK(mpi_shift_l(X, nbits - k));
	if (k > nbits)
		MPI_CHK(mpi_shift_r(X, k - nbits));

	X->p[0] |= 3;

	if (dh_flag == 0) {
		while ((ret = mpi_is_prime(X, f_rng, p_rng)) != 0) {
			if (ret != TROPICSSL_ERR_MPI_NOT_ACCEPTABLE)
				goto cleanup;

			MPI_CHK(mpi_add_int(X, X, 2));
		}
	} else {
		MPI_CHK(mpi_sub_int(&Y, X, 1));
		MPI_CHK(mpi_shift_r(&Y, 1));

		while (1) {
			if ((ret = mpi_is_prime(X, f_rng, p_rng)) == 0) {
				if ((ret = mpi_is_prime(&Y, f_rng, p_rng)) == 0)
					break;

				if (ret != TROPICSSL_ERR_MPI_NOT_ACCEPTABLE)
					goto cleanup;
			}

			if (ret != TROPICSSL_ERR_MPI_NOT_ACCEPTABLE)
				goto cleanup;

			MPI_CHK(mpi_add_int(&Y, X, 1));
			MPI_CHK(mpi_add_int(X, X, 2));
			MPI_CHK(mpi_shift_r(&Y, 1));
		}
	}

cleanup:

	mpi_free(&Y);

	return (ret);
}

#endif
#endif
/*
	mpg123lib_intern: Common non-public stuff for libmpg123

	copyright 1995-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	derived from the old mpg123.h
*/

#ifndef MPG123_H_INTERN
#define MPG123_H_INTERN

#define MPG123_RATES 9
#define MPG123_ENCODINGS 10

#include "config.h" /* Load this before _anything_ */

/* ABI conformance for other compilers.
   mpg123 needs 16byte-aligned stack for SSE and friends.
   gcc provides that, but others don't necessarily. */
#ifdef ABI_ALIGN_FUN
#ifndef attribute_align_arg
#if defined(__GNUC__) && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__>1)
#    define attribute_align_arg __attribute__((force_align_arg_pointer))
/* The gcc that can align the stack does not need the check... nor does it work with gcc 4.3+, anyway. */
#else
#    define attribute_align_arg
#    define NEED_ALIGNCHECK /* Other compilers get code to catch misaligned stack. */
#endif
#endif
#else
#define attribute_align_arg
/* We won't try the align check... */
#endif

/* export DLL symbols */
#if defined(WIN32) && defined(DYNAMIC_BUILD)
#define BUILD_MPG123_DLL
#endif
#include "compat.h"
#include "mpg123.h"

#define SKIP_JUNK 1

#ifndef M_PI
# define M_PI       3.14159265358979323846
#endif
#ifndef M_SQRT2
# define M_SQRT2	1.41421356237309504880
#endif

#ifdef SUNOS
#define memmove(dst,src,size) bcopy(src,dst,size)
#endif

/* some stuff has to go back to mpg123.h */
#ifdef REAL_IS_FLOAT
#  define real float
#  define REAL_SCANF "%f"
#  define REAL_PRINTF "%f"
#elif defined(REAL_IS_LONG_DOUBLE)
#  define real long double
#  define REAL_SCANF "%Lf"
#  define REAL_PRINTF "%Lf"
#elif defined(REAL_IS_FIXED)
/* Disable some output formats for fixed point decoder... */

# define real long

# define REAL_RADIX            15
# define REAL_FACTOR           (32.0 * 1024.0)

/* I just changed the (int) to (long) there... seemed right. */
# define DOUBLE_TO_REAL(x)					((long)((x) * REAL_FACTOR))
# define DOUBLE_TO_REAL_POW43(x)			((long)((x) * 8192.0))
# define DOUBLE_TO_REAL_SCALE_LAYER12(x)	((long)((x) * 67108864.0))
# define DOUBLE_TO_REAL_SCALE_LAYER3(x, y)	((long)((x) * pow(2.0,gainpow2_scale[y])))
# define REAL_TO_DOUBLE(x)					((double)(x) / REAL_FACTOR)
# define REAL_MUL(x, y)						(((long long)(x) * (long long)(y)) >> REAL_RADIX)
# define REAL_MUL_SCALE_LAYER12(x, y)		(((long long)(x) * (long long)(y)) >> 26)
# define REAL_MUL_SCALE_LAYER3(x, y, z)		(((long long)(x) * (long long)(y)) >> (13 + gainpow2_scale[z] - REAL_RADIX))
# define REAL_SCALE_LAYER12(x)				((long)((x) >> 26))
# define REAL_SCALE_LAYER3(x, y)			((long)((x) >> (gainpow2_scale[y] - REAL_RADIX)))
#  define REAL_SCANF "%ld"
#  define REAL_PRINTF "%ld"

#else
#  define real double
#  define REAL_SCANF "%lf"
#  define REAL_PRINTF "%f"
#endif

#ifndef REAL_IS_FIXED
# if (defined SIZEOF_INT32_T) && (SIZEOF_INT32_T != 4)
#  error "Bad 32bit types!!!"
# endif
#endif

#ifndef DOUBLE_TO_REAL
# define DOUBLE_TO_REAL(x)					(real)(x)
#endif
#ifndef DOUBLE_TO_REAL_POW43
# define DOUBLE_TO_REAL_POW43(x)			(real)(x)
#endif
#ifndef DOUBLE_TO_REAL_SCALE_LAYER12
# define DOUBLE_TO_REAL_SCALE_LAYER12(x)	(real)(x)
#endif
#ifndef DOUBLE_TO_REAL_SCALE_LAYER3
# define DOUBLE_TO_REAL_SCALE_LAYER3(x, y)	(real)(x)
#endif
#ifndef REAL_TO_DOUBLE
# define REAL_TO_DOUBLE(x)					(x)
#endif

#ifndef REAL_MUL
# define REAL_MUL(x, y)						((x) * (y))
#endif
#ifndef REAL_MUL_SCALE_LAYER12
# define REAL_MUL_SCALE_LAYER12(x, y)		((x) * (y))
#endif
#ifndef REAL_MUL_SCALE_LAYER3
# define REAL_MUL_SCALE_LAYER3(x, y, z)		((x) * (y))
#endif
#ifndef REAL_SCALE_LAYER12
# define REAL_SCALE_LAYER12(x)				(x)
#endif
#ifndef REAL_SCALE_LAYER3
# define REAL_SCALE_LAYER3(x, y)			(x)
#endif

/* used to be: AUDIOBUFSIZE = n*64 with n=1,2,3 ...
   now: factor on minimum frame buffer size (which takes upsampling into account) */
#define		AUDIOBUFSIZE		2

#include "true.h"

#define         MAX_NAME_SIZE           81
#define         SBLIMIT                 32
#define         SCALE_BLOCK             12
#define         SSLIMIT                 18

/* Same as MPG_M_* */
#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

/* We support short or float output samples...
   Short integer amplitude is scaled by this. */
#define SHORT_SCALE 32768
/* That scales a short-scaled value to a 32bit integer scaled one
   value = 2**31/2**15 */
#define S32_RESCALE 65536

/* Pre Shift fo 16 to 8 bit converter table */
#define AUSHIFT (3)

#include "optimize.h"
#include "decode.h"
#include "parse.h"
#include "frame.h"

/* fr is a mpg123_handle* by convention here... */
#define NOQUIET  (!(fr->p.flags & MPG123_QUIET))
#define VERBOSE  (NOQUIET && fr->p.verbose)
#define VERBOSE2 (NOQUIET && fr->p.verbose > 1)
#define VERBOSE3 (NOQUIET && fr->p.verbose > 2)
#define VERBOSE4 (NOQUIET && fr->p.verbose > 3)
#define PVERB(mp, level) (!((mp)->flags & MPG123_QUIET) && (mp)->verbose >= (level))

int decode_update(mpg123_handle *mh);
/* residing in format.c  */
off_t samples_to_bytes(mpg123_handle *fr , off_t s);
off_t bytes_to_samples(mpg123_handle *fr , off_t b);

#endif

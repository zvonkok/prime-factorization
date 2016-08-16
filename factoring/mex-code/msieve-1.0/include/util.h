/*--------------------------------------------------------------------
This source distribution is placed in the public domain by its author,
Jason Papadopoulos. You may use it for any purpose, free of charge,
without having to notify anyone. I disclaim any responsibility for any
errors.

Optionally, please be nice and tell me if you find this source to be
useful. Again optionally, if you add to the functionality present here
please consider making those additions public too, so that others may 
benefit from your work.	
       				   --jasonp@boo.net 6/19/05
--------------------------------------------------------------------*/

#ifndef _UTIL_H_
#define _UTIL_H_

/* system-specific header files ---------------------------------------*/

#ifdef WIN32

	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <process.h>

#else /* !WIN32 */

	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <errno.h>
	#include <stdint.h>
	#include <pthread.h>

#endif /* WIN32 */


/* system-independent header files ------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <time.h>

/* basic types --------------------------------------------------------*/

#ifdef WIN32

	typedef unsigned int uint32;
	typedef int int32;
	typedef __int64 int64;
	typedef unsigned __int64 uint64;

#else /* !WIN32 */

	typedef uint32_t uint32;
	typedef uint64_t uint64;
	#ifndef RS6K
	typedef int32_t int32;
	typedef int64_t int64;
	#endif

#endif /* WIN32 */

typedef unsigned short uint16;
typedef unsigned char uint8;
#ifndef RS6K
typedef char int8;
typedef short int16;
#endif

/* useful functions ---------------------------------------------------*/

void * aligned_malloc(uint32 len, uint32 align);
void aligned_free(void *newptr);
uint64 read_clock(void);

#define MIN(a,b) ((a) < (b)? (a) : (b))
#define MAX(a,b) ((a) > (b)? (a) : (b))

#if defined(_MSC_VER)
	#define INLINE __inline
	#define getpid _getpid
#elif !defined(RS6K)
	#define INLINE inline
#else
	#define INLINE /* nothing */
#endif

#if defined(__GNUC__) 
	#define PREFETCH(addr) __builtin_prefetch(addr) 
#else
	#define PREFETCH(addr) /* nothing */
#endif

#ifndef M_LN2
#define M_LN2		0.69314718055994530942
#endif

static INLINE uint32 
get_rand(uint32 *rand_seed, uint32 *rand_carry) {
   
	/* A multiply-with-carry generator by George Marsaglia.
	   The period is about 2^63. */

	#define RAND_MULT 2131995753

	uint64 temp;

	temp = (uint64)(*rand_seed) * 
		       (uint64)RAND_MULT + 
		       (uint64)(*rand_carry);
	*rand_seed = (uint32)temp;
	*rand_carry = (uint32)(temp >> 32);
	return (uint32)temp;
}

#endif /* _UTIL_H_ */

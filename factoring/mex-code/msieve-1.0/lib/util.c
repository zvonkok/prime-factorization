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

#include <util.h>

/*---------------------------------------------------------------------*/
void *
aligned_malloc(uint32 len, uint32 align) {

   /* this code internally calls malloc() with a size > len,
      rounds the returned pointer up to the next "align"-byte
      boundary, stores the original pointer in the bytes just
      before that address and returns a pointer to aligned
      memory. aligned_free() reverses the process so that the
      padded address returned here is automatically accounted for.

      The arithmetic is messy because there's no guarantee that
      type "void *" is the same size as type "unsigned long". For
      example, on the Alpha you can specify a long to be 4 bytes
      but pointers are all 8 bytes in size. */
      
	void *ptr, *aligned_ptr;
	unsigned long addr;

	ptr = malloc((size_t)(len+align));
	if (ptr == NULL) {
		printf("memory allocation failed\n");
		exit(-1);
	}

	 /* offset to next ALIGN-byte boundary */

	addr = (unsigned long)ptr;				
	addr = align - (addr & (align-1));
	aligned_ptr = (void *)((char *)ptr + addr);

	*( (void **)aligned_ptr - 1 ) = ptr;
	return aligned_ptr;
}

/*---------------------------------------------------------------------*/
void
aligned_free(void *newptr) {

	void *ptr;

	if (newptr==NULL) return;
	ptr = *( (void **)newptr - 1 );
	free(ptr);
}

/*------------------------------------------------------------------*/
uint64
read_clock( void ) {

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
	uint32 lo, hi;
	asm("rdtsc":"=d"(hi),"=a"(lo));
	return (uint64)hi << 32 | lo;

#elif defined(_MSC_VER)
	LARGE_INTEGER ret;
	QueryPerformanceCounter(&ret);
	return ret.QuadPart;

#else
	struct timeval thistime;   
	gettimeofday(&thistime, NULL);
	return thistime.tv_sec * 1000000 + thistime.tv_usec;
#endif
}


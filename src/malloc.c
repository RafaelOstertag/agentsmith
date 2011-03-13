#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if (HAVE_MALLOC == 0)
#undef malloc

void *malloc ();

 /* 
  * Allocate an N-byte block of memory from the heap.  If N is zero, allocate a
  * 1-byte block.
  */
     
void *
rpl_malloc (size_t n) {
    if (n == 0)
	n = 1;
    return malloc (n);
}

#endif /* defined (HAVE_MALLOC == 0 ) */

#if (HAVE_REALLOC == 0)
#undef realloc

void *realloc ();

 /* 
  * Allocate an N-byte block of memory from the heap.  If N is zero, allocate a
  * 1-byte block.
  */
     
void *
rpl_realloc (void* ptr, size_t n) {
    if (n == 0)
	n = 1;
    return realloc (ptr, n);
}

#endif /* defined (HAVE_MALLOC == 0 ) */

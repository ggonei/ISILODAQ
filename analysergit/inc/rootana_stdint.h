#ifndef INCLUDE_rootana_stdintH
#define INCLUDE_rootana_stdintH

// We have to make our own definitions of uint32_t and uint16_t, since
// rootcint (ROOT 5) on Macos 10.0 doesn't want to process stdint.h
// T. Lindner 

#if defined(OS_DARWIN) && defined(HAVE_ROOT)

#ifndef _UINT32_T
#define _UINT32_T
typedef unsigned int uint32_t;
#endif /* _UINT32_T */

#ifndef _UINT16_T
#define _UINT16_T
typedef unsigned short uint16_t;
#endif /* _UINT16_T */


#else

#include <stdint.h>

#endif


#endif

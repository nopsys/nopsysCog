/* Unix sqConfig.h -- platform identification and configuration */

/* This file has been superseded by autoconf for Unix variants. */

#include "config.h"

#ifndef UNIX
# define UNIX
#endif

// improved support in unix sqConfig.h. Here we just don't care
# define VMBIGENDIAN 0


// is this needed???
#if defined(__GNUC__)
/* Define the "don't generate functions with register parameters" attribute
 * for x86 and similar.  Do so for debugging; gdb typically can't call static
 * functions that have been optimized to use register arguments.
 */
# if defined(_M_I386) || defined(_X86_) || defined(i386) || defined(i486) || defined(i586) || defined(i686) || defined(__i386__) || defined(__386__) || defined(X86) || defined(I386)
/*#	define PlatformNoDbgRegParms __attribute__ ((regparm (0)))*/
# endif
# define NeverInline __attribute__ ((noinline))
#endif

#if defined( __clang__)
# define NeverInline __attribute__ ((noinline))
#endif

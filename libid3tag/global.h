/*
 * libid3tag - ID3 tag manipulation library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: /sd/opensource/trunk/Audio-Scan/libid3tag/global.h 59471 2009-10-13T14:39:35.686417Z andy  $
 */

# ifndef LIBID3TAG_GLOBAL_H
# define LIBID3TAG_GLOBAL_H

#ifdef _MSC_VER
#include "perl.h"
#endif

/* trace debugging */

//#define TRACE_DEBUG

#ifdef TRACE_DEBUG
# include <stdio.h>
# define DEBUG_TRACE(...) fprintf(stderr, __VA_ARGS__)
#else
# define DEBUG_TRACE(...)
#endif

/* conditional debugging */

# if defined(DEBUG) && defined(NDEBUG)
#  error "cannot define both DEBUG and NDEBUG"
# endif

# if defined(DEBUG)
#  include <stdio.h>
#  include "debug.h"
#  define malloc(sz)        id3_debug_malloc(sz,       __FILE__, __LINE__)
#  define calloc(n, sz)     id3_debug_calloc(n, sz,    __FILE__, __LINE__)
#  define realloc(ptr, sz)  id3_debug_realloc(ptr, sz, __FILE__, __LINE__)
#  define free(ptr)         id3_debug_free(ptr,        __FILE__, __LINE__)
#  define release(ptr)      id3_debug_release(ptr,     __FILE__, __LINE__)
# else
#  ifdef _MSC_VER
#    undef free
#    define free(ptr)       Safefree(ptr)
#  endif
#  define release(ptr)  (ptr)
# endif

/* conditional features */

# if !defined(HAVE_ASSERT_H)
#  if defined(NDEBUG)
#   if !defined _MSC_VER
#     define assert(x)	/* nothing */
#   endif
#  else
#   define assert(x)	do { if (!(x)) abort(); } while (0)
#  endif
# endif

# endif

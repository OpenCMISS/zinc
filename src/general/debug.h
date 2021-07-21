/*******************************************************************************
FILE : debug.h

LAST MODIFIED : 10 October 2003

DESCRIPTION :
Function definitions for debugging.

==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (DEBUG_H)
#define DEBUG_H

#include <cstdarg>

#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/zincsharedobject.h"

/*
Macros
------
*/

#define ENTER( function_name )

#define LEAVE

/* Following are the only question mark operators allowed in CMGUI, needed
	because it is machine dependent whether a pointer or NULL is returned for an
	allocation of zero size. Inlined for optimisation */
#if (defined (OPTIMISED)) || (defined (NDEBUG))

#include <cstdlib>

#define ALLOCATE( result , type , number ) \
 ( result = ( 0 < ( number ) ) ? ( type * )malloc( ( number ) * sizeof( type ) ) : ( type * )NULL )

#define DEALLOCATE( ptr ) { if ( ptr ) { free( (char *)( ptr ) ); ( ptr ) = NULL; } }

#define REALLOCATE( final , initial , type , number ) \
 ( final = ( 0 < ( number ) ) ? ( type * )realloc( (char *)( initial ) , ( number ) * sizeof( type ) ) : ( type * )NULL )

#define ASSERT_IF( expression , return_code , error_value )

#else /* (defined (OPTIMISED)) || (defined (NDEBUG)) */

#include <cstddef> // to define size_t

#define ALLOCATE( result , type , number ) \
( result = ( type *) allocate( ( number ) * sizeof( type ) , __FILE__ , \
	__LINE__, #type ))

#define DEALLOCATE( ptr ) \
{ deallocate((char *) ptr , __FILE__ , __LINE__ ); ( ptr )=NULL;}

#define REALLOCATE( final , initial , type , number ) \
( final = ( type *) reallocate( (char *)( initial ) , \
	( number ) * sizeof( type ) , __FILE__ , __LINE__, #type ))

#endif /* (defined (OPTIMISED)) || (defined (NDEBUG)) */

/* Treatment of an int to store the value in a void* and to extract it later.
   The pointer to/from integer conversions are not necessary portable so a
   macro is defined here.  */
/*
  The following would would on many machines but the pointer difference is
  only defined if both pointers point at the same object.  Pointers could
  contain segment information and this may be bad if the segment doesn't
  exist.
#define INT2VOIDPTR( i ) ( (void *)((char *)0 - i ) )
#define VOIDPTR2INT( vp ) ( (char *) vp - (char *)0 )

  gcc complains about conversions between integer and pointer types when the
  types are of different sizes.  A double explicit cast with an intermediate
  integer the same size as a pointer silences these errors.  One explicit cast
  allows the pointer/integer conversion while the other silences the possible
  truncation warning.

  An integer type is required for this intermediate type.  It should be at
  least as large as the smallest of the integer or pointer types to avoid an
  unnecessary truncation.

  ptrdiff_t is widely available (in stddef.h) and large enough to hold the
  difference between two pointers (to the same object).
*/
#define INT2VOIDPTR( i ) ( (void *)(ptrdiff_t) i )
#define VOIDPTR2INT( vp ) ( (ptrdiff_t) vp )
/*
  It is possible that on some platforms the integer type (long maybe) and the
  pointer are both larger than ptrdiff_t, in which case the above would cause
  an unnecessary truncation.

  intptr_t is defined as an integer large enough to hold a pointer, but is not
  necessarily defined.  If available, this would provide an intermediate type
  that would avoid an unnecessary truncation.  If not defined in standard
  headers, we may be able to define it ourselves.

#if defined (AIX)
#include <inttypes.h>
#else
#include <stdint.h>
#endif
#define INT2VOIDPTR( i ) ( (void *)(intptr_t) i )
#define VOIDPTR2INT( vp ) ( (intptr_t) vp )
*/

/*
Global functions
----------------
*/
#if defined (USE_PARAMETER_ON)
void use_parameter(int dummy, ... );
/*******************************************************************************
LAST MODIFIED : 10 June 1999

DESCRIPTION :
Prototype for function which is called in the development stage (when
USE_PARAMETER_ON is defined) to swallow unused parameters to functions which
would otherwise cause compiler warnings. For example, parameter <dummy_void>
is swallowed with the call USE_PARAMETER(dummy_void); at the start of function.
==============================================================================*/
#  if !defined(__cplusplus)
#    define USE_PARAMETER(dummy) use_parameter(0,dummy)
#  else
#    define USE_PARAMETER(dummy) (void)dummy;
#  endif
#else /* defined (USE_PARAMETER_ON) */
#define USE_PARAMETER(dummy)
#endif /* defined (USE_PARAMETER_ON) */

#if !(defined (OPTIMISED)) || (defined (NDEBUG))
char *allocate(size_t size, const char *file_name,int line_number,const char *type);
/*******************************************************************************
LAST MODIFIED : 7 January 1998

DESCRIPTION :
Wrapper for malloc.
==============================================================================*/

void deallocate(char *ptr,const char *file_name,int line_number);
/*******************************************************************************
LAST MODIFIED : 7 January 1998

DESCRIPTION :
Wrapper for free.
==============================================================================*/

char *reallocate(char *ptr,size_t size,const char *file_name,int line_number,
	const char *type);
/*******************************************************************************
LAST MODIFIED : 7 January 1998

DESCRIPTION :
Wrapper for realloc.
==============================================================================*/
#endif /* !(defined (OPTIMISED)) || (defined (NDEBUG)) */

int list_memory(int count, int show_pointers, int increment_counter,
	int show_structures);
/*******************************************************************************
LAST MODIFIED : 29 February 2000

DESCRIPTION :
Writes out memory blocks currently allocated.  Each time this is called an
internal counter is incremented and all subsequent ALLOCATIONS marked with this
new count_number.  i.e. To find a leak run till before the leak (all these
allocations will be marked count 1), call list_memory increment, do the leaky
thing several times (these have count 2), call list_memory increment, do the
leaky thing once (so that any old stuff with count 2 should have been
deallocated and recreated with count 3) and then list_memory 2.  This should
list no memory.
If <count_number> is zero all the memory allocated is written out.
If <count_number> is negative no memory is written out, just the total.
If <count_number> is positive only the memory with that count is written out.
???DB.  printf used because want to make sure that no allocation is going on
	while printing.
<show_pointers> toggles the output format to include the actual memory addresses
or not.  (It isn't useful for testing and output to record the changing
addresses).  If <show_structures> is set then for known types the objects are
cast to the actual object type and then the appropriate list function is called.
==============================================================================*/

int set_check_memory_output(int on);
/*******************************************************************************
LAST MODIFIED : 19 November 2001

DESCRIPTION :
If <on> is non-zero then check memory output is turned on, otherwise, it is
turned off.  Check memory involves calling display_message to give memory
change information for ALLOCATE, DEALLOCATE and REALLOCATE.  display_message
is allowed to use ALLOCATE, DEALLOCATE or REALLOCATE (infinite recursion
prevented).
==============================================================================*/

#  if defined (UNIX)
/**
 * Print a stack trace from the current location.
 */
void stack_trace(void);
#endif /*  defined (UNIX) */

#endif /* !defined (DEBUG_H) */

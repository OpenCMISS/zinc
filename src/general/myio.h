/*******************************************************************************
FILE : myio.h

LAST MODIFIED : 12 June 2003

DESCRIPTION :
Some additions/modifications to stdio.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (MYIO_H)
#define MYIO_H

#include <stdio.h>

#include "opencmiss/zinc/zincconfigure.h"


#if defined (UNIX) && defined (GENERIC_PC)
#if defined (CYGWIN)
#include <sys/param.h>
#else /* defined (CYGWIN) */
#include <endian.h>
#endif /* defined (CYGWIN) */
#endif /* defined (UNIX) && defined (GENERIC_PC) */
#if defined (SGI)
#include <sys/endian.h>
#endif /* defined (SGI) */
#if defined (AIX)
#include <sys/machine.h>
#endif /* defined (AIX) */
#if defined (WIN32_SYSTEM)
#define BYTE_ORDER 1234
#endif /* defined (WIN32_SYSTEM) */
#if defined (DARWIN)
#include <machine/endian.h>
#endif /* defined (DARWIN) */

#if ! defined BYTE_ORDER
Warning BYTE_ORDER not defined
#endif /* ! defined BYTE_ORDER */

/*
Global macros
-------------
*/
#if defined (BYTE_ORDER) && (1234==BYTE_ORDER)
#define BINARY_FILE_READ( char_ptr,sizeof_type,count,binary_file ) \
fread_little_to_big_endian(char_ptr,sizeof_type,count,binary_file)

#define BINARY_FILE_WRITE( char_ptr,sizeof_type,count,binary_file ) \
fwrite_big_to_little_endian(char_ptr,sizeof_type,count,binary_file)
#else /* defined (BYTE_ORDER) && (1234==BYTE_ORDER) */
#define BINARY_FILE_READ( char_ptr,sizeof_type,count,binary_file ) \
fread(char_ptr,sizeof_type,count,binary_file)

#define BINARY_FILE_WRITE( char_ptr,sizeof_type,count,binary_file ) \
fwrite(char_ptr,sizeof_type,count,binary_file)
#endif /* defined (BYTE_ORDER) && (1234==BYTE_ORDER) */

/*
Global functions
----------------
*/
#if defined (BYTE_ORDER) && (1234==BYTE_ORDER)
int fread_little_to_big_endian(char *char_ptr,unsigned sizeof_type,int count,
	FILE *binary_file);
/*******************************************************************************
LAST MODIFIED : 4 September 1995

DESCRIPTION :
Does an fread and then converts from little to big endian.
==============================================================================*/

int fwrite_big_to_little_endian(char *char_ptr,unsigned sizeof_type,int count,
	FILE *binary_file);
/*******************************************************************************
LAST MODIFIED : 4 September 1995

DESCRIPTION :
fwrites the little endian form of <char_ptr>.
==============================================================================*/
#endif /* defined (BYTE_ORDER) && (1234==BYTE_ORDER) */

int get_line_number(FILE *stream);
/*******************************************************************************
LAST MODIFIED : 6 March 2000

DESCRIPTION :
Function for calculating the current line in a file.
==============================================================================*/
#endif /* !defined (MYIO_H) */

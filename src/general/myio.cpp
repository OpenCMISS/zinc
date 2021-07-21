/*******************************************************************************
FILE : myio.c

LAST MODIFIED : 23 August 2004

DESCRIPTION :
Some additions/modifications to stdio.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "general/debug.h"
#include "general/myio.h"
#include "general/message.h"

/*
Global functions
----------------
*/

#if defined (BYTE_ORDER) && (1234==BYTE_ORDER)
int fread_little_to_big_endian(char *char_ptr,unsigned sizeof_type,int count,
	FILE* binary_file)
/*******************************************************************************
LAST MODIFIED : 4 September 1995

DESCRIPTION :
Does an fread and then converts from little to big endian.
==============================================================================*/
{
  char *bottom_byte,byte,*element,*top_byte;
	int fread_result,i,j;

	ENTER(fread_little_to_big_endian);
	if (count==(fread_result=fread(char_ptr,sizeof_type,count,binary_file)))
	{
		if ((sizeof(short)==sizeof_type)||(sizeof(int)==sizeof_type)||
			(sizeof(float)==sizeof_type)||(sizeof(double)==sizeof_type))
		{
			element=char_ptr;
			for (j=count;j>0;j--)
			{
				bottom_byte=element;
				top_byte=element+sizeof_type;
				for (i=sizeof_type/2;i>0;i--)
				{
					top_byte--;
					byte= *bottom_byte;
					*bottom_byte= *top_byte;
					*top_byte=byte;
					bottom_byte++;
				}
				element += sizeof_type;
			}
		}
	}
	LEAVE;

	return (fread_result);
} /* fread_little_to_big_endian */

int fwrite_big_to_little_endian(char *char_ptr,unsigned sizeof_type,int count,
	FILE* binary_file)
/*******************************************************************************
LAST MODIFIED : 4 September 1995

DESCRIPTION :
fwrites the little endian form of <char_ptr>.
==============================================================================*/
{
  char *bottom_byte,*element,*top_byte,*temp_char_ptr;
	int fwrite_result,i,j;

	ENTER(fwrite_big_to_little_endian);
	if ((sizeof(short)==sizeof_type)||(sizeof(int)==sizeof_type)||
		(sizeof(float)==sizeof_type)||(sizeof(double)==sizeof_type))
	{
		if (ALLOCATE(temp_char_ptr,char,sizeof_type*count))
		{
			element=temp_char_ptr;
			bottom_byte=char_ptr;
			for (j=count;j>0;j--)
			{
				top_byte=element+sizeof_type;
				for (i=sizeof_type;i>0;i--)
				{
					top_byte--;
					*top_byte= *bottom_byte;
					bottom_byte++;
				}
				element += sizeof_type;
			}
			fwrite_result=fwrite(temp_char_ptr,sizeof_type,count,binary_file);
			DEALLOCATE(temp_char_ptr);
		}
		else
		{
			fwrite_result=0;
		}
	}
	else
	{
		fwrite_result=fwrite(char_ptr,sizeof_type,count,binary_file);
	}
	LEAVE;

	return (fwrite_result);
} /* fwrite_big_to_little_endian */
#endif /* defined (BYTE_ORDER) && (1234==BYTE_ORDER) */

int get_line_number(FILE *stream)
/*******************************************************************************
LAST MODIFIED : 21 June 2001

DESCRIPTION :
Function for calculating the current line in a file.
==============================================================================*/
{
	int c,line_number;
	long location,temp_location;

	ENTER(get_line_number);
	line_number=0;
	if (stream)
	{
		location = ftell(stream);
		rewind(stream);
		temp_location = ftell(stream);
		while (temp_location<location)
		{
			do
			{
				c=fgetc(stream);
			} while (('\n'!=c)&&(EOF!=c));
			temp_location = ftell(stream);
			line_number++;
		}
		/* Re-set the position in the stream to the original location */
		fseek(stream,location,SEEK_SET);
	}
	LEAVE;

	return (line_number);
} /* get_line_number */

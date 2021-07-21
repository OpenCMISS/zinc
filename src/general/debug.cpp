/*******************************************************************************
FILE : debug.c

LAST MODIFIED : 16 December 2001

DESCRIPTION :
Function definitions for debugging.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/core.h"

#include "general/message.h"
#include "general/debug.h"

/*#define USE_EXTERNAL_STRUCTS*/
#if defined (USE_EXTERNAL_STRUCTS)
/* This code, while useful for debugging, causes this file to be dependent
	on parts of cmgui it shouldn't so it isn't on by default. */
#include "finite_element/finite_element.h"
#include "graphics/mcubes.h"
#include "graphics/volume_texture.h"
#endif /* defined (USE_EXTERNAL_STRUCTS) */

/*#define MEMORY_CHECKING*/

#if defined (MEMORY_CHECKING)
/* Must override all these Macros before we use them to ensure we don't get
	stuck in an infinite loop of allocating */
#undef ALLOCATE
#define ALLOCATE( result , type , number ) \
( result = ( type *) malloc( ( number ) * sizeof( type ) ))

#undef DEALLOCATE
#define DEALLOCATE( ptr ) \
{ free((char *) ptr ); ( ptr )=NULL;}

#undef ENTER
#define ENTER( function_name )

#undef LEAVE
#define LEAVE

#undef REALLOCATE
#define REALLOCATE( final , initial , type , number ) \
( final = ( type *) realloc( (char *)( initial ) , \
	( number ) * sizeof( type )))

#include "general/compare.h"
#include "general/indexed_list_private.h"
#endif /* defined (MEMORY_CHECKING) */

/*
Module types
------------
*/
#if defined (MEMORY_CHECKING)
struct Memory_block
/*******************************************************************************
LAST MODIFIED : 18 May 1998

DESCRIPTION :
Keeps a record of where a block of memory was allocated
==============================================================================*/
{
	char *filename_line, *type;
	size_t count,size;
	void *ptr;
	int access_count;
}; /* struct Memory_block */
#endif /* defined (MEMORY_CHECKING) */

/*
Module functions
----------------
*/
#if defined (MEMORY_CHECKING)
static struct Memory_block *CREATE(Memory_block)(void *pointer,
	const char *filename, int line, const char *type_string, int size,
	int count)
/*******************************************************************************
LAST MODIFIED : 28 February 2000

DESCRIPTION :
==============================================================================*/
{
	struct Memory_block *block;

	ENTER(CREATE(Memory_block));
	if (ALLOCATE(block, struct Memory_block, 1))
	{
		block->ptr = pointer;
		block->size = size;
		block->count = count;
		block->access_count = 0;

		if (ALLOCATE(block->filename_line, char, strlen(filename)+20))
		{
			sprintf(block->filename_line,"%s : %10d",filename,line);
			if (ALLOCATE(block->type,char,strlen(type_string)+1))
			{
				strcpy(block->type, type_string);
			}
			else
			{
				display_message(ERROR_MESSAGE,"CREATE(Memory_block).  "
					"Unable to allocate memory for memory_block type string identifier");
				DEALLOCATE(block);
				DEALLOCATE(block->filename_line);
				block = (struct Memory_block *)NULL;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"CREATE(Memory_block).  "
				"Unable to allocate memory for memory_block string identifier");
			DEALLOCATE(block);
			block = (struct Memory_block *)NULL;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(Memory_block).  "
			"Unable to allocate memory for memory_block list structure");
		block = (struct Memory_block *)NULL;
	}
	LEAVE;

	return (block);
} /* CREATE(Memory_block) */

int DESTROY(Memory_block)(struct Memory_block **block_address)
/*******************************************************************************
LAST MODIFIED : 28 February 2000

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct Memory_block *block;

	ENTER(DESTROY(Memory_block));
	if (block_address && (block = *block_address))
	{
		if (block->access_count <= 0)
		{
			if (block->filename_line)
			{
				DEALLOCATE(block->filename_line);
			}
			if (block->type)
			{
				DEALLOCATE(block->type);
			}
			DEALLOCATE(*block_address);
			return_code = 1;
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"DESTROY(Memory_block).  Destroy called when access count > 0.");
			*block_address = (struct Memory_block *)NULL;
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"DESTROY(Memory_block).  Invalid arguments.");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* DESTROY(Memory_block) */

DECLARE_LIST_TYPES(Memory_block);

PROTOTYPE_OBJECT_FUNCTIONS(Memory_block);
PROTOTYPE_LIST_FUNCTIONS(Memory_block);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(Memory_block,ptr,void *);

DECLARE_OBJECT_FUNCTIONS(Memory_block)
FULL_DECLARE_INDEXED_LIST_TYPE(Memory_block);
DECLARE_INDEXED_LIST_MODULE_FUNCTIONS(Memory_block,ptr,void *,compare_pointer)
DECLARE_INDEXED_LIST_FUNCTIONS(Memory_block)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_FUNCTION(Memory_block,ptr,void *,
	compare_pointer)
#endif /* defined (MEMORY_CHECKING) */

#if defined (MEMORY_CHECKING)
struct List_memory_data
{
	int count;
	int show_pointers;
	int show_structures;
	size_t total;
	size_t *count_total;
}; /* struct List_memory_data */

static int list_memory_block(struct Memory_block *block,
	void *list_memory_data_void)
/*******************************************************************************
LAST MODIFIED : 28 February 2000

DESCRIPTION :
==============================================================================*/
{
	int return_code;
	struct List_memory_data *list_memory_data;

	ENTER(list_memory_block);
	if (block&&
		(list_memory_data = (struct List_memory_data *)list_memory_data_void))
	{
		if (list_memory_data->show_pointers)
		{
			if (!list_memory_data->count||(block->count==list_memory_data->count))
			{
				printf("%s @ %x size %ld type %s\n",block->filename_line,block->ptr,
					block->size, block->type);
			}
			list_memory_data->count_total[block->count] += block->size;
			list_memory_data->total += block->size;
		}
		else
		{
			if (!list_memory_data->count||(block->count==list_memory_data->count))
			{
				printf("%s size %ld type %s\n",block->filename_line,block->size,
					block->type);
			}
			list_memory_data->count_total[block->count] += block->size;
			list_memory_data->total += block->size;
		}
		if (list_memory_data->show_structures)
		{
			if (!strcmp(block->type, "char"))
			{
				printf("   \"%s\"\n", (char *)block->ptr);
			}
#if defined (USE_EXTERNAL_STRUCTS)
				/* This code, while useful for debugging, causes this file to be dependent
					on parts of cmgui it shouldn't. */
				else if (!strcmp(block->type, "struct FE_field"))
				{
					list_FE_field(block->ptr, NULL);
				}
				else if (!strcmp(block->type, "struct FE_element"))
				{
					//list_FE_element(block->ptr, NULL);
				}
				else if (!strcmp(block->type, "struct MC_triangle"))
				{
					struct MC_triangle *triangle = (struct MC_triangle *)block->ptr;
					printf("  triangle_index %d\n", triangle->triangle_index);
					printf("  vertex_indices %d,%d,%d\n", triangle->vertex_index[0],
						triangle->vertex_index[1], triangle->vertex_index[2]);
					printf("  cell_ptr 0x%x\n", triangle->cell_ptr);
					printf("  list_index = %d\n", triangle->triangle_list_index);
				}
				else if (!strcmp(block->type, "struct MC_vertex"))
				{
					struct MC_vertex *vertex = (struct MC_vertex *)block->ptr;
					printf("  vertex_index %d\n", vertex->vertex_index);
				}
				else if (!strcmp(block->type, "struct Triangle"))
				{
					struct Triangle *triangle = (struct Triangle *)block->ptr;
					printf("  vertex1 %lf,%lf,%lf\n", triangle->v[0][0], triangle->v[0][1], triangle->v[0][2]);
					printf("  vertex2 %lf,%lf,%lf\n", triangle->v[1][0], triangle->v[1][1], triangle->v[1][2]);
					printf("  vertex3 %lf,%lf,%lf\n", triangle->v[2][0], triangle->v[2][1], triangle->v[2][2]);
					printf("  clip_history1 %d\n", triangle->clip_history);
					printf("  cell_history2 %d\n", triangle->clip_history2);
				}
				else if (!strcmp(block->type, "struct MC_vertex"))
				{
					struct MC_vertex *vertex = (struct MC_vertex *)block->ptr;
					printf("  vertex_index %d\n", vertex->vertex_index);
				}
#endif /* defined (USE_EXTERNAL_STRUCTS) */
		}
		return_code = 1;
	}
	else
	{
		printf("Unable to allocate memory total array\n");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_memory_block */
#endif /* defined (MEMORY_CHECKING) */

/*
Module variables
----------------
*/
#if defined (MEMORY_CHECKING)
/* to prevent recursion when display_message uses allocate, deallocate or
	reallocate */
static int display_message_call_in_progress=0;
static int check_memory_output_on=0,maximum_count=1;
static size_t total_allocated_memory=0;
static struct LIST(Memory_block) *memory_block_list =
   (struct LIST(Memory_block) *)NULL;
#endif /* defined (MEMORY_CHECKING) */

/*
Global functions
----------------
*/
#if defined (USE_PARAMETER_ON)
void use_parameter(int dummy, ... )
/*******************************************************************************
LAST MODIFIED : 26 November 2001

DESCRIPTION :
Definition of function which is called in the development stage (when
USE_PARAMETER_ON is defined) to swallow unused parameters to functions which
would otherwise cause compiler warnings. For example, parameter <dummy_void>
is swallowed with the call USE_PARAMETER(dummy_void); at the start of function.
==============================================================================*/
{
	if (dummy)
	{
	}
} /* use_parameter */
#endif /* defined (USE_PARAMETER_ON) */

#if !(defined (OPTIMISED)) || (defined (NDEBUG))
char *allocate(size_t size,const char *filename,int line, const char *type)
/*******************************************************************************
LAST MODIFIED : 26 November 2001

DESCRIPTION :
Wrapper for allocate which keeps track of allocated memory.
==============================================================================*/
{
	char *result;
#if defined (MEMORY_CHECKING)
	size_t previous_total_allocated_memory;
	struct Memory_block *new_block;
#endif /* defined (MEMORY_CHECKING) */

	ENTER(allocate);
#if !defined (MEMORY_CHECKING)
	USE_PARAMETER(filename);
	USE_PARAMETER(line);
	USE_PARAMETER(type);
#endif /* !defined (MEMORY_CHECKING) */
	if (0<size)
	{
		result = static_cast<char *>(malloc(size));
		if (!result)
		{
#if defined (MEMORY_CHECKING)
			if (!display_message_call_in_progress)
			{
				display_message_call_in_progress=1;
#endif /* defined (MEMORY_CHECKING) */
				display_message(ERROR_MESSAGE,
					"allocate.  Insufficient memory.  Size=%ld",size);
#if defined (MEMORY_CHECKING)
				display_message_call_in_progress=0;
			}
#endif /* defined (MEMORY_CHECKING) */
		}
#if defined (MEMORY_CHECKING)
		else
		{
			if (!display_message_call_in_progress)
			{
				display_message_call_in_progress=1;
				previous_total_allocated_memory=total_allocated_memory;
				total_allocated_memory += size;
				if (check_memory_output_on)
				{
					display_message(INFORMATION_MESSAGE,"%ld +%ld %ld %s %d\n",
						previous_total_allocated_memory,size,total_allocated_memory,
						filename,line);
				}
				if (!memory_block_list)
				{
					memory_block_list = CREATE(LIST(Memory_block))();
				}
				if (new_block=CREATE(Memory_block)((void *)result,
					filename, line, type, size, maximum_count))
				{
					if (!ADD_OBJECT_TO_LIST(Memory_block)(new_block,memory_block_list))
					{
						display_message(ERROR_MESSAGE,
							"allocate.  Unable to add Memory_block to list");
						DESTROY(Memory_block)(&new_block);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"allocate.  Unable to create Memory_block");
				}
				display_message_call_in_progress=0;
			}
		}
#endif /* defined (MEMORY_CHECKING) */
	}
	else
	{
#if defined (MEMORY_CHECKING)
		if (!display_message_call_in_progress)
		{
			display_message_call_in_progress=1;
			display_message(WARNING_MESSAGE,"allocate.  Zero size.  %s : %10d",
				filename,line);
			display_message_call_in_progress=0;
		}
#else /* defined (MEMORY_CHECKING) */
		display_message(WARNING_MESSAGE,"allocate.  Zero size");
#endif /* defined (MEMORY_CHECKING) */
		result=(char *)NULL;
	}
	LEAVE;

	return (result);
} /* allocate */

void deallocate(char *ptr,const char *filename,int line)
/*******************************************************************************
LAST MODIFIED : 19 November 2001

DESCRIPTION :
Wrapper for deallocate which keeps track of allocated memory.
==============================================================================*/
{
#if defined (MEMORY_CHECKING)
	size_t previous_total_allocated_memory;
	struct Memory_block *block;
#endif /* defined (MEMORY_CHECKING) */

	ENTER(deallocate);
#if !defined (MEMORY_CHECKING)
	USE_PARAMETER(filename);
	USE_PARAMETER(line);
#endif /* !defined (MEMORY_CHECKING) */
	if (ptr)
	{
#if defined (MEMORY_CHECKING)
		if (!display_message_call_in_progress)
		{
			display_message_call_in_progress=1;
			if (memory_block_list && (block =
				FIND_BY_IDENTIFIER_IN_LIST(Memory_block,ptr)(ptr, memory_block_list)))
			{
				previous_total_allocated_memory=total_allocated_memory;
				total_allocated_memory -= block->size;
				if (check_memory_output_on)
				{
					display_message(INFORMATION_MESSAGE,"%ld -%ld %ld %s %d\n",
						previous_total_allocated_memory,block->size,total_allocated_memory,
						filename,line);
				}
				REMOVE_OBJECT_FROM_LIST(Memory_block)(block, memory_block_list);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"deallocate.  Could not find ptr %x in memory block list",ptr);
				display_message(ERROR_MESSAGE,
					"deallocate.  called from %s at line %d",filename,line);
				printf("deallocate.  Could not find ptr %x in memory block list\n",ptr);
				printf("deallocate.  called from %s at line %d\n",filename,line);
			}
			display_message_call_in_progress=0;
		}
#endif /* defined (MEMORY_CHECKING) */
		free(ptr);
	}
	LEAVE;
} /* deallocate */

char *reallocate(char *ptr,size_t size,const char *filename,int line, const char *type)
/*******************************************************************************
LAST MODIFIED : 19 November 2001

DESCRIPTION :
Wrapper for reallocate which keeps track of allocated memory.
==============================================================================*/
{
	char *result;
#if defined (MEMORY_CHECKING)
	size_t previous_size,previous_total_allocated_memory;
	struct Memory_block *block,*new_block;
#endif /* defined (MEMORY_CHECKING) */

	ENTER(reallocate);
#if !defined (MEMORY_CHECKING)
	USE_PARAMETER(filename);
	USE_PARAMETER(line);
	USE_PARAMETER(type);
#endif /* !defined (MEMORY_CHECKING) */
	if (0<size)
	{
#if defined (MEMORY_CHECKING)
		if (!display_message_call_in_progress)
		{
			display_message_call_in_progress=1;
			if (!memory_block_list)
			{
				memory_block_list = CREATE(LIST(Memory_block))();
			}
			if (ptr)
			{
				if (!(block = FIND_BY_IDENTIFIER_IN_LIST(Memory_block,ptr)(ptr,
					memory_block_list)))
				{
					display_message(ERROR_MESSAGE,
						"reallocate.  Could not find ptr %x in memory block list", ptr);
					display_message(ERROR_MESSAGE,
						"reallocate.  called from %s at line %d.", filename, line);
					printf("reallocate.  Could not find ptr %x in memory block list\n",
						ptr);
					printf("reallocate.  called from %s at line %d.\n", filename, line);
				}
			}
			else
			{
				block=(struct Memory_block *)NULL;
			}
			display_message_call_in_progress=0;
		}
#endif /* defined (MEMORY_CHECKING) */
		result = static_cast<char *>(realloc(ptr,size));
		if (!result)
		{
#if defined (MEMORY_CHECKING)
			if (!display_message_call_in_progress)
			{
				display_message_call_in_progress=1;
#endif /* defined (MEMORY_CHECKING) */
				display_message(ERROR_MESSAGE,
					"reallocate.  Insufficient memory.  Size=%ld",size);
#if defined (MEMORY_CHECKING)
				display_message_call_in_progress=0;
			}
#endif /* defined (MEMORY_CHECKING) */
		}
#if defined (MEMORY_CHECKING)
		else
		{
			if (!display_message_call_in_progress)
			{
				display_message_call_in_progress=1;
				previous_total_allocated_memory=total_allocated_memory;
				previous_size=0;
				if (block)
				{
					previous_size=block->size;
					total_allocated_memory -= block->size;
					if (strcmp(type, block->type))
					{
						display_message(ERROR_MESSAGE,"reallocate.  "
							"Allocation types don't match %s realloced at %s : %10d",
							block->filename_line, filename, line);
					}
					REMOVE_OBJECT_FROM_LIST(Memory_block)(block, memory_block_list);
				}
				total_allocated_memory += size;
				if (check_memory_output_on)
				{
					display_message(INFORMATION_MESSAGE,"%ld -%ld +%ld %ld %s %d\n",
						previous_total_allocated_memory,previous_size,size,
						total_allocated_memory,filename,line);
				}
				if (new_block=CREATE(Memory_block)((void *)result,
					filename, line, type, size, maximum_count))
				{
					if (!ADD_OBJECT_TO_LIST(Memory_block)(new_block,memory_block_list))
					{
						display_message(ERROR_MESSAGE,
							"reallocate.  Unable to add Memory_block to list");
						DESTROY(Memory_block)(&new_block);
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"reallocate.  Unable to create Memory_block");
				}
				display_message_call_in_progress=0;
			}
		}
#endif /* defined (MEMORY_CHECKING) */
	}
	else
	{
#if defined (MEMORY_CHECKING)
		if (display_message_call_in_progress)
		{
			display_message_call_in_progress=1;
			display_message(WARNING_MESSAGE,
				"reallocate.  Zero size requested.  %s : %10d",filename,line);
			display_message_call_in_progress=0;
		}
#else /* defined (MEMORY_CHECKING) */
		display_message(WARNING_MESSAGE,"reallocate.  Zero size requested ");
#endif /* defined (MEMORY_CHECKING) */
		result=ptr;
	}
	LEAVE;

	return (result);
} /* reallocate */
#endif /* !(defined (OPTIMISED)) || (defined (NDEBUG)) */

int list_memory(int count,int show_pointers,int increment_counter,
	int show_structures)
/*******************************************************************************
LAST MODIFIED : 26 November 2001

DESCRIPTION :
Writes out memory blocks currently allocated.  Each time this is called an
internal counter is incremented and all subsequent ALLOCATIONS marked with this
new <count>.  i.e. To find a leak run till before the leak (all these
allocations will be marked count 1), call list_memory increment, do the leaky
thing several times (these have count 2), call list_memory increment, do the
leaky thing once (so that any old stuff with count 2 should have been
deallocated and recreated with count 3) and then list_memory 2.  This should
list no memory.
If <count> is zero all the memory allocated is written out.
If <count> is negative no memory is written out, just the total.
If <count> is positive only the memory with that count is written out.
<show_pointers> toggles the output format to include the actual memory addresses
or not.  (It isn't useful for testing and output to record the changing
addresses).
If <show_structures> is set then for known types the objects are cast to the
actual object type and then the appropriate list function is called.
???DB.  printf used because want to make sure that no allocation is going on
	while printing.
==============================================================================*/
{
	int return_code;
#if defined (MEMORY_CHECKING)
	int i;
	struct List_memory_data list_memory_data;
#endif /* defined (MEMORY_CHECKING) */

	ENTER(list_memory);
#if defined (MEMORY_CHECKING)
	if (ALLOCATE(list_memory_data.count_total, size_t, maximum_count + 1))
	{
		for (i = 0 ; i < (maximum_count + 1) ; i++)
		{
			list_memory_data.count_total[i] = 0;
		}
		list_memory_data.count = count;
		list_memory_data.show_pointers = show_pointers;
		list_memory_data.show_structures = show_structures;
		list_memory_data.total = 0;

		printf("cmzn Memory Dump\n");

		if (memory_block_list)
		{
			FOR_EACH_OBJECT_IN_LIST(Memory_block)(list_memory_block,
				(void *)&list_memory_data, memory_block_list);
		}

		if (maximum_count > 1)
		{
			for (i = 1 ; i < maximum_count + 1 ; i++)
			{
				printf("  Allocated memory with count %3d: %12ld\n", i,
					list_memory_data.count_total[i]);
			}
		}
		DEALLOCATE(list_memory_data.count_total);
		printf("Total allocated memory:            %12ld\n", list_memory_data.total);
		if (increment_counter)
		{
			maximum_count++;
			printf("Now allocating with count: %d\n", maximum_count);
		}
		return_code = 1;
	}
	else
	{
		printf("Unable to allocate memory total array\n");
		return_code = 0;
	}
#else /* defined (MEMORY_CHECKING) */
	USE_PARAMETER(count);
	USE_PARAMETER(show_pointers);
	USE_PARAMETER(increment_counter);
	USE_PARAMETER(show_structures);
	return_code = 0;
#endif /* defined (MEMORY_CHECKING) */
	LEAVE;

	return (return_code);
} /* list_memory */

int set_check_memory_output(int on)
/*******************************************************************************
LAST MODIFIED : 16 December 2001

DESCRIPTION :
If <on> is non-zero then check memory output is turned on, otherwise, it is
turned off.  Check memory involves calling display_message to give memory
change information for ALLOCATE, DEALLOCATE and REALLOCATE.  display_message
is allowed to use ALLOCATE, DEALLOCATE or REALLOCATE (infinite recursion
prevented).
==============================================================================*/
{
	int return_code;

	ENTER(set_check_memory_output);
	return_code=1;
#if defined (MEMORY_CHECKING)
	if (on)
	{
		check_memory_output_on=1;
	}
	else
	{
		check_memory_output_on=0;
	}
#else /* defined (MEMORY_CHECKING) */
	USE_PARAMETER(on);
#endif /* defined (MEMORY_CHECKING) */
	LEAVE;

	return (return_code);
} /* set_check_memory_output */

void *cmzn_allocate(int bytes)
/*******************************************************************************
LAST MODIFIED : 12 August 2003

DESCRIPTION :
==============================================================================*/
{
	void *return_ptr;

	ENTER(cmzn_allocate);
	if (bytes)
	{
		ALLOCATE(return_ptr, char, bytes);
	}
	else
	{
		return_ptr = NULL;
	}
	LEAVE;

	return (return_ptr);
} /* cmzn_allocate */

int cmzn_deallocate(void *ptr)
/*******************************************************************************
LAST MODIFIED : 12 September 2002

DESCRIPTION :
Frees the memory associated with the pointer.  Used to clean up when functions
return buffers allocated internally to cmiss.
==============================================================================*/
{
	int return_code;

	ENTER(cmzn_deallocate);
	if (ptr)
	{
		DEALLOCATE(ptr);
		return_code = 1;
	}
	else
	{
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_deallocate */

#  if defined (UNIX)
#include <execinfo.h>

void stack_trace(void)
{
	int j, nptrs;
	#define SIZE 100
	void *buffer[100];
	char **strings;

	nptrs = backtrace(buffer, SIZE);
	printf("backtrace() returned %d addresses\n", nptrs);

	/* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	would produce similar output to the following: */

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL)
	{
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

	for (j = 0; j < nptrs; j++)
		printf("%s\n", strings[j]);

	free(strings);
}

#endif /* defined (UNIX) */


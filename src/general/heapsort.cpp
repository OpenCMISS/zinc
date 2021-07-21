/*******************************************************************************
FILE : heapsort.c

LAST MODIFIED : 2 November 1995

DESCRIPTION :
Created because qsort didn't seem to be working properly in TurboC.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stddef.h>

#include "general/heapsort.h"
#include "general/debug.h"

int heapsort(void *base,size_t count,size_t size,
	int (*compare)(const void *,const void *))
/*******************************************************************************
LAST MODIFIED : 2 November 1995

DESCRIPTION :
An implementation of heapsort as described in "Numerical Recipes.  The Art of
Scientific Computing" by W.H. Press, B.P. Flannery, S.A. Teukolsky and W.T.
Vetterling.
==============================================================================*/
{
	int l,ir,i,j,k;
	char *raa=NULL;

	ENTER(heapsort);
	if (ALLOCATE(raa,char,size))
	{
		l=count/2+1;
		ir=count;
		/* The index l will be decremented from its initial value down to 1 during
			the heap createion phase.  Once l reaches 1, the index ir will be
			decremented from its initial value down to 1 during the heap selection
			phase. */
		while (ir>1)
		{
			if (l>1)
			{
				l--;
				for (k=size;k>0;)
				{
					k--;
					*(raa+k)= *(((char *)base)+(l-1)*size+k);
				}
			}
			else
			{
				for (k=size;k>0;)
				{
					k--;
					*(raa+k)= *(((char *)base)+(ir-1)*size+k);
					*(((char *)base)+(ir-1)*size+k)= *(((char *)base)+k);
				}
				ir--;
			}
			i=l;
			j=l+l;
			while (j<=ir)
			{
				if (j<ir)
				{
					if (compare((void *)(((char *)base)+(j-1)*size),
						(void *)(((char *)base)+j*size))<0)
					{
						j++;
					}
				}
				if (compare((void *)raa,(void *)(((char *)base)+(j-1)*size))<0)
				{
					for (k=size;k>0;)
					{
						k--;
						*(((char *)base)+(i-1)*size+k)= *(((char *)base)+(j-1)*size+k);
					}
					i=j;
					j=j+j;
				}
				else
				{
					j=ir+1;
				}
			}
			for (k=size;k>0;)
			{
				k--;
				*(((char *)base)+(i-1)*size+k)= *(raa+k);
			}
		}
		DEALLOCATE(raa);
	}
	LEAVE;

	return(1);
} /* heapsort */

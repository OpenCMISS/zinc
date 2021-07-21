/*******************************************************************************
FILE : heapsort.h

LAST MODIFIED : 4 July 1992

DESCRIPTION :
Created because qsort didn't seem to be working properly in TurboC.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stddef.h>

int heapsort(void *base,size_t count,size_t size,
	int (*compare)(const void *,const void *));
/*******************************************************************************
LAST MODIFIED : 5 October 1991

DESCRIPTION :
An implementation of heapsort as described in "Numerical Recipes.  The Art of
Scientific Computing" by W.H. Press, B.P. Flannery, S.A. Teukolsky and W.T.
Vetterling.
==============================================================================*/

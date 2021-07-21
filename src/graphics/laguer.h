/*******************************************************************************
FILE : laguer.h

LAST MODIFIED : 24 November 1994

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (LAGUER_H)
#define LAGUER_H

void laguer(fcomplex a[], int m, fcomplex *x, int *its);
void zroots(fcomplex a[], int m, fcomplex roots[], int polish);

#endif

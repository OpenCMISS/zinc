/*******************************************************************************
FILE : statistics.h

LAST MODIFIED : 4 May 2001

DESCRIPTION :
Statistical functions, eg. sampling from distributions, used by CMGUI.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (STATISTICS_H)
#define STATISTICS_H

int sample_Poisson_distribution(double mean);
/*******************************************************************************
LAST MODIFIED : 4 May 2001

DESCRIPTION :
Returns a single sample from a Poisson distribution with the given <mean>.
==============================================================================*/

#endif /* !defined (STATISTICS_H) */

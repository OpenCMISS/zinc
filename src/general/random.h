/*******************************************************************************
FILE : random.h

LAST MODIFIED : 26 June 2002

DESCRIPTION :
Standard macros for returning random numbers.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if !defined (RANDOM_H)
#define RANDOM_H


#include "opencmiss/zinc/zincconfigure.h"


#if defined (UNIX) /* switch (OPERATING_SYSTEM) */
/* Must #include <stdlib.h> in calling module to use: */
/* Returns a random number of the given <type> in the range [0.0, 1.0]
	 ie. INCLUDING 0.0 and 1.0 in the possible results */
#define CMGUI_RANDOM(type) ((type)random() / 2147483647.0)

/* Returns a random number of the given <type> in the range (0.0, 1.0)
	 ie. NOT INCLUDING 0.0 and 1.0 in the possible results */
#define CMGUI_RANDOM_NON_INCLUSIVE(type) (((type)random() + 1.0) / 2147483649.0)

/* partner random number seed function for CMGUI_RANDOM */
#define CMGUI_SEED_RANDOM(seed) srandom(seed)

#elif defined (WIN32_SYSTEM) /* switch (OPERATING_SYSTEM) */

/* Must #include <stdlib.h> in calling module to use: */
/* Returns a random number of the given <type> in the range [0.0, 1.0]
	 ie. INCLUDING 0.0 and 1.0 in the possible results */
#define CMGUI_RANDOM(type) ((type)rand() / (type)RAND_MAX)

/* Returns a random number of the given <type> in the range (0.0, 1.0)
	 ie. NOT INCLUDING 0.0 and 1.0 in the possible results */
#define CMGUI_RANDOM_NON_INCLUSIVE(type) (((type)rand() + 1.0) / ((type)RAND_MAX + 2.0))

/* partner random number seed function for CMGUI_RANDOM */
#define CMGUI_SEED_RANDOM(seed) srand(seed)

#endif /* switch (OPERATING_SYSTEM) */
#endif /* !defined (RANDOM_H) */

/*******************************************************************************
FILE : statistics.c

LAST MODIFIED : 7 May 2001

DESCRIPTION :
Statistical functions, eg. sampling from distributions, used by CMGUI.
==============================================================================*/
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "general/debug.h"
#include "general/geometry.h"
#include "general/random.h"
#include "general/message.h"

/*
Global functions
----------------
*/

#define HALF_LOG_2_PI 0.91893853320467267

/* pass integer n to this macro to return Stirling's approximation to log(N!) */
#define STIRLING_APPROXIMATION_LOG_N_FACTORIAL(n) \
((n + 0.5)*log(n) - n + HALF_LOG_2_PI)

int sample_Poisson_distribution(double mean)
/*******************************************************************************
LAST MODIFIED : 7 May 2001

DESCRIPTION :
Returns a single sample from a Poisson distribution with the given <mean>.
Uses Atkinson's method to handle large means, details found at:
http://www.jesus.ox.ac.uk/~clifford/a5/chap1/node13.html
Also verified from book:
"Stochastic Simulation" by Brian. D. Ripley, Wiley 1987 p79.
==============================================================================*/
{
	double a, b, c, k, limit, log_mean, product, u, x, y, z;
	int number;

	ENTER(sample_Poisson_distribution);
	if (0.0 <= mean)
	{
		if (mean < 30.0)
		{
			/* the simple method of sampling from a Poisson distribution is to
				 multiply uniform random variables on (0.0, 1.0) until they are less
				 than the appropirate limit; return the number of random variables
				 calculated less 1. This fails as the value exp(-mean) underflows with
				 fairly moderate mean values, and will also be less accurate as this
				 condition is approached */
			product = CMGUI_RANDOM_NON_INCLUSIVE(double);
			limit = exp(-mean);
			number = 0;
			while (product > limit)
			{
				product *= CMGUI_RANDOM_NON_INCLUSIVE(double);
				number++;
			}
		}
		else
		{
			/* Atkinson's method */
			b = PI / sqrt(3*mean);
			a = b * mean;
			c = .767 - (3.36 / mean);
			k = log(c / b) - mean;
			log_mean = log(mean);
			do
			{
				do
				{
					u = CMGUI_RANDOM_NON_INCLUSIVE(double);
					y = (a - log((1.0 - u) / u)) / b;
				}
				while (y <= -0.5);
				number = (int)(y + 0.5);
				x = (double)number;
				z = k + x*log_mean - STIRLING_APPROXIMATION_LOG_N_FACTORIAL(x);
			}
			while (log(CMGUI_RANDOM_NON_INCLUSIVE(double)*u*(1.0 - u)) > z);
			/* note Ripley algorithm requires > z in the above; clifford uses >= z;
				 I have converted "until (condition)" into "while (!condition)" */
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"sample_Poisson_distribution.  Negative mean");
		number = 0;
	}
	LEAVE;

	return (number);
} /* sample_Poisson_distribution */

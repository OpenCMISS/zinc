/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/* CAUTION: This is the ANSI C (only) version of the Numerical Recipes
	utility file complex.c.  Do not confuse this file with the same-named
	file complex.c that is supplied in the same subdirectory or archive
	as the header file complex.h.  *That* file contains both ANSI and
	traditional K&R versions, along with #ifdef macros to select the
	correct version.  *This* file contains only ANSI C.               */

#include <math.h>

#include "opencmiss/zinc/zincconfigure.h"
#define NRANSI
#include "graphics/complex.h"
#undef NRANSI

fcomplex Cadd(fcomplex a, fcomplex b)
{
	static fcomplex c;
	c.r=a.r+b.r;
	c.i=a.i+b.i;
	return c;
}

fcomplex Csub(fcomplex a, fcomplex b)
{
	static fcomplex c;
	c.r=a.r-b.r;
	c.i=a.i-b.i;
	return c;
}


fcomplex Cmul(fcomplex a, fcomplex b)
{
	static fcomplex c;
	c.r=a.r*b.r-a.i*b.i;
	c.i=a.i*b.r+a.r*b.i;
	return c;
}

fcomplex Complexr(ZnReal re, ZnReal im)
{
	static fcomplex c;
	c.r=re;
	c.i=im;
	return c;
}

fcomplex Conjg(fcomplex z)
{
	static fcomplex c;
	c.r=z.r;
	c.i = -z.i;
	return c;
}

fcomplex Cdiv(fcomplex a, fcomplex b)
{
	static fcomplex c;
	ZnReal r,den;
	if (fabs(b.r) >= fabs(b.i)) {
		r=b.i/b.r;
		den=b.r+r*b.i;
		c.r=(a.r+r*a.i)/den;
		c.i=(a.i-r*a.r)/den;
	} else {
		r=b.r/b.i;
		den=b.i+r*b.r;
		c.r=(a.r*r+a.i)/den;
		c.i=(a.i*r-a.r)/den;
	}
	return c;
}

ZnReal Cabs(fcomplex z)
{
	static ZnReal x,y,ans,temp;
	x=fabs(z.r);
	y=fabs(z.i);
	if (x == 0.0)
		ans=y;
	else if (y == 0.0)
		ans=x;
	else if (x > y) {
		temp=y/x;
		ans=x*sqrt(1.0+temp*temp);
	} else {
		temp=x/y;
		ans=y*sqrt(1.0+temp*temp);
	}
	return ans;
}

fcomplex Csqrt(fcomplex z)
{
	static fcomplex c;
	ZnReal x,y,w,r;
	if ((z.r == 0.0) && (z.i == 0.0)) {
		c.r=0.0;
		c.i=0.0;
		return c;
	} else {
		x=fabs(z.r);
		y=fabs(z.i);
		if (x >= y) {
			r=y/x;
			w=sqrt(x)*sqrt(0.5*(1.0+sqrt(1.0+r*r)));
		} else {
			r=x/y;
			w=sqrt(y)*sqrt(0.5*(r+sqrt(1.0+r*r)));
		}
		if (z.r >= 0.0) {
			c.r=w;
			c.i=z.i/(2.0*w);
		} else {
			c.i=(z.i >= 0) ? w : -w;
			c.r=z.i/(2.0*c.i);
		}
		return c;
	}
}

fcomplex RCmul(ZnReal x, fcomplex a)
{
	static fcomplex c;
	c.r=x*a.r;
	c.i=x*a.i;
	return c;
}
/* (C) Copr. 1986-92 Numerical Recipes Software ")121"1w#3Y". */
